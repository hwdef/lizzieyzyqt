#include "KataGoProcess.h"

#include <QCoreApplication>
#include <QTimer>

#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace lizzie::engine;

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool contains(const std::vector<std::string>& values, const std::string& needle)
{
    return std::ranges::find(values, needle) != values.end();
}

}  // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc < 2) {
        std::cerr << "katago_process_fixture_tests requires fake engine path\n";
        return EXIT_FAILURE;
    }

    try {
        KataGoProcess missingConfigProcess;
        QString missingConfigDiagnostic;
        QObject::connect(&missingConfigProcess, &KataGoProcess::startupFailed, [&](const QString& message) {
            missingConfigDiagnostic = message;
        });
        KataGoConfig missingConfig;
        missingConfig.executable = std::filesystem::path(argv[1]);
        missingConfig.model = "fake-model.bin.gz";
        missingConfig.workingDirectory = "/tmp";
        missingConfigProcess.startGtp(missingConfig);
        require(missingConfigDiagnostic.contains("Command:"), "startup diagnostics should include command");
        require(
            missingConfigDiagnostic.contains("Working directory: /tmp"),
            "startup diagnostics should include working directory");
        require(missingConfigDiagnostic.contains("Stderr: (none)"), "startup diagnostics should include stderr summary");

        {
            KataGoProcess failingProcess;
            KataGoConfig failingConfig;
            failingConfig.executable = std::filesystem::path(argv[1]);
            failingConfig.model = "fake-model.bin.gz";
            failingConfig.gtpConfig = "fake-gtp.cfg";
            failingConfig.extraArgs = {"--exit-with-error"};

            QString failureDiagnostic;
            bool exited = false;
            QObject::connect(&failingProcess, &KataGoProcess::startupFailed, [&](const QString& message) {
                failureDiagnostic = message;
            });
            QObject::connect(&failingProcess, &KataGoProcess::exited, [&](int, QProcess::ExitStatus) {
                exited = true;
                app.quit();
            });
            QTimer timeout;
            timeout.setSingleShot(true);
            QObject::connect(&timeout, &QTimer::timeout, &app, &QCoreApplication::quit);
            timeout.start(5000);

            failingProcess.startGtp(failingConfig);
            app.exec();
            failingProcess.stop();

            require(exited, "nonzero fake KataGo process should exit");
            require(
                failureDiagnostic.contains("KataGo exited: code 9"),
                "nonzero GTP exit should report the exit code");
            require(failureDiagnostic.contains("Command:"), "nonzero GTP exit diagnostics should include command");
            require(
                failureDiagnostic.contains("Working directory:"),
                "nonzero GTP exit diagnostics should include working directory");
            require(
                failureDiagnostic.contains("Stderr:") &&
                    failureDiagnostic.contains("fatal fake GTP startup failure"),
                "nonzero GTP exit diagnostics should include stderr summary");
        }

        {
            KataGoProcess crashingProcess;
            KataGoConfig crashingConfig;
            crashingConfig.executable = std::filesystem::path(argv[1]);
            crashingConfig.model = "fake-model.bin.gz";
            crashingConfig.gtpConfig = "fake-gtp.cfg";
            crashingConfig.extraArgs = {"--crash-with-error"};

            QString crashDiagnostic;
            int startupFailureCount = 0;
            bool exited = false;
            QObject::connect(&crashingProcess, &KataGoProcess::startupFailed, [&](const QString& message) {
                ++startupFailureCount;
                crashDiagnostic = message;
            });
            QObject::connect(&crashingProcess, &KataGoProcess::exited, [&](int, QProcess::ExitStatus) {
                exited = true;
                app.quit();
            });
            QTimer timeout;
            timeout.setSingleShot(true);
            QObject::connect(&timeout, &QTimer::timeout, &app, &QCoreApplication::quit);
            timeout.start(5000);

            crashingProcess.startGtp(crashingConfig);
            app.exec();
            crashingProcess.stop();

            require(exited, "crashing fake KataGo process should emit exited");
            require(startupFailureCount == 1, "crashing GTP process should emit exactly one startup failure");
            require(crashDiagnostic.contains("Process error:"), "crash diagnostics should include process error code");
            require(crashDiagnostic.contains("--crash-with-error"), "crash diagnostics should include command line");
            require(crashDiagnostic.contains("fatal fake GTP crash"), "crash diagnostics should include stderr summary");
        }

        {
            KataGoProcess exitingProcess;
            KataGoConfig exitingConfig;
            exitingConfig.executable = std::filesystem::path(argv[1]);
            exitingConfig.model = "fake-model.bin.gz";
            exitingConfig.gtpConfig = "fake-gtp.cfg";
            exitingConfig.extraArgs = {"--exit-after-list-commands"};

            bool exited = false;
            bool failed = false;
            std::vector<std::string> matchedCommands;
            QObject::connect(&exitingProcess, &KataGoProcess::startupFailed, [&](const QString& message) {
                failed = true;
                std::cerr << message.toStdString() << '\n';
                app.quit();
            });
            QObject::connect(&exitingProcess, &KataGoProcess::commandResponseReceived, [&](const QueuedGtpResponse& response) {
                if (response.command.has_value()) {
                    matchedCommands.push_back(response.command->name);
                }
            });
            QObject::connect(&exitingProcess, &KataGoProcess::exited, [&](int, QProcess::ExitStatus) {
                exited = true;
                app.quit();
            });
            QTimer timeout;
            timeout.setSingleShot(true);
            QObject::connect(&timeout, &QTimer::timeout, &app, &QCoreApplication::quit);
            timeout.start(5000);

            exitingProcess.startGtp(exitingConfig);
            app.exec();
            exitingProcess.stop();

            require(!failed, "clean GTP exit after startup probe should not report startup failure");
            require(exited, "clean GTP exit after startup probe should emit exited");
            require(contains(matchedCommands, "name"), "clean GTP exit should preserve final name response");
            require(contains(matchedCommands, "version"), "clean GTP exit should preserve final version response");
            require(
                contains(matchedCommands, "list_commands"),
                "clean GTP exit should preserve final list_commands response before exited");
        }

        {
            KataGoProcess stopExitProcess;
            KataGoConfig stopExitConfig;
            stopExitConfig.executable = std::filesystem::path(argv[1]);
            stopExitConfig.model = "fake-model.bin.gz";
            stopExitConfig.gtpConfig = "fake-gtp.cfg";
            stopExitConfig.extraArgs = {"--exit-after-kata-stop"};

            bool exited = false;
            bool failed = false;
            bool stopSent = false;
            std::vector<std::string> matchedCommands;
            QObject::connect(&stopExitProcess, &KataGoProcess::startupFailed, [&](const QString& message) {
                failed = true;
                std::cerr << message.toStdString() << '\n';
                app.quit();
            });
            QObject::connect(&stopExitProcess, &KataGoProcess::commandResponseReceived, [&](const QueuedGtpResponse& response) {
                if (response.command.has_value()) {
                    matchedCommands.push_back(response.command->name);
                }
                if (!stopSent && contains(matchedCommands, "name") && contains(matchedCommands, "version") &&
                    contains(matchedCommands, "list_commands")) {
                    stopSent = true;
                    stopExitProcess.sendCommand(GtpCommand{std::nullopt, "kata-stop", {}});
                }
            });
            QObject::connect(&stopExitProcess, &KataGoProcess::exited, [&](int, QProcess::ExitStatus) {
                exited = true;
                app.quit();
            });
            QTimer timeout;
            timeout.setSingleShot(true);
            QObject::connect(&timeout, &QTimer::timeout, &app, &QCoreApplication::quit);
            timeout.start(5000);

            stopExitProcess.startGtp(stopExitConfig);
            app.exec();
            stopExitProcess.stop();

            require(!failed, "GTP exit after kata-stop should not report startup failure");
            require(exited, "GTP exit after kata-stop should emit exited");
            require(stopSent, "GTP exit after kata-stop test should send kata-stop after startup probe");
            require(
                contains(matchedCommands, "kata-stop"),
                "GTP exit after kata-stop should preserve final kata-stop response before exited");
        }

        {
            KataGoProcess workingDirectoryProcess;
            KataGoConfig workingDirectoryConfig;
            workingDirectoryConfig.executable = std::filesystem::path(argv[1]);
            workingDirectoryConfig.model = "fake-model.bin.gz";
            workingDirectoryConfig.gtpConfig = "fake-gtp.cfg";
            workingDirectoryConfig.extraArgs = {"--print-cwd"};

            const std::filesystem::path inheritedWorkingDirectory = std::filesystem::current_path();
            const std::filesystem::path explicitWorkingDirectory =
                std::filesystem::temp_directory_path() / "lizzie-gtp-cwd-fixture";
            std::filesystem::create_directories(explicitWorkingDirectory);

            bool workingDirectoryFailed = false;
            bool workingDirectoryReady = false;
            std::vector<std::filesystem::path> observedWorkingDirectories;
            std::vector<std::string> workingDirectoryCommands;
            QObject::connect(&workingDirectoryProcess, &KataGoProcess::startupFailed, [&](const QString& message) {
                workingDirectoryFailed = true;
                std::cerr << message.toStdString() << '\n';
                app.quit();
            });
            QObject::connect(&workingDirectoryProcess, &KataGoProcess::stderrLineReceived, [&](const QString& line) {
                constexpr std::string_view prefix = "gtp fake cwd ";
                const std::string text = line.toStdString();
                if (text.starts_with(prefix)) {
                    observedWorkingDirectories.push_back(text.substr(prefix.size()));
                }
            });
            QObject::connect(&workingDirectoryProcess, &KataGoProcess::commandResponseReceived, [&](const QueuedGtpResponse& response) {
                if (response.command.has_value()) {
                    workingDirectoryCommands.push_back(response.command->name);
                }
                if (contains(workingDirectoryCommands, "name") &&
                    contains(workingDirectoryCommands, "version") &&
                    contains(workingDirectoryCommands, "list_commands")) {
                    workingDirectoryReady = true;
                    app.quit();
                }
            });

            workingDirectoryConfig.workingDirectory = explicitWorkingDirectory;
            QTimer::singleShot(5000, &app, &QCoreApplication::quit);
            workingDirectoryProcess.startGtp(workingDirectoryConfig);
            app.exec();
            workingDirectoryProcess.stop();
            require(workingDirectoryReady, "GTP process with explicit working directory should complete startup probe");
            require(!workingDirectoryFailed, "GTP process with explicit working directory should not fail");
            require(observedWorkingDirectories.size() == 1, "explicit GTP working directory run should report cwd");
            require(
                std::filesystem::equivalent(observedWorkingDirectories.front(), explicitWorkingDirectory),
                "GTP process should use configured working directory");

            workingDirectoryReady = false;
            workingDirectoryFailed = false;
            observedWorkingDirectories.clear();
            workingDirectoryCommands.clear();
            workingDirectoryConfig.workingDirectory.clear();
            QTimer::singleShot(5000, &app, &QCoreApplication::quit);
            workingDirectoryProcess.startGtp(workingDirectoryConfig);
            app.exec();
            workingDirectoryProcess.stop();
            require(workingDirectoryReady, "GTP process with inherited working directory should complete startup probe");
            require(!workingDirectoryFailed, "GTP process with inherited working directory should not fail");
            require(observedWorkingDirectories.size() == 1, "inherited GTP working directory run should report cwd");
            require(
                std::filesystem::equivalent(observedWorkingDirectories.front(), inheritedWorkingDirectory),
                "GTP process should clear stale working directory when config is empty");
        }

        {
            KataGoProcess libraryPathProcess;
            KataGoConfig libraryPathConfig;
            libraryPathConfig.executable = std::filesystem::path(argv[1]);
            libraryPathConfig.model = "fake-model.bin.gz";
            libraryPathConfig.gtpConfig = "fake-gtp.cfg";
            libraryPathConfig.libraryPaths = {
                std::filesystem::temp_directory_path() / "lizzie-cuda-lib-fixture",
                std::filesystem::temp_directory_path() / "lizzie-cudnn-lib-fixture",
            };
            libraryPathConfig.extraArgs = {"--print-library-path-env", "--exit-after-list-commands"};

            QString observedLibraryPath;
            bool exited = false;
            bool failed = false;
            QObject::connect(&libraryPathProcess, &KataGoProcess::startupFailed, [&](const QString& message) {
                failed = true;
                std::cerr << message.toStdString() << '\n';
                app.quit();
            });
            QObject::connect(&libraryPathProcess, &KataGoProcess::stderrLineReceived, [&](const QString& line) {
                constexpr std::string_view prefix = "gtp fake library path ";
                const std::string text = line.toStdString();
                if (text.starts_with(prefix)) {
                    observedLibraryPath = QString::fromStdString(text.substr(prefix.size()));
                }
            });
            QObject::connect(&libraryPathProcess, &KataGoProcess::exited, [&](int, QProcess::ExitStatus) {
                exited = true;
                app.quit();
            });

            QTimer::singleShot(5000, &app, &QCoreApplication::quit);
            libraryPathProcess.startGtp(libraryPathConfig);
            app.exec();
            libraryPathProcess.stop();

            require(exited, "GTP library path process should exit after startup probe");
            require(!failed, "GTP library path process should not fail");
            require(
                observedLibraryPath.contains("lizzie-cuda-lib-fixture"),
                "GTP process should pass configured CUDA library paths to the child environment");
            require(
                observedLibraryPath.contains("lizzie-cudnn-lib-fixture"),
                "GTP process should pass configured cuDNN library paths to the child environment");
        }

        KataGoProcess process;
        KataGoConfig config;
        config.executable = std::filesystem::path(argv[1]);
        config.model = "fake-model.bin.gz";
        config.gtpConfig = "fake-gtp.cfg";
        config.extraArgs = {
            "--raw-protocol-line",
            "--require-gtp-argv",
            "--backend-stderr-lines",
            "--ownership-only-kata-analyze-line",
        };

        bool failed = false;
        bool stderrSeen = false;
        bool openClSeen = false;
        bool tensorRtSeen = false;
        bool argvSeen = false;
        bool rawProtocolLineSeen = false;
        bool analysisSeen = false;
        bool ownershipOnlyAnalyzeLineSeen = false;
        bool invalidCommandRejected = false;
        bool invalidCommandProtocolSeen = false;
        bool analyzeCommandSent = false;
        bool genmoveCommandSent = false;
        bool genmoveSeen = false;
        std::vector<std::string> matchedCommands;
        std::vector<KataAnalyzeInfo> analysisInfos;
        std::optional<KataAnalyzeLine> analysisLine;

        QObject::connect(&process, &KataGoProcess::startupFailed, [&](const QString& message) {
            failed = true;
            std::cerr << message.toStdString() << '\n';
            app.quit();
        });
        QObject::connect(&process, &KataGoProcess::stderrLineReceived, [&](const QString& line) {
            if (line.contains("CUDA fake backend")) {
                stderrSeen = true;
            }
            if (line.contains("OpenCL fake backend")) {
                openClSeen = true;
            }
            if (line.contains("TensorRT fake backend")) {
                tensorRtSeen = true;
            }
            if (line.contains("gtp fake argv ok")) {
                argvSeen = true;
            }
        });
        QObject::connect(&process, &KataGoProcess::protocolLineReceived, [&](const QString& line) {
            if (line == "raw protocol diagnostic") {
                rawProtocolLineSeen = true;
            }
            if (line.contains("kata-stop clear_board")) {
                invalidCommandProtocolSeen = true;
            }
        });
        QObject::connect(&process, &KataGoProcess::commandResponseReceived, [&](const QueuedGtpResponse& response) {
            if (response.command.has_value()) {
                matchedCommands.push_back(response.command->name);
            }
            if (!analyzeCommandSent && contains(matchedCommands, "name") && contains(matchedCommands, "version") &&
                contains(matchedCommands, "list_commands")) {
                invalidCommandRejected =
                    process.sendCommand(GtpCommand{std::nullopt, "kata-stop\nclear_board", {}}) < 0;
                analyzeCommandSent = true;
                process.sendCommand(GtpCommand{std::nullopt, "kata-analyze", {"10", "ownership", "true"}});
            }
            if (response.command.has_value() && response.command->name == "genmove" && response.response.payload == "D4") {
                genmoveSeen = true;
                process.stop();
                app.quit();
            }
        });
        QObject::connect(&process, &KataGoProcess::kataAnalyzeInfoReceived, [&](const KataAnalyzeInfo& info) {
            analysisSeen = true;
            analysisInfos.push_back(info);
            if (!genmoveCommandSent) {
                genmoveCommandSent = true;
                process.sendCommand(GtpCommand{std::nullopt, "genmove", {"B"}});
            }
        });
        QObject::connect(&process, &KataGoProcess::kataAnalyzeLineReceived, [&](const KataAnalyzeLine& line) {
            if (line.infos.empty() && !line.rootInfo.has_value() && line.ownership.size() == 81 &&
                line.ownership[0] == 0.45 && line.ownership[1] == -0.45) {
                ownershipOnlyAnalyzeLineSeen = true;
            }
            analysisLine = line;
        });

        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        process.startGtp(config);
        app.exec();
        process.stop();

        require(!failed, "KataGoProcess should not report startup failure");
        require(stderrSeen, "stderr diagnostic line should be preserved");
        require(openClSeen, "OpenCL stderr diagnostic line should be preserved");
        require(tensorRtSeen, "TensorRT stderr diagnostic line should be preserved");
        require(argvSeen, "KataGoProcess should start KataGo in GTP mode with model and config");
        require(rawProtocolLineSeen, "raw unparsed protocol lines should be preserved");
        require(contains(matchedCommands, "name"), "startup probe should match name response");
        require(contains(matchedCommands, "version"), "startup probe should match version response");
        require(contains(matchedCommands, "list_commands"), "startup probe should match list_commands response");
        require(invalidCommandRejected, "KataGoProcess should reject invalid GTP command names");
        require(!invalidCommandProtocolSeen, "KataGoProcess should not write invalid GTP command names");
        require(analysisSeen, "raw kata-analyze info should be parsed");
        require(ownershipOnlyAnalyzeLineSeen, "ownership-only kata-analyze line should be emitted");
        require(analysisLine.has_value(), "raw kata-analyze line should be emitted");
        require(analysisLine->rootInfo.has_value(), "raw kata-analyze line should preserve rootInfo");
        require(analysisLine->rootInfo->visits == 96, "raw kata-analyze line should preserve root visits");
        require(analysisLine->rootInfo->scoreMean == 1.8, "raw kata-analyze line should preserve root scoreLead");
        require(analysisInfos.size() == 2, "multi-candidate kata-analyze line should emit each info block");
        require(analysisInfos[0].move == "D4", "parsed analysis info should preserve first move");
        require(analysisInfos[0].visits == 64, "parsed analysis info should preserve first visits");
        require(
            analysisInfos[0].pvVisits.size() == 3 && analysisInfos[0].pvVisits[1] == 32,
            "parsed analysis info should preserve PV visits");
        require(analysisInfos[1].move == "E5", "parsed analysis info should preserve second move");
        require(analysisInfos[1].scoreMean == -0.6, "parsed analysis info should map second scoreLead");
        require(genmoveSeen, "genmove response should be matched and preserved");
    } catch (const std::exception& error) {
        std::cerr << "katago_process_fixture_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "katago_process_fixture_tests passed\n";
    return EXIT_SUCCESS;
}
