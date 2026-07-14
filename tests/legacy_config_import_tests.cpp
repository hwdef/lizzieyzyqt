#include "LegacyConfigImport.h"

#include <QByteArray>
#include <QDir>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonObject>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

}  // namespace

int main(int argc, char* argv[])
{
    if (qgetenv("QT_QPA_PLATFORM").isEmpty()) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }

    QGuiApplication app(argc, argv);

    try {
        const QDir baseDir("/tmp/lizzie legacy");

        QJsonArray engines;
        engines.push_back(QJsonObject{
            {"command", R"(/opt/katago-compare gtp -model models/compare.bin.gz -config cfg/compare.cfg)"},
            {"isDefault", false},
        });
        engines.push_back(QJsonObject{
            {"command",
             R"(bin/katago gtp -model models/main.bin.gz -config cfg/main.cfg --override-config "logDir=C:\KataGo Logs\main run" "" quoted"""value)"},
            {"isDefault", true},
        });
        engines.push_back(QJsonObject{
            {"command", "ssh remote katago"},
            {"useJavaSSH", true},
        });

        QJsonObject leelaz;
        leelaz.insert("engine-settings-list", engines);
        leelaz.insert("engine-start-location", "engine cwd");
        leelaz.insert("analyze-update-interval-centisec", 123);
        leelaz.insert("max-analyze-time-seconds", 9);

        QJsonObject ui;
        ui.insert(
            "analysis-engine-command",
            R"(bin/katago analysis -model models/main.bin.gz -config cfg/analysis.cfg -quit-without-waiting)");
        ui.insert("limit-time", true);
        ui.insert("limit-playout", true);
        ui.insert("limit-playouts", 456);
        ui.insert("kata-visits-playouts-settings", true);
        ui.insert("kata-visits-txt", 789);
        ui.insert("kata-playouts-txt", 987);
        ui.insert("chk-kata-engine-pda", true);
        ui.insert("txt-kata-engine-pda", "-1.25");
        ui.insert("chk-kata-engine-wrn", true);
        ui.insert("txt-kata-engine-wrn", "0.35");
        ui.insert("use-language", 1);
        ui.insert("theme", "dark-theme");
        ui.insert("show-katago-estimate", true);

        const QJsonObject root{{"leelaz", leelaz}, {"ui", ui}};
        const lizzie::app::LegacyConfigImportResult imported =
            lizzie::app::importLegacyConfigObject(root, baseDir, lizzie::app::EngineUiSettings{});

        require(imported.hasUsableKataGo, "legacy import should find usable KataGo settings");
        require(imported.importedEngines == 2, "legacy import should import main and compare engines");
        require(imported.notes.contains("Skipped Java SSH engine entry"), "legacy import should report skipped SSH engine");

        const QString mainExecutable = QString::fromStdString(imported.settings.config.executable.string());
        require(mainExecutable == "/tmp/lizzie legacy/bin/katago", "main executable should resolve relative path");
        require(
            QString::fromStdString(imported.settings.config.model.string()) == "/tmp/lizzie legacy/models/main.bin.gz",
            "main model should resolve relative path");
        require(
            QString::fromStdString(imported.settings.config.gtpConfig.string()) == "/tmp/lizzie legacy/cfg/main.cfg",
            "main GTP config should resolve relative path");
        require(
            QString::fromStdString(imported.settings.config.analysisConfig.string()) ==
                "/tmp/lizzie legacy/cfg/analysis.cfg",
            "analysis config should merge into main config");
        require(
            QString::fromStdString(imported.settings.config.workingDirectory.string()) ==
                "/tmp/lizzie legacy/engine cwd",
            "main working directory should import from legacy engine-start-location");
        require(
            imported.settings.config.extraArgs ==
                std::vector<std::string>({
                    "--override-config",
                    R"(logDir=C:\KataGo Logs\main run)",
                    "",
                    "quoted\"value",
                }),
            "main extra args should preserve quoted spaces, empty args, backslashes, and literal quotes");

        require(
            imported.settings.comparisonConfig.executable == "/opt/katago-compare",
            "compare executable should import");
        require(
            QString::fromStdString(imported.settings.comparisonConfig.model.string()) ==
                "/tmp/lizzie legacy/models/compare.bin.gz",
            "compare model should resolve relative path");
        require(
            QString::fromStdString(imported.settings.comparisonConfig.gtpConfig.string()) ==
                "/tmp/lizzie legacy/cfg/compare.cfg",
            "compare GTP config should resolve relative path");
        require(
            QString::fromStdString(imported.settings.comparisonConfig.workingDirectory.string()) ==
                "/tmp/lizzie legacy/engine cwd",
            "compare working directory should import from legacy engine-start-location");

        require(imported.settings.realtimeOptions.intervalCentiseconds == 123, "interval should import");
        require(imported.settings.realtimeOptions.maxTimeSeconds == 9.0, "max time should import");
        require(imported.settings.realtimeOptions.maxVisits == 789, "max visits should import");
        require(imported.settings.realtimeOptions.maxPlayouts == 987, "max playouts should import");
        require(
            imported.settings.realtimeOptions.playoutDoublingAdvantage == -1.25,
            "PDA should import");
        require(imported.settings.realtimeOptions.analysisWideRootNoise == 0.35, "WRN should import");
        require(imported.settings.appearance.language == lizzie::app::LanguageMode::Chinese, "language should import");
        require(imported.settings.appearance.theme == lizzie::app::ThemeMode::Dark, "theme should import");
        require(imported.settings.boardDisplay.showOwnership, "ownership display hint should import");

        QJsonArray stringBoolEngines;
        stringBoolEngines.push_back(QJsonObject{
            {"command", "katago gtp --model models/compare-string.bin.gz --config cfg/compare-string.cfg"},
            {"isDefault", "false"},
        });
        stringBoolEngines.push_back(QJsonObject{
            {"command", "katago gtp --model models/main-string.bin.gz --config cfg/main-string.cfg"},
            {"isDefault", "true"},
        });
        stringBoolEngines.push_back(QJsonObject{
            {"command", "katago gtp --model models/ssh.bin.gz --config cfg/ssh.cfg"},
            {"useJavaSSH", "true"},
        });
        QJsonObject stringBoolLeelaz;
        stringBoolLeelaz.insert("engine-settings-list", stringBoolEngines);
        stringBoolLeelaz.insert("max-analyze-time-seconds", "12");
        QJsonObject stringBoolUi;
        stringBoolUi.insert("limit-time", "true");
        stringBoolUi.insert("limit-playout", "1");
        stringBoolUi.insert("limit-playouts", "345");
        stringBoolUi.insert("kata-visits-playouts-settings", "true");
        stringBoolUi.insert("kata-visits-txt", "678");
        stringBoolUi.insert("kata-playouts-txt", "901");
        stringBoolUi.insert("autoload-kata-engine-pda", "true");
        stringBoolUi.insert("auto-load-txt-kata-engine-pda", "-0.5");
        stringBoolUi.insert("chk-kata-engine-wrn", "1");
        stringBoolUi.insert("txt-kata-engine-wrn", "0.25");
        stringBoolUi.insert("show-katago-estimate-onmainboard", "true");
        const lizzie::app::LegacyConfigImportResult stringBoolImported =
            lizzie::app::importLegacyConfigObject(
                QJsonObject{{"leelaz", stringBoolLeelaz}, {"ui", stringBoolUi}},
                baseDir,
                lizzie::app::EngineUiSettings{});
        require(
            QString::fromStdString(stringBoolImported.settings.config.model.string()) ==
                "/tmp/lizzie legacy/models/main-string.bin.gz",
            "string true isDefault should choose the main legacy engine first");
        require(
            QString::fromStdString(stringBoolImported.settings.comparisonConfig.model.string()) ==
                "/tmp/lizzie legacy/models/compare-string.bin.gz",
            "string false isDefault should leave the compare legacy engine second");
        require(
            stringBoolImported.notes.contains("Skipped Java SSH engine entry"),
            "string true useJavaSSH should skip Java SSH entries");
        require(stringBoolImported.settings.realtimeOptions.maxTimeSeconds == 12.0, "string true max time flag should import");
        require(stringBoolImported.settings.realtimeOptions.maxVisits == 678, "string true visits flag should import");
        require(stringBoolImported.settings.realtimeOptions.maxPlayouts == 901, "string true playouts flag should import");
        require(
            stringBoolImported.settings.realtimeOptions.playoutDoublingAdvantage == -0.5,
            "string true PDA flag should import");
        require(
            stringBoolImported.settings.realtimeOptions.analysisWideRootNoise == 0.25,
            "string one WRN flag should import");
        require(
            stringBoolImported.settings.boardDisplay.showOwnership,
            "string true ownership display hint should import");

        QJsonObject nonFiniteUi;
        nonFiniteUi.insert("chk-kata-engine-pda", true);
        nonFiniteUi.insert("txt-kata-engine-pda", "nan");
        nonFiniteUi.insert("chk-kata-engine-wrn", true);
        nonFiniteUi.insert("txt-kata-engine-wrn", "inf");
        const QJsonObject nonFiniteRoot{{"leelaz", QJsonObject{}}, {"ui", nonFiniteUi}};
        const lizzie::app::LegacyConfigImportResult nonFinite =
            lizzie::app::importLegacyConfigObject(nonFiniteRoot, baseDir, lizzie::app::EngineUiSettings{});
        require(
            !nonFinite.settings.realtimeOptions.playoutDoublingAdvantage.has_value(),
            "non-finite legacy PDA should not import");
        require(
            !nonFinite.settings.realtimeOptions.analysisWideRootNoise.has_value(),
            "non-finite legacy WRN should not import");

        QJsonObject boundedLeelaz;
        boundedLeelaz.insert("analyze-update-interval-centisec", "20000");
        boundedLeelaz.insert("max-analyze-time-seconds", "999999.5");
        QJsonObject boundedUi;
        boundedUi.insert("limit-time", true);
        boundedUi.insert("limit-playout", true);
        boundedUi.insert("limit-playouts", "200000001");
        boundedUi.insert("kata-visits-playouts-settings", true);
        boundedUi.insert("kata-visits-txt", "200000002");
        boundedUi.insert("kata-playouts-txt", "200000003");
        boundedUi.insert("chk-kata-engine-pda", true);
        boundedUi.insert("txt-kata-engine-pda", "25");
        boundedUi.insert("chk-kata-engine-wrn", true);
        boundedUi.insert("txt-kata-engine-wrn", "99");
        const lizzie::app::LegacyConfigImportResult bounded =
            lizzie::app::importLegacyConfigObject(
                QJsonObject{{"leelaz", boundedLeelaz}, {"ui", boundedUi}},
                baseDir,
                lizzie::app::EngineUiSettings{});
        require(
            bounded.settings.realtimeOptions.intervalCentiseconds == 10000,
            "legacy interval should clamp to the settings maximum");
        require(
            bounded.settings.realtimeOptions.maxTimeSeconds == 86400.0,
            "legacy max time should import from string values and clamp to the settings maximum");
        require(
            bounded.settings.realtimeOptions.maxVisits == 100000000,
            "legacy max visits should import from string values and clamp to the settings maximum");
        require(
            bounded.settings.realtimeOptions.maxPlayouts == 100000000,
            "legacy max playouts should import from string values and clamp to the settings maximum");
        require(
            bounded.settings.realtimeOptions.playoutDoublingAdvantage == 10.0,
            "legacy PDA should clamp to the settings maximum");
        require(
            bounded.settings.realtimeOptions.analysisWideRootNoise == 10.0,
            "legacy WRN should clamp to the settings maximum");

        QJsonObject disabledLeelaz;
        disabledLeelaz.insert("analyze-update-interval-centisec", "0");
        disabledLeelaz.insert("max-analyze-time-seconds", "-1");
        QJsonObject disabledUi;
        disabledUi.insert("limit-time", true);
        disabledUi.insert("limit-playout", true);
        disabledUi.insert("limit-playouts", "0");
        disabledUi.insert("kata-visits-playouts-settings", true);
        disabledUi.insert("kata-visits-txt", "-1");
        disabledUi.insert("kata-playouts-txt", "not-a-number");
        disabledUi.insert("chk-kata-engine-pda", true);
        disabledUi.insert("txt-kata-engine-pda", "0");
        disabledUi.insert("chk-kata-engine-wrn", true);
        disabledUi.insert("txt-kata-engine-wrn", "-0.1");
        const lizzie::app::LegacyConfigImportResult disabled =
            lizzie::app::importLegacyConfigObject(
                QJsonObject{{"leelaz", disabledLeelaz}, {"ui", disabledUi}},
                baseDir,
                lizzie::app::EngineUiSettings{});
        require(
            disabled.settings.realtimeOptions.intervalCentiseconds == 50,
            "zero legacy interval should leave the default interval");
        require(
            !disabled.settings.realtimeOptions.maxTimeSeconds.has_value(),
            "negative legacy max time should not import");
        require(
            !disabled.settings.realtimeOptions.maxVisits.has_value(),
            "negative legacy max visits should not import");
        require(
            !disabled.settings.realtimeOptions.maxPlayouts.has_value(),
            "invalid legacy max playouts should not import");
        require(
            !disabled.settings.realtimeOptions.playoutDoublingAdvantage.has_value(),
            "zero legacy PDA should not import");
        require(
            !disabled.settings.realtimeOptions.analysisWideRootNoise.has_value(),
            "negative legacy WRN should not import");

        lizzie::app::EngineUiSettings staleSettings;
        staleSettings.config.executable = "/old/katago";
        staleSettings.config.model = "/old/model.bin.gz";
        staleSettings.config.gtpConfig = "/old/gtp.cfg";
        staleSettings.config.analysisConfig = "/old/analysis.cfg";
        staleSettings.config.workingDirectory = "/old/workdir";
        staleSettings.config.extraArgs = {"--old-flag"};
        staleSettings.comparisonConfig.executable = "/old/compare";
        staleSettings.comparisonConfig.workingDirectory = "/old/compare-workdir";
        const lizzie::app::LegacyConfigImportResult replaced =
            lizzie::app::importLegacyConfigObject(root, baseDir, staleSettings);
        require(
            QString::fromStdString(replaced.settings.config.executable.string()) ==
                "/tmp/lizzie legacy/bin/katago",
            "legacy import should replace stale main executable settings");
        require(
            QString::fromStdString(replaced.settings.config.gtpConfig.string()) ==
                "/tmp/lizzie legacy/cfg/main.cfg",
            "legacy import should replace stale main GTP config");
        require(
            QString::fromStdString(replaced.settings.config.analysisConfig.string()) ==
                "/tmp/lizzie legacy/cfg/analysis.cfg",
            "legacy import should replace stale analysis config");
        require(
            replaced.settings.config.extraArgs.size() == 4 &&
                replaced.settings.config.extraArgs.front() == "--override-config" &&
                replaced.settings.config.extraArgs[2].empty(),
            "legacy import should replace stale extra args");
        require(
            QString::fromStdString(replaced.settings.config.workingDirectory.string()) ==
                "/tmp/lizzie legacy/engine cwd",
            "legacy import should replace stale main working directory");
        require(
            replaced.settings.comparisonConfig.executable == "/opt/katago-compare",
            "legacy import should replace stale compare engine settings");
        require(
            QString::fromStdString(replaced.settings.comparisonConfig.gtpConfig.string()) ==
                "/tmp/lizzie legacy/cfg/compare.cfg",
            "legacy import should replace stale compare GTP config");
        require(
            QString::fromStdString(replaced.settings.comparisonConfig.workingDirectory.string()) ==
                "/tmp/lizzie legacy/engine cwd",
            "legacy import should replace stale compare working directory");

        QJsonArray analysisCompareEngines;
        analysisCompareEngines.push_back(QJsonObject{
            {"command", "katago gtp --model models/main.bin.gz --config cfg/main.cfg"},
            {"isDefault", true},
        });
        analysisCompareEngines.push_back(QJsonObject{
            {"command",
             "katago analysis --model models/compare-analysis.bin.gz --config cfg/compare-analysis.cfg "
             "-quit-without-waiting --override-config logDir=/tmp/compare-analysis"},
            {"isDefault", false},
        });
        const QJsonObject analysisCompareRoot{
            {"leelaz",
             QJsonObject{
                 {"engine-settings-list", analysisCompareEngines},
                 {"engine-start-location", "engine cwd"},
             }},
            {"ui", QJsonObject{}},
        };
        lizzie::app::EngineUiSettings staleCompareAnalysisSettings;
        staleCompareAnalysisSettings.comparisonConfig.analysisConfig = "/old/compare-analysis.cfg";
        const lizzie::app::LegacyConfigImportResult analysisCompareImported =
            lizzie::app::importLegacyConfigObject(analysisCompareRoot, baseDir, staleCompareAnalysisSettings);
        require(analysisCompareImported.importedEngines == 2, "legacy import should count compare analysis engine");
        require(
            QString::fromStdString(analysisCompareImported.settings.comparisonConfig.model.string()) ==
                "/tmp/lizzie legacy/models/compare-analysis.bin.gz",
            "compare analysis model should resolve relative path");
        require(
            QString::fromStdString(analysisCompareImported.settings.comparisonConfig.analysisConfig.string()) ==
                "/tmp/lizzie legacy/cfg/compare-analysis.cfg",
            "compare analysis config should resolve relative path");
        require(
            analysisCompareImported.settings.comparisonConfig.extraArgs.size() == 2 &&
                analysisCompareImported.settings.comparisonConfig.extraArgs[0] == "--override-config" &&
                analysisCompareImported.settings.comparisonConfig.extraArgs[1] == "logDir=/tmp/compare-analysis",
            "compare analysis extra args should import");
        require(
            QString::fromStdString(analysisCompareImported.settings.comparisonConfig.workingDirectory.string()) ==
                "/tmp/lizzie legacy/engine cwd",
            "compare analysis working directory should import from legacy engine-start-location");

        const QJsonObject unsupportedRoot{
            {"leelaz", QJsonObject{{"engine-command", "leelaz --gtp"}}},
            {"ui", QJsonObject{}},
        };
        const lizzie::app::LegacyConfigImportResult unsupported =
            lizzie::app::importLegacyConfigObject(unsupportedRoot, baseDir, lizzie::app::EngineUiSettings{});
        require(!unsupported.hasUsableKataGo, "unsupported legacy engine command should not be usable");
        require(
            unsupported.notes.contains("Skipped engine command that is not a KataGo gtp/analysis command"),
            "unsupported legacy engine command should produce a note");

        const QJsonObject leelazGtpRoot{
            {"leelaz",
             QJsonObject{
                 {"engine-command", "leelaz gtp --model models/leelaz.bin.gz --config cfg/leelaz.cfg"}}},
            {"ui", QJsonObject{}},
        };
        const lizzie::app::LegacyConfigImportResult leelazGtp =
            lizzie::app::importLegacyConfigObject(leelazGtpRoot, baseDir, lizzie::app::EngineUiSettings{});
        require(!leelazGtp.hasUsableKataGo, "legacy Leela-style GTP command should not be imported as KataGo");
        require(
            leelazGtp.notes.contains("Skipped engine command that is not a KataGo gtp/analysis command"),
            "legacy Leela-style GTP command should produce a skipped-command note");

        const QJsonObject analysisOnlyRoot{
            {"leelaz", QJsonObject{}},
            {"ui",
             QJsonObject{
                 {"analysis-engine-command",
                  "katago analysis --model models/analysis-only.bin.gz --config cfg/analysis-only.cfg "
                  "-quit-without-waiting"}}},
        };
        const lizzie::app::LegacyConfigImportResult analysisOnly =
            lizzie::app::importLegacyConfigObject(analysisOnlyRoot, baseDir, lizzie::app::EngineUiSettings{});
        require(analysisOnly.hasUsableKataGo, "analysis-only Java config import should be usable");
        require(analysisOnly.importedEngines == 1, "analysis-only Java config import should count the main engine");
        require(
            analysisOnly.settings.config.hasAnalysisMinimum(),
            "analysis-only Java config import should produce analysis settings");
        require(
            !analysisOnly.settings.config.hasGtpMinimum(),
            "analysis-only Java config import should not invent realtime GTP settings");

        const QJsonObject estimateOnlyRoot{
            {"leelaz", QJsonObject{}},
            {"ui",
             QJsonObject{
                 {"katago-estimate-engine-command",
                  "katago gtp --model models/estimate-only.bin.gz --config cfg/estimate-only.cfg"}}},
        };
        const lizzie::app::LegacyConfigImportResult estimateOnly =
            lizzie::app::importLegacyConfigObject(estimateOnlyRoot, baseDir, lizzie::app::EngineUiSettings{});
        require(estimateOnly.hasUsableKataGo, "estimate-only Java config import should be usable");
        require(estimateOnly.importedEngines == 1, "estimate-only Java config import should count the main engine");
        require(
            estimateOnly.settings.config.hasGtpMinimum(),
            "estimate-only Java config import should produce realtime GTP settings");
        require(
            !estimateOnly.settings.config.hasAnalysisMinimum(),
            "estimate-only Java config import should not invent analysis settings");

        const QJsonObject pathCommandRoot{
            {"leelaz",
             QJsonObject{
                 {"engine-command", "katago gtp --model models/path-model.bin.gz --config cfg/path-gtp.cfg"}}},
            {"ui", QJsonObject{}},
        };
        const lizzie::app::LegacyConfigImportResult pathCommand =
            lizzie::app::importLegacyConfigObject(pathCommandRoot, baseDir, lizzie::app::EngineUiSettings{});
        require(pathCommand.hasUsableKataGo, "bare PATH KataGo command should be usable");
        require(pathCommand.settings.config.executable == "katago", "bare executable should stay PATH-resolved");
        require(
            QString::fromStdString(pathCommand.settings.config.model.string()) ==
                "/tmp/lizzie legacy/models/path-model.bin.gz",
            "long --model option should resolve relative model path");
        require(
            QString::fromStdString(pathCommand.settings.config.gtpConfig.string()) ==
                "/tmp/lizzie legacy/cfg/path-gtp.cfg",
            "long --config option should resolve relative GTP config path");
    } catch (const std::exception& error) {
        std::cerr << "legacy_config_import_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "legacy_config_import_tests passed\n";
    return EXIT_SUCCESS;
}
