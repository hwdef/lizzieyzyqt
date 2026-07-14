#include "AnalysisProcess.h"

#include <QCoreApplication>
#include <QTimer>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
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

AnalysisRequest makeRequest(const std::string& id)
{
    AnalysisRequest request;
    request.id = id;
    request.boardSize = BoardSize::square(19);
    request.rules = "Chinese";
    request.komi = 7.5;
    request.initialPlayer = Color::Black;
    request.moves.push_back(Move::play(Color::Black, Point{3, 15}));
    return request;
}

AnalysisRequest makeWhiteRootRequest(const std::string& id)
{
    AnalysisRequest request;
    request.id = id;
    request.boardSize = BoardSize::square(19);
    request.rules = "Chinese";
    request.komi = 7.5;
    request.initialPlayer = Color::White;
    return request;
}

AnalysisRequest makeHandicapRootRequest(const std::string& id)
{
    AnalysisRequest request;
    request.id = id;
    request.boardSize = BoardSize::square(9);
    request.rules = "Chinese";
    request.komi = 7.5;
    request.initialPlayer = Color::White;
    request.initialStones = {
        InitialStone{Color::Black, Point{3, 3}},
        InitialStone{Color::Black, Point{5, 3}},
    };
    return request;
}

}  // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc < 2) {
        std::cerr << "analysis_process_fixture_tests requires fake analysis engine path\n";
        return EXIT_FAILURE;
    }

    try {
        AnalysisProcess missingConfigProcess;
        QString missingConfigDiagnostic;
        QObject::connect(&missingConfigProcess, &AnalysisProcess::failed, [&](const QString& message) {
            missingConfigDiagnostic = message;
        });
        bool missingConfigFinished = false;
        bool missingConfigCancelled = true;
        QObject::connect(&missingConfigProcess, &AnalysisProcess::finished, [&](bool cancelled) {
            missingConfigFinished = true;
            missingConfigCancelled = cancelled;
        });
        KataGoConfig missingConfig;
        missingConfig.executable = std::filesystem::path(argv[1]);
        missingConfig.model = "fake-model.bin.gz";
        missingConfig.workingDirectory = "/tmp";
        missingConfigProcess.startAnalysis(missingConfig, {makeRequest("missing-config")});
        require(missingConfigDiagnostic.contains("Command:"), "analysis diagnostics should include command");
        require(
            missingConfigDiagnostic.contains("Working directory: /tmp"),
            "analysis diagnostics should include working directory");
        require(missingConfigDiagnostic.contains("Stderr: (none)"), "analysis diagnostics should include stderr summary");
        require(missingConfigFinished, "missing analysis config should still emit finished");
        require(!missingConfigCancelled, "missing analysis config should not be reported as cancelled");

        AnalysisProcess failedStartProcess;
        KataGoConfig failedStartConfig;
        failedStartConfig.executable =
            std::filesystem::temp_directory_path() / "lizzie-missing-analysis-engine";
        failedStartConfig.model = "fake-model.bin.gz";
        failedStartConfig.analysisConfig = "fake-analysis.cfg";
        QString failedStartDiagnostic;
        bool failedStartFinished = false;
        bool failedStartCancelled = true;
        QObject::connect(&failedStartProcess, &AnalysisProcess::failed, [&](const QString& message) {
            failedStartDiagnostic = message;
        });
        QObject::connect(&failedStartProcess, &AnalysisProcess::finished, [&](bool cancelled) {
            failedStartFinished = true;
            failedStartCancelled = cancelled;
            app.quit();
        });

        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        failedStartProcess.startAnalysis(failedStartConfig, {makeRequest("failed-start")});
        app.exec();

        require(failedStartFinished, "failed-to-start analysis process should emit finished");
        require(!failedStartCancelled, "failed-to-start analysis process should not be reported as cancelled");
        require(failedStartDiagnostic.contains("Command:"), "failed-to-start diagnostics should include command");
        require(
            failedStartDiagnostic.contains("Process error:"),
            "failed-to-start diagnostics should include process error code");

        AnalysisProcess libraryPathProcess;
        KataGoConfig libraryPathConfig;
        libraryPathConfig.executable = std::filesystem::path(argv[1]);
        libraryPathConfig.model = "fake-model.bin.gz";
        libraryPathConfig.analysisConfig = "fake-analysis.cfg";
        libraryPathConfig.libraryPaths = {
            std::filesystem::temp_directory_path() / "lizzie-analysis-cuda-lib-fixture",
            std::filesystem::temp_directory_path() / "lizzie-analysis-cudnn-lib-fixture",
        };
        libraryPathConfig.extraArgs = {"--print-library-path-env"};
        QString observedLibraryPath;
        bool libraryPathFailed = false;
        bool libraryPathFinished = false;
        QObject::connect(&libraryPathProcess, &AnalysisProcess::failed, [&](const QString& message) {
            libraryPathFailed = true;
            std::cerr << message.toStdString() << '\n';
            app.quit();
        });
        QObject::connect(&libraryPathProcess, &AnalysisProcess::stderrLineReceived, [&](const QString& line) {
            constexpr std::string_view prefix = "analysis fake library path ";
            const std::string text = line.toStdString();
            if (text.starts_with(prefix)) {
                observedLibraryPath = QString::fromStdString(text.substr(prefix.size()));
            }
        });
        QObject::connect(&libraryPathProcess, &AnalysisProcess::finished, [&](bool cancelled) {
            libraryPathFinished = !cancelled;
            app.quit();
        });

        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        libraryPathProcess.startAnalysis(libraryPathConfig, {makeRequest("library-path")});
        app.exec();
        libraryPathProcess.cancel();

        require(!libraryPathFailed, "analysis library path process should not fail");
        require(libraryPathFinished, "analysis library path process should finish");
        require(
            observedLibraryPath.contains("lizzie-analysis-cuda-lib-fixture"),
            "analysis process should pass configured CUDA library paths to the child environment");
        require(
            observedLibraryPath.contains("lizzie-analysis-cudnn-lib-fixture"),
            "analysis process should pass configured cuDNN library paths to the child environment");

        AnalysisProcess process;
        KataGoConfig config;
        config.executable = std::filesystem::path(argv[1]);
        config.model = "fake-model.bin.gz";
        config.analysisConfig = "fake-analysis.cfg";
        config.extraArgs = {"--require-analysis-argv"};

        AnalysisProcess emptyRequestIdProcess;
        QString emptyRequestIdDiagnostic;
        bool emptyRequestIdStarted = false;
        bool emptyRequestIdFinished = false;
        bool emptyRequestIdCancelled = true;
        QObject::connect(&emptyRequestIdProcess, &AnalysisProcess::started, [&]() {
            emptyRequestIdStarted = true;
        });
        QObject::connect(&emptyRequestIdProcess, &AnalysisProcess::failed, [&](const QString& message) {
            emptyRequestIdDiagnostic = message;
        });
        QObject::connect(&emptyRequestIdProcess, &AnalysisProcess::finished, [&](bool cancelled) {
            emptyRequestIdFinished = true;
            emptyRequestIdCancelled = cancelled;
        });
        emptyRequestIdProcess.startAnalysis(config, {makeRequest("")});
        require(!emptyRequestIdStarted, "empty request id should be rejected before starting KataGo analysis");
        require(emptyRequestIdFinished, "empty request id should emit finished");
        require(!emptyRequestIdCancelled, "empty request id should not be reported as cancelled");
        require(
            emptyRequestIdDiagnostic.contains("analysis request id is required"),
            "empty request id diagnostic should explain the input error");

        AnalysisProcess duplicateRequestIdProcess;
        QString duplicateRequestIdDiagnostic;
        bool duplicateRequestIdStarted = false;
        bool duplicateRequestIdFinished = false;
        bool duplicateRequestIdCancelled = true;
        QObject::connect(&duplicateRequestIdProcess, &AnalysisProcess::started, [&]() {
            duplicateRequestIdStarted = true;
        });
        QObject::connect(&duplicateRequestIdProcess, &AnalysisProcess::failed, [&](const QString& message) {
            duplicateRequestIdDiagnostic = message;
        });
        QObject::connect(&duplicateRequestIdProcess, &AnalysisProcess::finished, [&](bool cancelled) {
            duplicateRequestIdFinished = true;
            duplicateRequestIdCancelled = cancelled;
        });
        duplicateRequestIdProcess.startAnalysis(
            config,
            {makeRequest("node:duplicate-id"), makeRequest("node:duplicate-id")});
        require(!duplicateRequestIdStarted, "duplicate request id should be rejected before starting KataGo analysis");
        require(duplicateRequestIdFinished, "duplicate request id should emit finished");
        require(!duplicateRequestIdCancelled, "duplicate request id should not be reported as cancelled");
        require(
            duplicateRequestIdDiagnostic.contains("duplicate analysis request id: node:duplicate-id"),
            "duplicate request id diagnostic should name the duplicated id");

        bool failed = false;
        bool stderrSeen = false;
        bool argvSeen = false;
        bool finished = false;
        int lastCompleted = 0;
        int lastTotal = 0;
        std::vector<AnalysisResponse> responses;
        std::vector<AnalysisRequest> requests = {
            makeRequest("node:1"),
            makeRequest("node:2"),
            makeWhiteRootRequest("node:white-root"),
        };

        QObject::connect(&process, &AnalysisProcess::failed, [&](const QString& message) {
            failed = true;
            std::cerr << message.toStdString() << '\n';
            app.quit();
        });
        QObject::connect(&process, &AnalysisProcess::stderrLineReceived, [&](const QString& line) {
            if (line.contains("analysis fake ready")) {
                stderrSeen = true;
            }
            if (line.contains("analysis fake argv ok")) {
                argvSeen = true;
            }
        });
        QObject::connect(&process, &AnalysisProcess::progressChanged, [&](int completed, int total) {
            lastCompleted = completed;
            lastTotal = total;
        });
        QObject::connect(&process, &AnalysisProcess::responseReceived, [&](const AnalysisResponse& response) {
            responses.push_back(response);
        });
        QObject::connect(&process, &AnalysisProcess::finished, [&](bool cancelled) {
            finished = !cancelled;
            app.quit();
        });

        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        process.startAnalysis(config, requests);
        app.exec();
        process.cancel();

        require(!failed, "analysis process should not fail");
        require(stderrSeen, "analysis stderr diagnostics should be preserved");
        require(argvSeen, "analysis process should start KataGo in analysis mode with model, config, and quit flag");
        require(finished, "analysis process should finish normally");
        require(lastCompleted == 3 && lastTotal == 3, "analysis progress should reach total");
        require(responses.size() == 3, "analysis process should emit three responses");
        require(responses.front().rootVisits == 128, "analysis response should parse rootInfo");
        require(responses.front().moveInfos.size() == 1, "analysis response should parse moveInfos");
        require(responses.front().moveInfos.front().move.type == MoveType::Play, "analysis move should parse");
        require(responses.front().ownership.size() == 19 * 19, "analysis response should preserve 19x19 ownership");
        require(
            responses.front().moveInfos.front().ownership.size() == 19 * 19,
            "analysis response should preserve candidate ownership");
        require(
            responses.back().id == "node:white-root" && responses.back().moveInfos.front().move.color == Color::White,
            "analysis process should use initialPlayer for root responses without moves");

        AnalysisProcess duplicateProcess;
        KataGoConfig duplicateConfig = config;
        duplicateConfig.extraArgs = {"--duplicate-response"};
        bool duplicateFailed = false;
        bool duplicateFinished = false;
        bool duplicateCancelled = true;
        QString duplicateIgnoredId;
        QString duplicateIgnoredLine;
        int duplicateCompleted = 0;
        int duplicateTotal = 0;
        std::vector<AnalysisResponse> duplicateResponses;
        QObject::connect(&duplicateProcess, &AnalysisProcess::failed, [&](const QString& message) {
            duplicateFailed = true;
            std::cerr << message.toStdString() << '\n';
        });
        QObject::connect(
            &duplicateProcess,
            &AnalysisProcess::duplicateResponseIgnored,
            [&](const QString& id, const QString& line) {
                duplicateIgnoredId = id;
                duplicateIgnoredLine = line;
            });
        QObject::connect(&duplicateProcess, &AnalysisProcess::responseReceived, [&](const AnalysisResponse& response) {
            duplicateResponses.push_back(response);
        });
        QObject::connect(&duplicateProcess, &AnalysisProcess::progressChanged, [&](int completed, int total) {
            duplicateCompleted = completed;
            duplicateTotal = total;
        });
        QObject::connect(&duplicateProcess, &AnalysisProcess::finished, [&](bool cancelled) {
            duplicateFinished = true;
            duplicateCancelled = cancelled;
            app.quit();
        });

        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        duplicateProcess.startAnalysis(duplicateConfig, {makeRequest("node:duplicate")});
        app.exec();

        require(duplicateFinished, "duplicate analysis response run should finish");
        require(!duplicateCancelled, "duplicate analysis response run should not be reported as cancelled");
        require(!duplicateFailed, "duplicate analysis response should not fail the whole process");
        require(duplicateResponses.size() == 1, "duplicate analysis response should only emit one applied response");
        require(duplicateCompleted == 1 && duplicateTotal == 1, "duplicate response should not advance progress twice");
        require(duplicateIgnoredId == "node:duplicate", "duplicate response should report the ignored request id");
        require(
            duplicateIgnoredLine.contains("node:duplicate"),
            "duplicate response should be preserved as a duplicate-response diagnostic line");

        AnalysisProcess unknownProcess;
        KataGoConfig unknownConfig = config;
        unknownConfig.extraArgs = {"--unknown-response"};
        bool unknownFailed = false;
        bool unknownParseFailed = false;
        bool unknownFinished = false;
        bool unknownCancelled = true;
        QString unknownIgnoredId;
        QString unknownIgnoredLine;
        int unknownCompleted = 0;
        int unknownTotal = 0;
        std::vector<AnalysisResponse> unknownResponses;
        QObject::connect(&unknownProcess, &AnalysisProcess::failed, [&](const QString& message) {
            unknownFailed = true;
            std::cerr << message.toStdString() << '\n';
        });
        QObject::connect(&unknownProcess, &AnalysisProcess::parseFailed, [&](const QString&) {
            unknownParseFailed = true;
        });
        QObject::connect(
            &unknownProcess,
            &AnalysisProcess::unknownResponseIgnored,
            [&](const QString& id, const QString& line) {
                unknownIgnoredId = id;
                unknownIgnoredLine = line;
            });
        QObject::connect(&unknownProcess, &AnalysisProcess::responseReceived, [&](const AnalysisResponse& response) {
            unknownResponses.push_back(response);
        });
        QObject::connect(&unknownProcess, &AnalysisProcess::progressChanged, [&](int completed, int total) {
            unknownCompleted = completed;
            unknownTotal = total;
        });
        QObject::connect(&unknownProcess, &AnalysisProcess::finished, [&](bool cancelled) {
            unknownFinished = true;
            unknownCancelled = cancelled;
            app.quit();
        });

        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        unknownProcess.startAnalysis(unknownConfig, {makeRequest("node:known")});
        app.exec();

        require(unknownFinished, "unknown analysis response run should finish");
        require(!unknownCancelled, "unknown analysis response run should not be reported as cancelled");
        require(!unknownFailed, "unknown analysis response should not fail the whole process");
        require(!unknownParseFailed, "unknown analysis response should not be reported as malformed JSON");
        require(unknownResponses.size() == 1, "unknown analysis response should not emit an applied response");
        require(unknownResponses.front().id == "node:known", "known response should still be applied after an unknown id");
        require(unknownCompleted == 1 && unknownTotal == 1, "unknown response should not advance progress");
        require(unknownIgnoredId == "node:unknown", "unknown response should report the ignored request id");
        require(
            unknownIgnoredLine.contains("node:unknown"),
            "unknown response should be preserved as an unknown-response diagnostic line");

        AnalysisProcess handicapProcess;
        KataGoConfig handicapConfig = config;
        handicapConfig.extraArgs = {"--require-handicap-request"};
        bool handicapFailed = false;
        bool handicapValidated = false;
        bool handicapFinished = false;
        std::vector<AnalysisResponse> handicapResponses;
        QObject::connect(&handicapProcess, &AnalysisProcess::failed, [&](const QString& message) {
            handicapFailed = true;
            std::cerr << message.toStdString() << '\n';
            app.quit();
        });
        QObject::connect(&handicapProcess, &AnalysisProcess::stderrLineReceived, [&](const QString& line) {
            if (line.contains("analysis fake handicap request ok")) {
                handicapValidated = true;
            }
        });
        QObject::connect(&handicapProcess, &AnalysisProcess::responseReceived, [&](const AnalysisResponse& response) {
            handicapResponses.push_back(response);
        });
        QObject::connect(&handicapProcess, &AnalysisProcess::finished, [&](bool cancelled) {
            handicapFinished = !cancelled;
            app.quit();
        });

        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        handicapProcess.startAnalysis(handicapConfig, {makeHandicapRootRequest("node:handicap-root")});
        app.exec();
        handicapProcess.cancel();

        require(!handicapFailed, "handicap analysis request should not fail fake validation");
        require(handicapValidated, "analysis process should send initialStones to KataGo analysis stdin");
        require(handicapFinished, "handicap analysis request should finish normally");
        require(handicapResponses.size() == 1, "handicap analysis request should emit one response");
        require(handicapResponses.front().id == "node:handicap-root", "handicap response should preserve request id");
        require(
            handicapResponses.front().moveInfos.front().move.color == Color::White,
            "handicap root response should use White as the first candidate color");

        AnalysisProcess workingDirectoryProcess;
        KataGoConfig workingDirectoryConfig = config;
        const std::filesystem::path inheritedWorkingDirectory = std::filesystem::current_path();
        const std::filesystem::path explicitWorkingDirectory =
            std::filesystem::temp_directory_path() / "lizzie-analysis-cwd-fixture";
        std::filesystem::create_directories(explicitWorkingDirectory);
        std::vector<std::filesystem::path> observedWorkingDirectories;
        bool workingDirectoryFailed = false;
        bool workingDirectoryFinished = false;
        QObject::connect(&workingDirectoryProcess, &AnalysisProcess::failed, [&](const QString& message) {
            workingDirectoryFailed = true;
            std::cerr << message.toStdString() << '\n';
        });
        QObject::connect(&workingDirectoryProcess, &AnalysisProcess::stderrLineReceived, [&](const QString& line) {
            constexpr std::string_view prefix = "analysis fake cwd ";
            const std::string text = line.toStdString();
            if (text.starts_with(prefix)) {
                observedWorkingDirectories.push_back(text.substr(prefix.size()));
            }
        });
        QObject::connect(&workingDirectoryProcess, &AnalysisProcess::finished, [&](bool) {
            workingDirectoryFinished = true;
            app.quit();
        });

        workingDirectoryConfig.workingDirectory = explicitWorkingDirectory;
        workingDirectoryConfig.extraArgs = {"--print-cwd"};
        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        workingDirectoryProcess.startAnalysis(workingDirectoryConfig, {makeRequest("node:cwd-explicit")});
        app.exec();
        require(workingDirectoryFinished, "analysis process with explicit working directory should finish");
        require(!workingDirectoryFailed, "analysis process with explicit working directory should not fail");
        require(observedWorkingDirectories.size() == 1, "explicit working directory run should report cwd");
        require(
            std::filesystem::equivalent(observedWorkingDirectories.front(), explicitWorkingDirectory),
            "analysis process should use configured working directory");

        workingDirectoryFinished = false;
        workingDirectoryFailed = false;
        observedWorkingDirectories.clear();
        workingDirectoryConfig.workingDirectory.clear();
        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        workingDirectoryProcess.startAnalysis(workingDirectoryConfig, {makeRequest("node:cwd-inherited")});
        app.exec();
        require(workingDirectoryFinished, "analysis process with inherited working directory should finish");
        require(!workingDirectoryFailed, "analysis process with inherited working directory should not fail");
        require(observedWorkingDirectories.size() == 1, "inherited working directory run should report cwd");
        require(
            std::filesystem::equivalent(observedWorkingDirectories.front(), inheritedWorkingDirectory),
            "analysis process should clear stale working directory when config is empty");

        AnalysisProcess failingProcess;
        KataGoConfig failingConfig = config;
        failingConfig.extraArgs = {"--fail-after-start"};
        QString failureDiagnostic;
        bool failureFinished = false;
        QObject::connect(&failingProcess, &AnalysisProcess::failed, [&](const QString& message) {
            failureDiagnostic = message;
        });
        QObject::connect(&failingProcess, &AnalysisProcess::finished, [&](bool) {
            failureFinished = true;
            app.quit();
        });

        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        failingProcess.startAnalysis(failingConfig, {makeRequest("node:failed")});
        app.exec();

        require(failureFinished, "failing analysis process should still finish");
        require(failureDiagnostic.contains("KataGo analysis exited: code 9"), "nonzero analysis exit should fail");
        require(failureDiagnostic.contains("Command:"), "nonzero exit diagnostics should include command");
        require(failureDiagnostic.contains("--fail-after-start"), "nonzero exit diagnostics should include extra args");
        require(failureDiagnostic.contains("analysis fake fatal"), "nonzero exit diagnostics should include stderr");

        AnalysisProcess crashingProcess;
        KataGoConfig crashingConfig = config;
        crashingConfig.extraArgs = {"--crash-after-start"};
        int crashFailureCount = 0;
        QString crashDiagnostic;
        bool crashFinished = false;
        QObject::connect(&crashingProcess, &AnalysisProcess::failed, [&](const QString& message) {
            ++crashFailureCount;
            crashDiagnostic = message;
        });
        QObject::connect(&crashingProcess, &AnalysisProcess::finished, [&](bool) {
            crashFinished = true;
            app.quit();
        });

        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        crashingProcess.startAnalysis(crashingConfig, {makeRequest("node:crashed")});
        app.exec();

        require(crashFinished, "crashing analysis process should still finish");
        require(crashFailureCount == 1, "crashing analysis process should emit exactly one process failure");
        require(crashDiagnostic.contains("Process error:"), "crash diagnostics should include process error");
        require(crashDiagnostic.contains("--crash-after-start"), "crash diagnostics should include command line");
        require(crashDiagnostic.contains("analysis fake crash"), "crash diagnostics should include stderr");

        AnalysisProcess malformedProcess;
        KataGoConfig malformedConfig = config;
        malformedConfig.extraArgs = {"--malformed-after-start"};
        QString malformedDiagnostic;
        QString malformedLine;
        bool malformedFinished = false;
        QObject::connect(&malformedProcess, &AnalysisProcess::failed, [&](const QString& message) {
            malformedDiagnostic = message;
        });
        QObject::connect(&malformedProcess, &AnalysisProcess::parseFailed, [&](const QString& line) {
            malformedLine = line;
        });
        QObject::connect(&malformedProcess, &AnalysisProcess::finished, [&](bool) {
            malformedFinished = true;
            app.quit();
        });

        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        malformedProcess.startAnalysis(malformedConfig, {makeRequest("node:malformed")});
        app.exec();

        require(malformedFinished, "malformed analysis process should still finish");
        require(malformedLine == "not-json", "malformed analysis output should be reported");
        require(
            malformedDiagnostic.contains("KataGo analysis ended before all responses: 0/1 completed"),
            "incomplete analysis responses should fail");
        require(
            malformedDiagnostic.contains("--malformed-after-start"),
            "incomplete analysis diagnostics should include command line");

        AnalysisProcess errorProcess;
        KataGoConfig errorConfig = config;
        errorConfig.extraArgs = {"--error-after-start"};
        bool errorFailed = false;
        bool errorFinished = false;
        bool errorRequestFailed = false;
        bool errorResponseReceived = false;
        int errorCompleted = 0;
        int errorTotal = 0;
        QString errorId;
        QString errorMessage;
        QObject::connect(&errorProcess, &AnalysisProcess::failed, [&](const QString& message) {
            errorFailed = true;
            std::cerr << message.toStdString() << '\n';
        });
        QObject::connect(&errorProcess, &AnalysisProcess::responseReceived, [&](const AnalysisResponse&) {
            errorResponseReceived = true;
        });
        QObject::connect(&errorProcess, &AnalysisProcess::requestFailed, [&](const QString& id, const QString& message, const QString&) {
            errorRequestFailed = true;
            errorId = id;
            errorMessage = message;
        });
        QObject::connect(&errorProcess, &AnalysisProcess::progressChanged, [&](int completed, int total) {
            errorCompleted = completed;
            errorTotal = total;
        });
        QObject::connect(&errorProcess, &AnalysisProcess::finished, [&](bool cancelled) {
            errorFinished = !cancelled;
            app.quit();
        });

        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        errorProcess.startAnalysis(errorConfig, {makeRequest("node:error")});
        app.exec();

        require(errorFinished, "analysis error response process should finish normally");
        require(!errorFailed, "analysis error response should not fail the whole process");
        require(errorRequestFailed, "analysis error response should emit requestFailed");
        require(errorId == "node:error", "analysis error response should preserve request id");
        require(errorMessage.contains("analysis fake node error"), "analysis error response should preserve message");
        require(!errorResponseReceived, "analysis error response should not emit responseReceived");
        require(errorCompleted == 1 && errorTotal == 1, "analysis error response should count toward progress");

        AnalysisProcess partialExitProcess;
        KataGoConfig partialExitConfig = config;
        partialExitConfig.extraArgs = {"--exit-after-response"};
        QString partialExitDiagnostic;
        bool partialExitFinished = false;
        bool partialExitCancelled = true;
        int partialCompleted = 0;
        int partialTotal = 0;
        std::vector<AnalysisResponse> partialResponses;
        QObject::connect(&partialExitProcess, &AnalysisProcess::failed, [&](const QString& message) {
            partialExitDiagnostic = message;
        });
        QObject::connect(&partialExitProcess, &AnalysisProcess::responseReceived, [&](const AnalysisResponse& response) {
            partialResponses.push_back(response);
        });
        QObject::connect(&partialExitProcess, &AnalysisProcess::progressChanged, [&](int completed, int total) {
            partialCompleted = completed;
            partialTotal = total;
        });
        QObject::connect(&partialExitProcess, &AnalysisProcess::finished, [&](bool cancelled) {
            partialExitFinished = true;
            partialExitCancelled = cancelled;
            app.quit();
        });

        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        partialExitProcess.startAnalysis(
            partialExitConfig,
            {makeRequest("node:partial-1"), makeRequest("node:partial-2")});
        app.exec();

        require(partialExitFinished, "partial analysis exit should emit finished");
        require(!partialExitCancelled, "partial analysis exit should not be reported as cancelled");
        require(partialResponses.size() == 1, "partial analysis exit should preserve completed responses");
        require(partialCompleted == 1 && partialTotal == 2, "partial analysis exit should report partial progress");
        require(
            partialExitDiagnostic.contains("KataGo analysis ended before all responses: 1/2 completed"),
            "partial analysis exit should diagnose missing responses");
        require(
            partialExitDiagnostic.contains("--exit-after-response"),
            "partial analysis exit diagnostics should include command line");

        AnalysisProcess cancellingProcess;
        KataGoConfig cancellingConfig = config;
        cancellingConfig.extraArgs = {"--slow-after-start"};
        bool cancelFailed = false;
        bool cancelFinished = false;
        bool cancelFlag = false;
        bool cancelStarted = false;
        bool cancelSawSlowRequest = false;
        int cancelCompleted = -1;
        int cancelTotal = -1;
        int cancelResponses = 0;
        QObject::connect(&cancellingProcess, &AnalysisProcess::failed, [&](const QString& message) {
            cancelFailed = true;
            std::cerr << message.toStdString() << '\n';
        });
        QObject::connect(&cancellingProcess, &AnalysisProcess::started, [&] {
            cancelStarted = true;
        });
        QObject::connect(&cancellingProcess, &AnalysisProcess::stderrLineReceived, [&](const QString& line) {
            if (line.contains("analysis fake slow request received")) {
                cancelSawSlowRequest = true;
                cancellingProcess.cancel();
            }
        });
        QObject::connect(&cancellingProcess, &AnalysisProcess::progressChanged, [&](int completed, int total) {
            cancelCompleted = completed;
            cancelTotal = total;
        });
        QObject::connect(&cancellingProcess, &AnalysisProcess::responseReceived, [&]() {
            ++cancelResponses;
        });
        QObject::connect(&cancellingProcess, &AnalysisProcess::finished, [&](bool cancelled) {
            cancelFinished = true;
            cancelFlag = cancelled;
            app.quit();
        });

        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        cancellingProcess.startAnalysis(cancellingConfig, {makeRequest("node:slow")});
        app.exec();

        require(cancelStarted, "cancellable analysis process should start");
        require(cancelSawSlowRequest, "cancellable analysis process should reach a running slow request");
        require(cancelCompleted == 0 && cancelTotal == 1, "cancellable analysis process should report zero progress");
        require(cancelFinished, "cancellable analysis process should emit finished");
        require(cancelFlag, "cancellable analysis process should report cancelled");
        require(!cancelFailed, "cancelled analysis process should not fail");
        require(cancelResponses == 0, "cancelled analysis process should not emit responses after cancellation");

        AnalysisProcess partialCancelProcess;
        KataGoConfig partialCancelConfig = config;
        partialCancelConfig.extraArgs = {"--slow-after-first-response"};
        bool partialCancelFailed = false;
        bool partialCancelFinished = false;
        bool partialCancelFlag = false;
        int partialCancelCompleted = 0;
        int partialCancelTotal = 0;
        std::vector<AnalysisResponse> partialCancelResponses;
        QObject::connect(&partialCancelProcess, &AnalysisProcess::failed, [&](const QString& message) {
            partialCancelFailed = true;
            std::cerr << message.toStdString() << '\n';
        });
        QObject::connect(&partialCancelProcess, &AnalysisProcess::responseReceived, [&](const AnalysisResponse& response) {
            partialCancelResponses.push_back(response);
            QTimer::singleShot(0, &partialCancelProcess, &AnalysisProcess::cancel);
        });
        QObject::connect(&partialCancelProcess, &AnalysisProcess::progressChanged, [&](int completed, int total) {
            partialCancelCompleted = completed;
            partialCancelTotal = total;
        });
        QObject::connect(&partialCancelProcess, &AnalysisProcess::finished, [&](bool cancelled) {
            partialCancelFinished = true;
            partialCancelFlag = cancelled;
            app.quit();
        });

        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        partialCancelProcess.startAnalysis(
            partialCancelConfig,
            {makeRequest("node:partial-cancel-1"), makeRequest("node:partial-cancel-2")});
        app.exec();

        require(partialCancelFinished, "partially completed cancellation should emit finished");
        require(partialCancelFlag, "partially completed cancellation should report cancelled");
        require(!partialCancelFailed, "partially completed cancellation should not fail for missing responses");
        require(
            partialCancelResponses.size() == 1 && partialCancelResponses.front().id == "node:partial-cancel-1",
            "partially completed cancellation should preserve the completed response only");
        require(
            partialCancelCompleted == 1 && partialCancelTotal == 2,
            "partially completed cancellation should keep partial progress without forcing completion");
    } catch (const std::exception& error) {
        std::cerr << "analysis_process_fixture_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "analysis_process_fixture_tests passed\n";
    return EXIT_SUCCESS;
}
