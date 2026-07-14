#pragma once

#include "GtpProtocol.h"
#include "GtpCommandQueue.h"
#include "KataGoConfig.h"

#include <QMetaType>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <optional>
#include <string_view>

namespace lizzie::engine {

class KataGoProcess : public QObject {
    Q_OBJECT

public:
    explicit KataGoProcess(QObject* parent = nullptr);

    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] int nextCommandId() const;

public slots:
    void startGtp(const lizzie::engine::KataGoConfig& config);
    void stop();
    int sendCommand(const lizzie::engine::GtpCommand& command);
    void requestStartupProbe();

signals:
    void started();
    void responseReceived(const lizzie::engine::GtpResponse& response);
    void commandResponseReceived(const lizzie::engine::QueuedGtpResponse& response);
    void kataAnalyzeLineReceived(const lizzie::engine::KataAnalyzeLine& line);
    void kataAnalyzeInfoReceived(const lizzie::engine::KataAnalyzeInfo& info);
    void protocolLineReceived(const QString& line);
    void stderrLineReceived(const QString& line);
    void startupFailed(const QString& message);
    void exited(int exitCode, QProcess::ExitStatus status);

private slots:
    void handleStarted();
    void handleStdout();
    void handleStderr();
    void handleError(QProcess::ProcessError error);
    void handleFinished(int exitCode, QProcess::ExitStatus status);

private:
    void consumeLines(QByteArray* buffer, QProcess::ProcessChannel channel);
    void handleStdoutLine(QString line);
    void emitAnalyzeLine(std::string_view line);
    void rememberStderrLine(const QString& line);
    [[nodiscard]] QString diagnosticMessage(const QString& summary, std::optional<QProcess::ProcessError> error = std::nullopt) const;
    [[nodiscard]] QString stderrSummary() const;

    QProcess process_;
    GtpStreamParser parser_;
    GtpCommandQueue queue_;
    QByteArray stdoutBuffer_;
    QByteArray stderrBuffer_;
    QStringList recentStderrLines_;
    std::uint64_t runSerial_ = 0;
    bool stopping_ = false;
    bool failureReported_ = false;
};

}  // namespace lizzie::engine

Q_DECLARE_METATYPE(lizzie::engine::GtpCommand)
Q_DECLARE_METATYPE(lizzie::engine::KataAnalyzeLine)
Q_DECLARE_METATYPE(lizzie::engine::KataAnalyzeInfo)
Q_DECLARE_METATYPE(lizzie::engine::GtpResponse)
Q_DECLARE_METATYPE(lizzie::engine::QueuedGtpResponse)
