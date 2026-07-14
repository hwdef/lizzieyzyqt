#include "AnalysisProcess.h"

#include "ProcessDiagnostics.h"
#include "ProcessEnvironment.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

namespace lizzie::engine {

namespace {

lizzie::core::Color sideToMoveForRequest(const AnalysisRequest& request)
{
    if (request.moves.empty()) {
        return request.initialPlayer;
    }
    return lizzie::core::opponent(request.moves.back().color);
}

std::optional<std::string> idFromJsonLine(const QString& line)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(line.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return std::nullopt;
    }
    const QString id = document.object().value("id").toString();
    if (id.isEmpty()) {
        return std::nullopt;
    }
    return id.toStdString();
}

}  // namespace

AnalysisProcess::AnalysisProcess(QObject* parent)
    : QObject(parent)
{
    cancelKillTimer_.setSingleShot(true);
    cancelKillTimer_.setInterval(3000);
    connect(&cancelKillTimer_, &QTimer::timeout, this, [this] {
        if (cancelling_ && process_.state() != QProcess::NotRunning) {
            process_.kill();
        }
    });
    connect(&process_, &QProcess::started, this, &AnalysisProcess::handleStarted);
    connect(&process_, &QProcess::readyReadStandardOutput, this, &AnalysisProcess::handleStdout);
    connect(&process_, &QProcess::readyReadStandardError, this, &AnalysisProcess::handleStderr);
    connect(&process_, &QProcess::errorOccurred, this, &AnalysisProcess::handleError);
    connect(&process_, &QProcess::finished, this, &AnalysisProcess::handleFinished);
}

bool AnalysisProcess::isRunning() const
{
    return process_.state() != QProcess::NotRunning;
}

void AnalysisProcess::startAnalysis(const KataGoConfig& config, const std::vector<AnalysisRequest>& requests)
{
    if (isRunning()) {
        cancelAndWaitForRestart();
    }

    ++runSerial_;
    finishedEmitted_ = false;
    failedEmitted_ = false;
    requests_ = requests;
    toMoveById_.clear();
    boardSizeById_.clear();
    completedIds_.clear();
    stdoutBuffer_.clear();
    stderrBuffer_.clear();
    recentStderrLines_.clear();
    completed_ = 0;
    cancelling_ = false;

    if (!config.workingDirectory.empty()) {
        process_.setWorkingDirectory(QString::fromStdString(config.workingDirectory.string()));
    } else {
        process_.setWorkingDirectory({});
    }
    process_.setProgram(QString::fromStdString(config.executable.string()));
    process_.setArguments(toQStringList(config.analysisArguments()));
    process_.setProcessEnvironment(processEnvironmentForConfig(config));
    for (const AnalysisRequest& request : requests_) {
        if (request.id.empty()) {
            emitFailedOnce("analysis request id is required");
            finishRun(false);
            return;
        }
        if (toMoveById_.contains(request.id)) {
            emitFailedOnce(QString("duplicate analysis request id: %1").arg(QString::fromStdString(request.id)));
            finishRun(false);
            return;
        }
        toMoveById_[request.id] = sideToMoveForRequest(request);
        boardSizeById_[request.id] = request.boardSize;
    }
    if (!config.hasAnalysisMinimum()) {
        emitFailedOnce("KataGo executable, model, and analysis config are required");
        finishRun(false);
        return;
    }
    process_.start();
}

void AnalysisProcess::cancel()
{
    if (!isRunning()) {
        return;
    }
    if (cancelling_) {
        return;
    }
    cancelling_ = true;
    process_.terminate();
    cancelKillTimer_.start();
}

void AnalysisProcess::cancelAndWait()
{
    cancelAndWaitForRestart();
}

void AnalysisProcess::handleStarted()
{
    emit started();
    emit progressChanged(0, static_cast<int>(requests_.size()));
    writeRequests();
}

void AnalysisProcess::handleStdout()
{
    stdoutBuffer_ += process_.readAllStandardOutput();
    consumeLines(&stdoutBuffer_, QProcess::StandardOutput);
}

void AnalysisProcess::handleStderr()
{
    stderrBuffer_ += process_.readAllStandardError();
    consumeLines(&stderrBuffer_, QProcess::StandardError);
}

void AnalysisProcess::handleError(QProcess::ProcessError error)
{
    if (cancelling_ && error == QProcess::Crashed) {
        return;
    }
    handleStderr();
    emitFailedOnce(process_.errorString(), error);
    if (error == QProcess::FailedToStart) {
        cancelKillTimer_.stop();
        finishRun(false);
    }
}

void AnalysisProcess::handleFinished(int exitCode, QProcess::ExitStatus status)
{
    const std::uint64_t serial = runSerial_;
    cancelKillTimer_.stop();
    QTimer::singleShot(0, this, [this, serial, exitCode, status] {
        if (serial != runSerial_ || finishedEmitted_) {
            return;
        }
        handleStdout();
        handleStderr();
        if (!stdoutBuffer_.isEmpty()) {
            handleStdoutLine(QString::fromUtf8(stdoutBuffer_));
            stdoutBuffer_.clear();
        }
        if (!stderrBuffer_.isEmpty()) {
            const QString line = QString::fromUtf8(stderrBuffer_);
            rememberStderrLine(line);
            emit stderrLineReceived(line);
            stderrBuffer_.clear();
        }

        const bool failedExit = !cancelling_ && (status != QProcess::NormalExit || exitCode != 0);
        if (failedExit) {
            emitFailedOnce(
                QString("KataGo analysis exited: code %1, status %2").arg(exitCode).arg(static_cast<int>(status)));
        } else if (!cancelling_ && completed_ < requests_.size()) {
            emitFailedOnce(QString("KataGo analysis ended before all responses: %1/%2 completed")
                               .arg(completed_)
                               .arg(requests_.size()));
        }

        finishRun(cancelling_);
    });
}

