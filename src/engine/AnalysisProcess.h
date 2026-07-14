#pragma once

#include "AnalysisJson.h"
#include "KataGoConfig.h"

#include <QMetaType>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace lizzie::engine {

class AnalysisProcess : public QObject {
    Q_OBJECT

public:
    explicit AnalysisProcess(QObject* parent = nullptr);

    [[nodiscard]] bool isRunning() const;

public slots:
    void startAnalysis(
        const lizzie::engine::KataGoConfig& config,
        const std::vector<lizzie::engine::AnalysisRequest>& requests);
    void cancel();
    void cancelAndWait();

signals:
    void started();
    void responseReceived(const lizzie::engine::AnalysisResponse& response);
    void requestFailed(const QString& id, const QString& message, const QString& rawLine);
    void progressChanged(int completed, int total);
    void rawLineReceived(const QString& line);
    void stderrLineReceived(const QString& line);
    void parseFailed(const QString& line);
    void unknownResponseIgnored(const QString& id, const QString& rawLine);
    void duplicateResponseIgnored(const QString& id, const QString& rawLine);
    void failed(const QString& message);
    void finished(bool cancelled);

private slots:
    void handleStarted();
    void handleStdout();
    void handleStderr();
    void handleError(QProcess::ProcessError error);
    void handleFinished(int exitCode, QProcess::ExitStatus status);

private:
    void consumeLines(QByteArray* buffer, QProcess::ProcessChannel channel);
    void handleStdoutLine(QString line);
    void writeRequests();
    void rememberStderrLine(const QString& line);
    void cancelAndWaitForRestart();
    void emitFailedOnce(const QString& summary, std::optional<QProcess::ProcessError> error = std::nullopt);
    void finishRun(bool cancelled);
    [[nodiscard]] QString diagnosticMessage(const QString& summary, std::optional<QProcess::ProcessError> error = std::nullopt) const;
    [[nodiscard]] QString stderrSummary() const;

    QProcess process_;
    QTimer cancelKillTimer_;
    std::vector<AnalysisRequest> requests_;
    std::unordered_map<std::string, lizzie::core::Color> toMoveById_;
    std::unordered_map<std::string, lizzie::core::BoardSize> boardSizeById_;
    std::unordered_set<std::string> completedIds_;
    QByteArray stdoutBuffer_;
    QByteArray stderrBuffer_;
    QStringList recentStderrLines_;
    std::uint64_t runSerial_ = 0;
    int completed_ = 0;
    bool cancelling_ = false;
    bool failedEmitted_ = false;
    bool finishedEmitted_ = false;
};

}  // namespace lizzie::engine

Q_DECLARE_METATYPE(lizzie::engine::AnalysisRequest)
Q_DECLARE_METATYPE(lizzie::engine::AnalysisResponse)
