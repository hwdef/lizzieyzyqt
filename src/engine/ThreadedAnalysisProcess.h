#pragma once

#include "AnalysisJson.h"
#include "KataGoConfig.h"

#include <QObject>

#include <memory>
#include <vector>

class QThread;

namespace lizzie::engine {

class AnalysisProcess;

class ThreadedAnalysisProcess : public QObject {
    Q_OBJECT

public:
    explicit ThreadedAnalysisProcess(QObject* parent = nullptr);
    ~ThreadedAnalysisProcess() override;

    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] bool hasWorkerThread() const;
    [[nodiscard]] bool workerThreadIsCurrentThread() const;

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

private:
    void invokeCancelAndWait(Qt::ConnectionType connectionType);

    std::unique_ptr<QThread> workerThread_;
    lizzie::engine::AnalysisProcess* worker_ = nullptr;
    bool running_ = false;
};

}  // namespace lizzie::engine