void AnalysisProcess::cancelAndWaitForRestart()
{
    if (!isRunning()) {
        return;
    }
    cancelling_ = true;
    process_.terminate();
    if (!process_.waitForFinished(3000)) {
        process_.kill();
        process_.waitForFinished(1000);
    }
    cancelKillTimer_.stop();
}

void AnalysisProcess::consumeLines(QByteArray* buffer, QProcess::ProcessChannel channel)
{
    for (;;) {
        const qsizetype newline = buffer->indexOf('\n');
        if (newline < 0) {
            break;
        }
        const QByteArray line = buffer->left(newline);
        buffer->remove(0, newline + 1);
        if (channel == QProcess::StandardOutput) {
            handleStdoutLine(QString::fromUtf8(line));
        } else {
            const QString text = QString::fromUtf8(line);
            rememberStderrLine(text);
            emit stderrLineReceived(text);
        }
    }
}

void AnalysisProcess::handleStdoutLine(QString line)
{
    if (line.endsWith('\r')) {
        line.chop(1);
    }
    emit rawLineReceived(line);

    const std::optional<std::string> id = idFromJsonLine(line);
    if (!id.has_value()) {
        emit parseFailed(line);
        return;
    }
    const auto iter = toMoveById_.find(*id);
    const auto boardSizeIter = boardSizeById_.find(*id);
    if (iter == toMoveById_.end() || boardSizeIter == boardSizeById_.end()) {
        emit unknownResponseIgnored(QString::fromStdString(*id), line);
        return;
    }

    const std::optional<AnalysisResponse> response =
        parseAnalysisResponse(line.toStdString(), boardSizeIter->second, iter->second);
    if (!response.has_value()) {
        emit parseFailed(line);
        return;
    }
    if (!completedIds_.insert(response->id).second) {
        emit duplicateResponseIgnored(QString::fromStdString(response->id), line);
        return;
    }

    ++completed_;
    if (!response->errorMessage.empty()) {
        emit requestFailed(
            QString::fromStdString(response->id),
            QString::fromStdString(response->errorMessage),
            line);
    } else {
        emit responseReceived(*response);
    }
    emit progressChanged(completed_, static_cast<int>(requests_.size()));
}

void AnalysisProcess::writeRequests()
{
    for (const AnalysisRequest& request : requests_) {
        const std::string line = request.toJsonLine();
        process_.write(line.data(), static_cast<qint64>(line.size()));
    }
    process_.closeWriteChannel();
}

void AnalysisProcess::rememberStderrLine(const QString& line)
{
    QString trimmed = line;
    while (trimmed.endsWith('\n') || trimmed.endsWith('\r')) {
        trimmed.chop(1);
    }
    if (trimmed.isEmpty()) {
        return;
    }
    recentStderrLines_.push_back(trimmed);
    while (recentStderrLines_.size() > 12) {
        recentStderrLines_.removeFirst();
    }
}

void AnalysisProcess::emitFailedOnce(const QString& summary, std::optional<QProcess::ProcessError> error)
{
    if (failedEmitted_) {
        return;
    }
    failedEmitted_ = true;
    emit failed(diagnosticMessage(summary, error));
}

void AnalysisProcess::finishRun(bool cancelled)
{
    if (finishedEmitted_) {
        return;
    }
    finishedEmitted_ = true;
    emit finished(cancelled);
    requests_.clear();
    toMoveById_.clear();
    boardSizeById_.clear();
    cancelling_ = false;
}

QString AnalysisProcess::diagnosticMessage(const QString& summary, std::optional<QProcess::ProcessError> error) const
{
    QStringList lines;
    lines.push_back(summary);
    if (error.has_value()) {
        lines.push_back(QString("Process error: %1").arg(static_cast<int>(*error)));
    }
    lines.push_back("Command: " + commandLineForDiagnostics(process_));
    const QString workingDirectory = process_.workingDirectory().isEmpty()
        ? QDir::currentPath() + " (inherited)"
        : process_.workingDirectory();
    lines.push_back("Working directory: " + workingDirectory);
    lines.push_back("Library path: " + processLibraryPathForDiagnostics(process_.processEnvironment()));
    lines.push_back("Stderr: " + stderrSummary());
    return lines.join('\n');
}

QString AnalysisProcess::stderrSummary() const
{
    QStringList lines = recentStderrLines_;
    if (!stderrBuffer_.isEmpty()) {
        lines.push_back(QString::fromUtf8(stderrBuffer_).trimmed());
    }
    lines.removeAll(QString());
    if (lines.isEmpty()) {
        return "(none)";
    }
    QString summary = lines.join('\n');
    constexpr qsizetype kMaxLength = 4000;
    if (summary.size() > kMaxLength) {
        summary = "..." + summary.right(kMaxLength);
    }
    return summary;
}

}  // namespace lizzie::engine
