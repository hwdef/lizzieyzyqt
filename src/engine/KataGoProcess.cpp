#include "KataGoProcess.h"

#include "ProcessDiagnostics.h"
#include "ProcessEnvironment.h"

#include <QDir>
#include <QTimer>

namespace lizzie::engine {

KataGoProcess::KataGoProcess(QObject* parent)
    : QObject(parent)
{
    connect(&process_, &QProcess::started, this, &KataGoProcess::handleStarted);
    connect(&process_, &QProcess::readyReadStandardOutput, this, &KataGoProcess::handleStdout);
    connect(&process_, &QProcess::readyReadStandardError, this, &KataGoProcess::handleStderr);
    connect(&process_, &QProcess::errorOccurred, this, &KataGoProcess::handleError);
    connect(&process_, &QProcess::finished, this, &KataGoProcess::handleFinished);
}

bool KataGoProcess::isRunning() const
{
    return process_.state() != QProcess::NotRunning;
}

int KataGoProcess::nextCommandId() const
{
    return queue_.nextCommandId();
}

void KataGoProcess::startGtp(const KataGoConfig& config)
{
    if (isRunning()) {
        stop();
    }
    ++runSerial_;
    parser_.reset();
    queue_.reset();
    stdoutBuffer_.clear();
    stderrBuffer_.clear();
    recentStderrLines_.clear();
    stopping_ = false;
    failureReported_ = false;

    if (!config.workingDirectory.empty()) {
        process_.setWorkingDirectory(QString::fromStdString(config.workingDirectory.string()));
    } else {
        process_.setWorkingDirectory({});
    }
    process_.setProgram(QString::fromStdString(config.executable.string()));
    process_.setArguments(toQStringList(config.gtpArguments()));
    process_.setProcessEnvironment(processEnvironmentForConfig(config));
    if (!config.hasGtpMinimum()) {
        emit startupFailed(diagnosticMessage("KataGo executable, model, and GTP config are required"));
        return;
    }
    process_.start();
}

void KataGoProcess::stop()
{
    if (!isRunning()) {
        return;
    }
    stopping_ = true;
    if (process_.state() == QProcess::Running) {
        process_.write("quit\n");
        process_.closeWriteChannel();
        if (process_.waitForFinished(1000)) {
            return;
        }
    }
    process_.terminate();
    if (!process_.waitForFinished(3000)) {
        process_.kill();
        process_.waitForFinished(1000);
    }
}

int KataGoProcess::sendCommand(const GtpCommand& command)
{
    if (!isRunning()) {
        emit startupFailed(diagnosticMessage("KataGo process is not running"));
        return -1;
    }
    if (!isValidGtpCommandName(command.name)) {
        return -1;
    }

    GtpCommand outgoing = queue_.enqueue(command);

    const std::string line = outgoing.toLine();
    process_.write(line.data(), static_cast<qint64>(line.size()));
    emit protocolLineReceived(QString::fromStdString(line.substr(0, line.size() - 1)));
    return *outgoing.id;
}

void KataGoProcess::requestStartupProbe()
{
    sendCommand(GtpCommand{std::nullopt, "name", {}});
    sendCommand(GtpCommand{std::nullopt, "version", {}});
    sendCommand(GtpCommand{std::nullopt, "list_commands", {}});
}

void KataGoProcess::handleStarted()
{
    emit started();
    requestStartupProbe();
}

void KataGoProcess::handleStdout()
{
    stdoutBuffer_ += process_.readAllStandardOutput();
    consumeLines(&stdoutBuffer_, QProcess::StandardOutput);
}

void KataGoProcess::handleStderr()
{
    stderrBuffer_ += process_.readAllStandardError();
    consumeLines(&stderrBuffer_, QProcess::StandardError);
}

void KataGoProcess::handleError(QProcess::ProcessError error)
{
    if (stopping_) {
        return;
    }
    handleStderr();
    failureReported_ = true;
    emit startupFailed(diagnosticMessage(process_.errorString(), error));
}

void KataGoProcess::handleFinished(int exitCode, QProcess::ExitStatus status)
{
    const std::uint64_t serial = runSerial_;
    const bool wasStopping = stopping_;
    QTimer::singleShot(0, this, [this, serial, wasStopping, exitCode, status] {
        if (serial != runSerial_) {
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
        if (!wasStopping && !failureReported_ && (status != QProcess::NormalExit || exitCode != 0)) {
            failureReported_ = true;
            emit startupFailed(
                diagnosticMessage(QString("KataGo exited: code %1, status %2").arg(exitCode).arg(static_cast<int>(status))));
        }
        emit exited(exitCode, status);
        stopping_ = false;
    });
}

void KataGoProcess::consumeLines(QByteArray* buffer, QProcess::ProcessChannel channel)
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

void KataGoProcess::handleStdoutLine(QString line)
{
    if (line.endsWith('\r')) {
        line.chop(1);
    }
    emit protocolLineReceived(line);

    const std::optional<GtpEvent> event = parser_.feedLine(line.toStdString());
    if (!event.has_value()) {
        emitAnalyzeLine(line.toStdString());
        return;
    }
    if (event->type == GtpEvent::Type::Response && event->response.has_value()) {
        emit responseReceived(*event->response);
        emit commandResponseReceived(queue_.resolve(*event->response));
    } else if (event->type == GtpEvent::Type::RawLine) {
        emitAnalyzeLine(event->rawLine);
    }
}

void KataGoProcess::emitAnalyzeLine(std::string_view line)
{
    const KataAnalyzeLine update = parseKataAnalyzeLine(line);
    if (update.infos.empty() && !update.rootInfo.has_value() && update.ownership.empty()) {
        return;
    }
    emit kataAnalyzeLineReceived(update);
    for (const KataAnalyzeInfo& info : update.infos) {
        emit kataAnalyzeInfoReceived(info);
    }
}

void KataGoProcess::rememberStderrLine(const QString& line)
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

QString KataGoProcess::diagnosticMessage(const QString& summary, std::optional<QProcess::ProcessError> error) const
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

QString KataGoProcess::stderrSummary() const
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
