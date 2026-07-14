#include "ThreadedAnalysisProcess.h"

#include "AnalysisProcess.h"

#include <QSemaphore>
#include <QThread>

namespace lizzie::engine {

ThreadedAnalysisProcess::ThreadedAnalysisProcess(QObject* parent)
    : QObject(parent)
    , workerThread_(std::make_unique<QThread>())
{
    qRegisterMetaType<lizzie::engine::AnalysisResponse>();

    workerThread_->setObjectName(QStringLiteral("LizzieYzy KataGo analysis worker"));

    auto* bootstrap = new QObject;
    bootstrap->moveToThread(workerThread_.get());

    QSemaphore ready;
    connect(workerThread_.get(), &QThread::started, bootstrap, [this, bootstrap, &ready] {
        worker_ = new AnalysisProcess;

        connect(worker_, &AnalysisProcess::started, this, [this] {
            running_ = true;
            emit started();
        });
        connect(worker_, &AnalysisProcess::responseReceived, this, &ThreadedAnalysisProcess::responseReceived);
        connect(worker_, &AnalysisProcess::requestFailed, this, &ThreadedAnalysisProcess::requestFailed);
        connect(worker_, &AnalysisProcess::progressChanged, this, &ThreadedAnalysisProcess::progressChanged);
        connect(worker_, &AnalysisProcess::rawLineReceived, this, &ThreadedAnalysisProcess::rawLineReceived);
        connect(worker_, &AnalysisProcess::stderrLineReceived, this, &ThreadedAnalysisProcess::stderrLineReceived);
        connect(worker_, &AnalysisProcess::parseFailed, this, &ThreadedAnalysisProcess::parseFailed);
        connect(
            worker_,
            &AnalysisProcess::unknownResponseIgnored,
            this,
            &ThreadedAnalysisProcess::unknownResponseIgnored);
        connect(
            worker_,
            &AnalysisProcess::duplicateResponseIgnored,
            this,
            &ThreadedAnalysisProcess::duplicateResponseIgnored);
        connect(worker_, &AnalysisProcess::failed, this, [this](const QString& message) {
            running_ = false;
            emit failed(message);
        });
        connect(worker_, &AnalysisProcess::finished, this, [this](bool cancelled) {
            running_ = false;
            emit finished(cancelled);
        });
        connect(workerThread_.get(), &QThread::finished, worker_, &QObject::deleteLater);

        bootstrap->deleteLater();
        ready.release();
    });

    workerThread_->start();
    ready.acquire();
}

ThreadedAnalysisProcess::~ThreadedAnalysisProcess()
{
    if (workerThread_->isRunning()) {
        invokeCancelAndWait(Qt::BlockingQueuedConnection);
        workerThread_->quit();
        workerThread_->wait();
    }
    worker_ = nullptr;
}

bool ThreadedAnalysisProcess::isRunning() const
{
    return running_;
}

bool ThreadedAnalysisProcess::hasWorkerThread() const
{
    return worker_ != nullptr && worker_->thread() == workerThread_.get() && workerThread_->isRunning();
}

bool ThreadedAnalysisProcess::workerThreadIsCurrentThread() const
{
    return hasWorkerThread() && workerThread_.get() == QThread::currentThread();
}

void ThreadedAnalysisProcess::startAnalysis(const KataGoConfig& config, const std::vector<AnalysisRequest>& requests)
{
    running_ = true;
    QMetaObject::invokeMethod(
        worker_,
        [worker = worker_, config, requests] {
            worker->startAnalysis(config, requests);
        },
        Qt::QueuedConnection);
}

void ThreadedAnalysisProcess::cancel()
{
    if (worker_ == nullptr || !workerThread_->isRunning()) {
        return;
    }
    QMetaObject::invokeMethod(
        worker_,
        [worker = worker_] {
            worker->cancel();
        },
        Qt::QueuedConnection);
}

void ThreadedAnalysisProcess::cancelAndWait()
{
    invokeCancelAndWait(Qt::BlockingQueuedConnection);
    running_ = false;
}

void ThreadedAnalysisProcess::invokeCancelAndWait(Qt::ConnectionType connectionType)
{
    if (worker_ == nullptr || !workerThread_->isRunning()) {
        return;
    }
    if (QThread::currentThread() == worker_->thread()) {
        worker_->cancelAndWait();
        return;
    }
    QMetaObject::invokeMethod(
        worker_,
        [worker = worker_] {
            worker->cancelAndWait();
        },
        connectionType);
}

}  // namespace lizzie::engine
