#include "ThreadedAnalysisProcess.h"
#include "ThreadedKataGoProcess.h"

#include <QCoreApplication>
#include <QTimer>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace lizzie::core;
using namespace lizzie::engine;

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

AnalysisRequest makeRequest(std::string id)
{
    AnalysisRequest request;
    request.id = std::move(id);
    request.boardSize = BoardSize::square(9);
    request.rules = "Chinese";
    request.komi = 7.5;
    request.moves = {
        Move::play(Color::Black, Point{2, 2}),
        Move::play(Color::White, Point{3, 3}),
    };
    return request;
}

void runWithTimeout(QCoreApplication* app, int timeoutMs)
{
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, app, &QCoreApplication::quit);
    timeout.start(timeoutMs);
    app->exec();
}

void testThreadedKataGoProcess(QCoreApplication* app, const char* fakeKataGoPath)
{
    ThreadedKataGoProcess process;
    require(process.hasWorkerThread(), "GTP process proxy should create a worker object on its worker thread");
    require(
        !process.workerThreadIsCurrentThread(),
        "GTP process worker thread should be separate from the calling UI/test thread");

    KataGoConfig config;
    config.executable = std::filesystem::path(fakeKataGoPath);
    config.model = "fake-model.bin.gz";
    config.gtpConfig = "fake-gtp.cfg";
    config.extraArgs = {"--require-gtp-argv"};

    bool failed = false;
    bool argvSeen = false;
    bool listCommandsSeen = false;
    bool analyzeSeen = false;
    bool reservedStartupIds = false;
    bool invalidCommandRejected = false;
    bool duplicateExplicitNameSeen = false;
    bool duplicateExplicitVersionSeen = false;

    QObject::connect(&process, &ThreadedKataGoProcess::startupFailed, [&](const QString& message) {
        failed = true;
        std::cerr << message.toStdString() << '\n';
        app->quit();
    });
    QObject::connect(&process, &ThreadedKataGoProcess::stderrLineReceived, [&](const QString& line) {
        if (line.contains("gtp fake argv ok")) {
            argvSeen = true;
        }
    });
    QObject::connect(&process, &ThreadedKataGoProcess::commandResponseReceived, [&](const QueuedGtpResponse& response) {
        if (!response.command.has_value()) {
            return;
        }
        if (response.command->name == "list_commands") {
            listCommandsSeen = true;
            invalidCommandRejected =
                process.sendCommand(GtpCommand{std::nullopt, "kata-stop\nclear_board", {}}) < 0;
            const int duplicateNameId = process.sendCommand(GtpCommand{4, "name", {}});
            const int duplicateVersionId = process.sendCommand(GtpCommand{4, "version", {}});
            const int analyzeId = process.sendCommand(GtpCommand{std::nullopt, "kata-analyze", {"10"}});
            reservedStartupIds = duplicateNameId == 4 && duplicateVersionId == 5 && analyzeId == 6;
        } else if (response.command->name == "name" && response.command->id == 4) {
            duplicateExplicitNameSeen = response.response.payload == "KataGo Fake";
        } else if (response.command->name == "version" && response.command->id == 5) {
            duplicateExplicitVersionSeen = response.response.payload == "1.15.0-fake";
        }
    });
    QObject::connect(&process, &ThreadedKataGoProcess::kataAnalyzeLineReceived, [&](const KataAnalyzeLine& line) {
        if (!line.infos.empty() && line.rootInfo.has_value()) {
            analyzeSeen = true;
            process.stop();
            app->quit();
        }
    });

    process.startGtp(config);
    runWithTimeout(app, 5000);
    process.stop();

    require(!failed, "threaded GTP process should not fail");
    require(argvSeen, "threaded GTP process should start KataGo with the configured argv");
    require(listCommandsSeen, "threaded GTP process should preserve startup probe responses");
    require(invalidCommandRejected, "threaded GTP proxy should reject invalid command names before reserving ids");
    require(reservedStartupIds, "threaded GTP proxy should reserve startup probe ids before UI commands");
    require(
        duplicateExplicitNameSeen && duplicateExplicitVersionSeen,
        "threaded GTP proxy should keep returned ids aligned with worker-queued duplicate explicit ids");
    require(analyzeSeen, "threaded GTP process should forward kata-analyze output");
}

