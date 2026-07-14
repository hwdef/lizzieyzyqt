#include "ThreadedKataGoProcess.h"

#include "KataGoProcess.h"

#include <QSemaphore>
#include <QThread>

#include <limits>

namespace lizzie::engine {

namespace {

bool commandIdIsUsable(int id)
{
    return id > 0 && id < std::numeric_limits<int>::max();
}

int nextCandidateId(int id)
{
    if (id <= 0 || id >= std::numeric_limits<int>::max() - 1) {
        return 1;
    }
    return id + 1;
}

}  // namespace

ThreadedKataGoProcess::ThreadedKataGoProcess(QObject* parent)
    : QObject(parent)
    , workerThread_(std::make_unique<QThread>())
{
    qRegisterMetaType<lizzie::engine::GtpCommand>();
    qRegisterMetaType<lizzie::engine::GtpResponse>();
    qRegisterMetaType<lizzie::engine::QueuedGtpResponse>();
    qRegisterMetaType<lizzie::engine::KataAnalyzeLine>();
    qRegisterMetaType<lizzie::engine::KataAnalyzeInfo>();
    qRegisterMetaType<lizzie::engine::ProcessExitStatus>();

    workerThread_->setObjectName(QStringLiteral("LizzieYzy KataGo GTP worker"));

    auto* bootstrap = new QObject;
    bootstrap->moveToThread(workerThread_.get());

    QSemaphore ready;
    connect(workerThread_.get(), &QThread::started, bootstrap, [this, bootstrap, &ready] {
        worker_ = new KataGoProcess;

        connect(worker_, &KataGoProcess::started, this, [this] {
            running_ = true;
            emit started();
        });
        connect(worker_, &KataGoProcess::responseReceived, this, &ThreadedKataGoProcess::responseReceived);
        connect(
            worker_,
            &KataGoProcess::commandResponseReceived,
            this,
            &ThreadedKataGoProcess::commandResponseReceived);
        connect(worker_, &KataGoProcess::kataAnalyzeLineReceived, this, &ThreadedKataGoProcess::kataAnalyzeLineReceived);
        connect(worker_, &KataGoProcess::kataAnalyzeInfoReceived, this, &ThreadedKataGoProcess::kataAnalyzeInfoReceived);
        connect(worker_, &KataGoProcess::protocolLineReceived, this, &ThreadedKataGoProcess::protocolLineReceived);
        connect(worker_, &KataGoProcess::stderrLineReceived, this, &ThreadedKataGoProcess::stderrLineReceived);
        connect(worker_, &KataGoProcess::startupFailed, this, [this](const QString& message) {
            running_ = false;
            emit startupFailed(message);
        });
        connect(worker_, &KataGoProcess::exited, this, [this](int exitCode, QProcess::ExitStatus status) {
            if (suppressedExitCount_ > 0) {
                --suppressedExitCount_;
                return;
            }
            running_ = false;
            const ProcessExitStatus publicStatus = status == QProcess::NormalExit
                ? ProcessExitStatus::NormalExit
                : ProcessExitStatus::CrashExit;
            emit exited(exitCode, publicStatus);
        });
        connect(workerThread_.get(), &QThread::finished, worker_, &QObject::deleteLater);

        bootstrap->deleteLater();
        ready.release();
    });

    workerThread_->start();
    ready.acquire();
}

ThreadedKataGoProcess::~ThreadedKataGoProcess()
{
    if (workerThread_->isRunning()) {
        invokeWorkerStop(Qt::BlockingQueuedConnection);
        workerThread_->quit();
        workerThread_->wait();
    }
    worker_ = nullptr;
}

bool ThreadedKataGoProcess::isRunning() const
{
    return running_;
}

int ThreadedKataGoProcess::nextCommandId() const
{
    return nextCommandId_;
}

bool ThreadedKataGoProcess::hasWorkerThread() const
{
    return worker_ != nullptr && worker_->thread() == workerThread_.get() && workerThread_->isRunning();
}

bool ThreadedKataGoProcess::workerThreadIsCurrentThread() const
{
    return hasWorkerThread() && workerThread_.get() == QThread::currentThread();
}

void ThreadedKataGoProcess::startGtp(const KataGoConfig& config)
{
    running_ = false;
    nextCommandId_ = 4;
    QMetaObject::invokeMethod(
        worker_,
        [worker = worker_, config] {
            worker->startGtp(config);
        },
        Qt::QueuedConnection);
}

void ThreadedKataGoProcess::stop()
{
    if (running_ || workerProcessIsRunning()) {
        ++suppressedExitCount_;
    }
    invokeWorkerStop(Qt::BlockingQueuedConnection);
    running_ = false;
}

int ThreadedKataGoProcess::sendCommand(const GtpCommand& command)
{
    if (!running_) {
        emit startupFailed(QStringLiteral("KataGo process is not running"));
        return -1;
    }
    if (!isValidGtpCommandName(command.name)) {
        return -1;
    }

    GtpCommand outgoing = command;
    const int id = reserveCommandId();
    outgoing.id = id;
    QMetaObject::invokeMethod(
        worker_,
        [worker = worker_, outgoing] {
            if (worker->isRunning()) {
                worker->sendCommand(outgoing);
            }
        },
        Qt::QueuedConnection);
    return id;
}

void ThreadedKataGoProcess::requestStartupProbe()
{
    for (int index = 0; index < 3; ++index) {
        nextCommandId_ = nextCandidateId(nextCommandId_);
    }
    QMetaObject::invokeMethod(
        worker_,
        [worker = worker_] {
            worker->requestStartupProbe();
        },
        Qt::QueuedConnection);
}

void ThreadedKataGoProcess::invokeWorkerStop(Qt::ConnectionType connectionType)
{
    if (worker_ == nullptr || !workerThread_->isRunning()) {
        return;
    }
    if (QThread::currentThread() == worker_->thread()) {
        worker_->stop();
        return;
    }
    QMetaObject::invokeMethod(
        worker_,
        [worker = worker_] {
            worker->stop();
        },
        connectionType);
}

bool ThreadedKataGoProcess::workerProcessIsRunning()
{
    if (worker_ == nullptr || !workerThread_->isRunning()) {
        return false;
    }
    if (QThread::currentThread() == worker_->thread()) {
        return worker_->isRunning();
    }

    bool active = false;
    QMetaObject::invokeMethod(
        worker_,
        [worker = worker_, &active] {
            active = worker->isRunning();
        },
        Qt::BlockingQueuedConnection);
    return active;
}

int ThreadedKataGoProcess::reserveCommandId()
{
    const int result = commandIdIsUsable(nextCommandId_) ? nextCommandId_ : 1;
    nextCommandId_ = nextCandidateId(result);
    return result;
}

}  // namespace lizzie::engine
