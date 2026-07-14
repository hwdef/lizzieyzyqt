#pragma once

#include "GtpCommandQueue.h"
#include "GtpProtocol.h"
#include "KataGoConfig.h"

#include <QMetaType>
#include <QObject>
#include <QString>

#include <memory>
#include <optional>

class QThread;

namespace lizzie::engine {

class KataGoProcess;

enum class ProcessExitStatus {
    NormalExit = 0,
    CrashExit = 1,
};

class ThreadedKataGoProcess : public QObject {
    Q_OBJECT

public:
    explicit ThreadedKataGoProcess(QObject* parent = nullptr);
    ~ThreadedKataGoProcess() override;

    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] int nextCommandId() const;
    [[nodiscard]] bool hasWorkerThread() const;
    [[nodiscard]] bool workerThreadIsCurrentThread() const;

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
    void exited(int exitCode, lizzie::engine::ProcessExitStatus status);

private:
    void invokeWorkerStop(Qt::ConnectionType connectionType);
    [[nodiscard]] bool workerProcessIsRunning();
    [[nodiscard]] int reserveCommandId();

    std::unique_ptr<QThread> workerThread_;
    lizzie::engine::KataGoProcess* worker_ = nullptr;
    int nextCommandId_ = 4;
    int suppressedExitCount_ = 0;
    bool running_ = false;
};

}  // namespace lizzie::engine

Q_DECLARE_METATYPE(lizzie::engine::ProcessExitStatus)