void testThreadedKataGoBurstAnalysis(QCoreApplication* app, const char* fakeKataGoPath)
{
    ThreadedKataGoProcess process;
    require(process.hasWorkerThread(), "burst GTP process should use a worker thread");
    require(
        !process.workerThreadIsCurrentThread(),
        "burst GTP worker thread should be separate from the calling UI/test thread");

    KataGoConfig config;
    config.executable = std::filesystem::path(fakeKataGoPath);
    config.model = "fake-model.bin.gz";
    config.gtpConfig = "fake-gtp.cfg";
    config.extraArgs = {"--burst-analyze-lines"};

    bool failed = false;
    bool analyzeSent = false;
    int heartbeatTicks = 0;
    int analysisLines = 0;
    int analysisInfos = 0;
    QTimer heartbeat;
    heartbeat.setInterval(1);

    QObject::connect(&heartbeat, &QTimer::timeout, [&] {
        ++heartbeatTicks;
        if (analysisLines >= 120 && analysisInfos >= 120) {
            process.stop();
            app->quit();
        }
    });
    QObject::connect(&process, &ThreadedKataGoProcess::startupFailed, [&](const QString& message) {
        failed = true;
        std::cerr << message.toStdString() << '\n';
        app->quit();
    });
    QObject::connect(&process, &ThreadedKataGoProcess::commandResponseReceived, [&](const QueuedGtpResponse& response) {
        if (analyzeSent || !response.command.has_value() || response.command->name != "list_commands") {
            return;
        }
        analyzeSent = true;
        heartbeat.start();
        process.sendCommand(GtpCommand{std::nullopt, "kata-analyze", {"1"}});
    });
    QObject::connect(&process, &ThreadedKataGoProcess::kataAnalyzeLineReceived, [&](const KataAnalyzeLine& line) {
        if (!line.infos.empty() && line.rootInfo.has_value()) {
            ++analysisLines;
        }
    });
    QObject::connect(&process, &ThreadedKataGoProcess::kataAnalyzeInfoReceived, [&](const KataAnalyzeInfo&) {
        ++analysisInfos;
    });

    process.startGtp(config);
    runWithTimeout(app, 5000);
    heartbeat.stop();
    process.stop();

    require(!failed, "burst threaded GTP process should not fail");
    require(analyzeSent, "burst threaded GTP process should send kata-analyze after startup probe");
    require(analysisLines == 120, "burst threaded GTP process should forward every analysis line");
    require(analysisInfos == 120, "burst threaded GTP process should forward every analysis info block");
    require(heartbeatTicks > 0, "UI/test event loop should keep processing timer events during burst analysis output");
}

void testThreadedAnalysisProcess(QCoreApplication* app, const char* fakeAnalysisPath)
{
    ThreadedAnalysisProcess process;
    require(process.hasWorkerThread(), "analysis process proxy should create a worker object on its worker thread");
    require(
        !process.workerThreadIsCurrentThread(),
        "analysis process worker thread should be separate from the calling UI/test thread");

    KataGoConfig config;
    config.executable = std::filesystem::path(fakeAnalysisPath);
    config.model = "fake-model.bin.gz";
    config.analysisConfig = "fake-analysis.cfg";
    config.extraArgs = {"--require-analysis-argv"};

    bool failed = false;
    bool argvSeen = false;
    bool finished = false;
    int completed = 0;
    int total = 0;
    std::vector<AnalysisResponse> responses;

    QObject::connect(&process, &ThreadedAnalysisProcess::failed, [&](const QString& message) {
        failed = true;
        std::cerr << message.toStdString() << '\n';
        app->quit();
    });
    QObject::connect(&process, &ThreadedAnalysisProcess::stderrLineReceived, [&](const QString& line) {
        if (line.contains("analysis fake argv ok")) {
            argvSeen = true;
        }
    });
    QObject::connect(&process, &ThreadedAnalysisProcess::progressChanged, [&](int nextCompleted, int nextTotal) {
        completed = nextCompleted;
        total = nextTotal;
    });
    QObject::connect(&process, &ThreadedAnalysisProcess::responseReceived, [&](const AnalysisResponse& response) {
        responses.push_back(response);
    });
    QObject::connect(&process, &ThreadedAnalysisProcess::finished, [&](bool cancelled) {
        finished = !cancelled;
        app->quit();
    });

    process.startAnalysis(config, {makeRequest("node:threaded")});
    runWithTimeout(app, 5000);
    process.cancelAndWait();

    require(!failed, "threaded analysis process should not fail");
    require(argvSeen, "threaded analysis process should start KataGo analysis with the configured argv");
    require(finished, "threaded analysis process should finish normally");
    require(completed == 1 && total == 1, "threaded analysis process should report progress");
    require(responses.size() == 1, "threaded analysis process should forward one response");
    require(responses.front().rootVisits == 128, "threaded analysis process should parse root visits");
    require(responses.front().moveInfos.size() == 1, "threaded analysis process should parse candidate moves");
}

}  // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc < 3) {
        std::cerr << "engine_worker_tests requires fake GTP and fake analysis engine paths\n";
        return EXIT_FAILURE;
    }

    try {
        testThreadedKataGoProcess(&app, argv[1]);
        testThreadedKataGoBurstAnalysis(&app, argv[1]);
        testThreadedAnalysisProcess(&app, argv[2]);
    } catch (const std::exception& error) {
        std::cerr << "engine_worker_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "engine_worker_tests passed\n";
    return EXIT_SUCCESS;
}
