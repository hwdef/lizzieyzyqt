#include "AppSettings.h"

#include <QByteArray>
#include <QGuiApplication>
#include <QKeySequence>
#include <QSettings>
#include <QTemporaryDir>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool near(double left, double right)
{
    return std::abs(left - right) < 0.000001;
}

void testCommandArgumentRoundTrip()
{
    const std::vector<std::string> arguments{
        "--override-config",
        "logDir=C:\\KataGo Logs\\main run",
        R"(quoted"value\path)",
        "",
        "plain",
    };
    const QString joined = lizzie::app::joinCommandArguments(arguments);
    const std::vector<std::string> split = lizzie::app::splitCommandArguments(joined);
    require(split == arguments, "joined command arguments should split back to the original vector");
    require(
        lizzie::app::splitCommandArguments(QStringLiteral(R"(  "" "a b" quoted"""value plain  )")) ==
            std::vector<std::string>({"", "a b", "quoted\"value", "plain"}),
        "command argument splitter should preserve empty quoted arguments and triple-quote escapes");
}

}  // namespace

int main(int argc, char* argv[])
{
    if (qgetenv("QT_QPA_PLATFORM").isEmpty()) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    QGuiApplication app(argc, argv);

    try {
        testCommandArgumentRoundTrip();

        const lizzie::app::EngineUiSettings settings;
        require(settings.appearance.theme == lizzie::app::ThemeMode::System, "theme should default to system");
        require(
            settings.appearance.language == lizzie::app::LanguageMode::English,
            "language should default to English");
        require(settings.boardDisplay.showOwnership, "ownership should default to visible");
        require(
            settings.boardDisplay.ownershipDisplayMode == lizzie::app::OwnershipDisplayMode::MainBoard,
            "ownership should default to the main board");
        require(settings.fileBehavior.importLegacySgfAnalysis, "legacy SGF analysis import should default on");
        require(settings.fileBehavior.loadAnalysisSidecar, "analysis sidecar loading should default on");
        require(
            settings.fileBehavior.writeAnalysisSidecarAfterBatch,
            "batch sidecar writing should default on");
        require(settings.shortcuts.analyze == QKeySequence(Qt::Key_F5), "Analyze shortcut default should be F5");
        require(settings.shortcuts.batchAnalyze == QKeySequence("Ctrl+B"), "Batch shortcut default should be Ctrl+B");
        require(
            settings.shortcuts.settings == QKeySequence(QKeySequence::Preferences),
            "Settings shortcut should use the platform preference sequence");
        require(!settings.config.hasGtpMinimum(), "empty app settings should not invent a GTP config");
        require(!settings.config.hasAnalysisMinimum(), "empty app settings should not invent an analysis config");

        QTemporaryDir dir;
        require(dir.isValid(), "temporary settings directory should be valid");
        QSettings storedSettings(dir.filePath("settings.ini"), QSettings::IniFormat);
        lizzie::app::EngineUiSettings stored;
        stored.config.executable = "/opt/katago";
        stored.config.model = "/models/main.bin.gz";
        stored.config.gtpConfig = "/configs/main-gtp.cfg";
        stored.config.analysisConfig = "/configs/main-analysis.cfg";
        stored.config.workingDirectory = "/work/main";
        stored.config.libraryPaths = {"/opt/cuda/lib", "/opt/cudnn/lib"};
        stored.config.extraArgs = {"--override-config", "logDir=/tmp/main logs", R"(quoted"value\path)"};
        stored.comparisonConfig.executable = "/opt/katago-compare";
        stored.comparisonConfig.model = "/models/compare.bin.gz";
        stored.comparisonConfig.gtpConfig = "/configs/compare-gtp.cfg";
        stored.comparisonConfig.analysisConfig = "/configs/compare-analysis.cfg";
        stored.comparisonConfig.workingDirectory = "/work/compare";
        stored.comparisonConfig.libraryPaths = {"/opt/cuda-compare/lib", "/opt/cudnn-compare/lib"};
        stored.comparisonConfig.extraArgs = {"--override-config", "logDir=/tmp/compare logs"};
        stored.realtimeOptions.intervalCentiseconds = 125;
        stored.realtimeOptions.includeOwnership = false;
        stored.realtimeOptions.maxVisits = 777;
        stored.realtimeOptions.maxPlayouts = 888;
        stored.realtimeOptions.maxTimeSeconds = 9.5;
        stored.realtimeOptions.playoutDoublingAdvantage = -1.25;
        stored.realtimeOptions.analysisWideRootNoise = 0.35;
        stored.appearance.theme = lizzie::app::ThemeMode::Dark;
        stored.appearance.language = lizzie::app::LanguageMode::Chinese;
        stored.appearance.fontScale = 1.3;
        stored.boardDisplay.showOwnership = false;
        stored.boardDisplay.ownershipDisplayMode = lizzie::app::OwnershipDisplayMode::BothBoards;
        stored.boardDisplay.ownershipOpacity = 0.72;
        stored.boardDisplay.blackStoneTexture = "/textures/black stone.png";
        stored.boardDisplay.whiteStoneTexture = "/textures/white stone.png";
        stored.fileBehavior.importLegacySgfAnalysis = false;
        stored.fileBehavior.loadAnalysisSidecar = false;
        stored.fileBehavior.writeAnalysisSidecarAfterBatch = false;
        stored.shortcuts.analyze = QKeySequence("Ctrl+Alt+A");
        stored.shortcuts.passMove = QKeySequence();
        stored.shortcuts.settings = QKeySequence("Ctrl+,");

        lizzie::app::saveEngineUiSettings(storedSettings, stored);
        storedSettings.sync();

        QSettings reloadedSettings(dir.filePath("settings.ini"), QSettings::IniFormat);
        const lizzie::app::EngineUiSettings reloaded = lizzie::app::loadEngineUiSettings(reloadedSettings);
        require(reloaded.config.executable == "/opt/katago", "engine executable should persist");
        require(reloaded.config.model == "/models/main.bin.gz", "engine model should persist");
        require(reloaded.config.gtpConfig == "/configs/main-gtp.cfg", "engine GTP config should persist");
        require(
            reloaded.config.analysisConfig == "/configs/main-analysis.cfg",
            "engine analysis config should persist");
        require(reloaded.config.workingDirectory == "/work/main", "engine working directory should persist");
        require(reloaded.config.libraryPaths == stored.config.libraryPaths, "engine library paths should persist");
        require(
            reloaded.config.extraArgs == stored.config.extraArgs,
            "engine extra args with quotes and spaces should round-trip");
        require(reloaded.comparisonConfig.executable == "/opt/katago-compare", "compare executable should persist");
        require(reloaded.comparisonConfig.model == "/models/compare.bin.gz", "compare model should persist");
        require(reloaded.comparisonConfig.gtpConfig == "/configs/compare-gtp.cfg", "compare GTP config should persist");
        require(
            reloaded.comparisonConfig.analysisConfig == "/configs/compare-analysis.cfg",
            "compare analysis config should persist");
        require(
            reloaded.comparisonConfig.workingDirectory == "/work/compare",
            "compare working directory should persist");
        require(
            reloaded.comparisonConfig.libraryPaths == stored.comparisonConfig.libraryPaths,
            "compare library paths should persist");
        require(
            reloaded.comparisonConfig.extraArgs == stored.comparisonConfig.extraArgs,
            "compare extra args with spaces should round-trip");
        require(reloaded.realtimeOptions.intervalCentiseconds == 125, "analysis interval should persist");
        require(!reloaded.realtimeOptions.includeOwnership, "analysis ownership flag should persist");
        require(reloaded.realtimeOptions.maxVisits == 777, "maxVisits should persist");
        require(reloaded.realtimeOptions.maxPlayouts == 888, "maxPlayouts should persist");
        require(
            reloaded.realtimeOptions.maxTimeSeconds.has_value() &&
                near(*reloaded.realtimeOptions.maxTimeSeconds, 9.5),
            "maxTime should persist");
        require(
            reloaded.realtimeOptions.playoutDoublingAdvantage.has_value() &&
                near(*reloaded.realtimeOptions.playoutDoublingAdvantage, -1.25),
            "PDA should persist");
        require(
            reloaded.realtimeOptions.analysisWideRootNoise.has_value() &&
                near(*reloaded.realtimeOptions.analysisWideRootNoise, 0.35),
            "WRN should persist");
        require(reloaded.appearance.theme == lizzie::app::ThemeMode::Dark, "theme should persist");
        require(reloaded.appearance.language == lizzie::app::LanguageMode::Chinese, "language should persist");
        require(near(reloaded.appearance.fontScale, 1.3), "font scale should persist");
        require(!reloaded.boardDisplay.showOwnership, "ownership visibility should persist");
        require(
            reloaded.boardDisplay.ownershipDisplayMode == lizzie::app::OwnershipDisplayMode::BothBoards,
            "ownership display mode should persist");
        require(near(reloaded.boardDisplay.ownershipOpacity, 0.72), "ownership opacity should persist");
        require(
            reloaded.boardDisplay.blackStoneTexture == "/textures/black stone.png",
            "black texture path should persist");
        require(
            reloaded.boardDisplay.whiteStoneTexture == "/textures/white stone.png",
            "white texture path should persist");
        require(!reloaded.fileBehavior.importLegacySgfAnalysis, "legacy import behavior should persist");
        require(!reloaded.fileBehavior.loadAnalysisSidecar, "sidecar load behavior should persist");
        require(!reloaded.fileBehavior.writeAnalysisSidecarAfterBatch, "sidecar write behavior should persist");
        require(reloaded.shortcuts.analyze == QKeySequence("Ctrl+Alt+A"), "custom shortcut should persist");
        require(reloaded.shortcuts.passMove.isEmpty(), "disabled shortcut should persist");
        require(reloaded.shortcuts.settings == QKeySequence("Ctrl+,"), "settings shortcut should persist");

        QSettings whitespaceSettings(dir.filePath("whitespace.ini"), QSettings::IniFormat);
        whitespaceSettings.setValue("engine/executable", " \t ");
        whitespaceSettings.setValue("engine/model", "/models/main.bin.gz");
        whitespaceSettings.setValue("engine/gtpConfig", "/configs/main-gtp.cfg");
        whitespaceSettings.setValue("engine/analysisConfig", "/configs/main-analysis.cfg");
        whitespaceSettings.setValue("compare/executable", "/opt/katago-compare");
        whitespaceSettings.setValue("compare/model", " \n ");
        whitespaceSettings.setValue("compare/gtpConfig", "/configs/compare-gtp.cfg");
        whitespaceSettings.setValue("compare/analysisConfig", "/configs/compare-analysis.cfg");
        whitespaceSettings.sync();

        QSettings whitespaceReload(dir.filePath("whitespace.ini"), QSettings::IniFormat);
        const lizzie::app::EngineUiSettings whitespaceLoaded =
            lizzie::app::loadEngineUiSettings(whitespaceReload);
        require(
            !whitespaceLoaded.config.hasGtpMinimum(),
            "whitespace-only stored executable should not complete GTP settings");
        require(
            !whitespaceLoaded.config.hasAnalysisMinimum(),
            "whitespace-only stored executable should not complete analysis settings");
        require(
            !whitespaceLoaded.comparisonConfig.hasGtpMinimum(),
            "whitespace-only stored compare model should not complete compare GTP settings");
        require(
            !whitespaceLoaded.comparisonConfig.hasAnalysisMinimum(),
            "whitespace-only stored compare model should not complete compare analysis settings");

        QSettings stringBoolSettings(dir.filePath("string-bool.ini"), QSettings::IniFormat);
        stringBoolSettings.setValue("analysis/includeOwnership", "false");
        stringBoolSettings.setValue("board/showOwnership", "0");
        stringBoolSettings.setValue("files/importLegacySgfAnalysis", "false");
        stringBoolSettings.setValue("files/loadAnalysisSidecar", "0");
        stringBoolSettings.setValue("files/writeAnalysisSidecarAfterBatch", "true");
        stringBoolSettings.sync();

        QSettings stringBoolReload(dir.filePath("string-bool.ini"), QSettings::IniFormat);
        const lizzie::app::EngineUiSettings stringBoolLoaded =
            lizzie::app::loadEngineUiSettings(stringBoolReload);
        require(!stringBoolLoaded.realtimeOptions.includeOwnership, "string false ownership setting should load");
        require(!stringBoolLoaded.boardDisplay.showOwnership, "string zero board ownership setting should load");
        require(
            !stringBoolLoaded.fileBehavior.importLegacySgfAnalysis,
            "string false legacy import setting should load");
        require(!stringBoolLoaded.fileBehavior.loadAnalysisSidecar, "string zero sidecar load setting should load");
        require(
            stringBoolLoaded.fileBehavior.writeAnalysisSidecarAfterBatch,
            "string true sidecar write setting should load");

        QSettings malformedSettings(dir.filePath("malformed.ini"), QSettings::IniFormat);
        malformedSettings.setValue("analysis/intervalCentiseconds", 0);
        malformedSettings.setValue("analysis/includeOwnership", "not-a-bool");
        malformedSettings.setValue("analysis/maxVisits", 200000000);
        malformedSettings.setValue("analysis/maxPlayouts", -10);
        malformedSettings.setValue("analysis/maxTimeSeconds", 999999.0);
        malformedSettings.setValue("analysis/playoutDoublingAdvantage", -25.0);
        malformedSettings.setValue("analysis/analysisWideRootNoise", 99.0);
        malformedSettings.setValue("appearance/theme", "solarized");
        malformedSettings.setValue("appearance/language", "de");
        malformedSettings.setValue("appearance/fontScale", 0.1);
        malformedSettings.setValue("board/showOwnership", "maybe");
        malformedSettings.setValue("board/ownershipDisplayMode", "floating");
        malformedSettings.setValue("board/ownershipOpacity", 4.0);
        malformedSettings.setValue("files/importLegacySgfAnalysis", "maybe");
        malformedSettings.setValue("files/loadAnalysisSidecar", "not-a-bool");
        malformedSettings.setValue("files/writeAnalysisSidecarAfterBatch", "enabled");
        malformedSettings.sync();

        QSettings malformedReload(dir.filePath("malformed.ini"), QSettings::IniFormat);
        const lizzie::app::EngineUiSettings normalized = lizzie::app::loadEngineUiSettings(malformedReload);
        require(normalized.realtimeOptions.intervalCentiseconds == 1, "invalid interval should clamp to dialog minimum");
        require(normalized.realtimeOptions.includeOwnership, "invalid ownership bool should fall back to enabled");
        require(normalized.realtimeOptions.maxVisits == 100000000, "max visits should clamp to dialog maximum");
        require(!normalized.realtimeOptions.maxPlayouts.has_value(), "negative max playouts should clear the limit");
        require(
            normalized.realtimeOptions.maxTimeSeconds.has_value() &&
                near(*normalized.realtimeOptions.maxTimeSeconds, 86400.0),
            "max time should clamp to dialog maximum");
        require(
            normalized.realtimeOptions.playoutDoublingAdvantage.has_value() &&
                near(*normalized.realtimeOptions.playoutDoublingAdvantage, -10.0),
            "PDA should clamp to dialog minimum");
        require(
            normalized.realtimeOptions.analysisWideRootNoise.has_value() &&
                near(*normalized.realtimeOptions.analysisWideRootNoise, 10.0),
            "WRN should clamp to dialog maximum");
        require(normalized.appearance.theme == lizzie::app::ThemeMode::System, "invalid theme should fall back to system");
        require(
            normalized.appearance.language == lizzie::app::LanguageMode::English,
            "invalid language should fall back to English");
        require(near(normalized.appearance.fontScale, 0.75), "font scale should clamp to dialog minimum");
        require(normalized.boardDisplay.showOwnership, "invalid board ownership bool should fall back to enabled");
        require(
            normalized.boardDisplay.ownershipDisplayMode == lizzie::app::OwnershipDisplayMode::MainBoard,
            "invalid ownership display mode should fall back to main board");
        require(near(normalized.boardDisplay.ownershipOpacity, 1.0), "ownership opacity should clamp to dialog maximum");
        require(
            normalized.fileBehavior.importLegacySgfAnalysis,
            "invalid legacy import bool should fall back to enabled");
        require(normalized.fileBehavior.loadAnalysisSidecar, "invalid sidecar load bool should fall back to enabled");
        require(
            normalized.fileBehavior.writeAnalysisSidecarAfterBatch,
            "invalid sidecar write bool should fall back to enabled");

        const double infinity = std::numeric_limits<double>::infinity();
        const double quietNaN = std::numeric_limits<double>::quiet_NaN();

        QSettings nonFiniteSettings(dir.filePath("nonfinite.ini"), QSettings::IniFormat);
        nonFiniteSettings.setValue("analysis/maxTimeSeconds", infinity);
        nonFiniteSettings.setValue("analysis/playoutDoublingAdvantage", quietNaN);
        nonFiniteSettings.setValue("analysis/analysisWideRootNoise", -infinity);
        nonFiniteSettings.setValue("appearance/fontScale", infinity);
        nonFiniteSettings.setValue("board/ownershipOpacity", quietNaN);
        nonFiniteSettings.sync();

        QSettings nonFiniteReload(dir.filePath("nonfinite.ini"), QSettings::IniFormat);
        const lizzie::app::EngineUiSettings nonFiniteNormalized =
            lizzie::app::loadEngineUiSettings(nonFiniteReload);
        require(
            !nonFiniteNormalized.realtimeOptions.maxTimeSeconds.has_value(),
            "non-finite stored max time should clear the limit");
        require(
            !nonFiniteNormalized.realtimeOptions.playoutDoublingAdvantage.has_value(),
            "non-finite stored PDA should clear the setting");
        require(
            !nonFiniteNormalized.realtimeOptions.analysisWideRootNoise.has_value(),
            "non-finite stored WRN should clear the setting");
        require(near(nonFiniteNormalized.appearance.fontScale, 1.0), "non-finite font scale should use the default");
        require(
            near(nonFiniteNormalized.boardDisplay.ownershipOpacity, 0.45),
            "non-finite ownership opacity should use the default");

        QSettings noisySaveSettings(dir.filePath("noisy-save.ini"), QSettings::IniFormat);
        lizzie::app::EngineUiSettings noisySave;
        noisySave.realtimeOptions.intervalCentiseconds = -20;
        noisySave.realtimeOptions.maxVisits = -5;
        noisySave.realtimeOptions.maxPlayouts = 200000000;
        noisySave.realtimeOptions.maxTimeSeconds = infinity;
        noisySave.realtimeOptions.playoutDoublingAdvantage = quietNaN;
        noisySave.realtimeOptions.analysisWideRootNoise = 99.0;
        noisySave.appearance.fontScale = -infinity;
        noisySave.boardDisplay.ownershipOpacity = quietNaN;
        lizzie::app::saveEngineUiSettings(noisySaveSettings, noisySave);
        noisySaveSettings.sync();

        QSettings noisySaveReload(dir.filePath("noisy-save.ini"), QSettings::IniFormat);
        require(
            noisySaveReload.value("analysis/intervalCentiseconds").toInt() == 1,
            "saved interval should clamp to the minimum");
        require(
            noisySaveReload.value("analysis/maxVisits").toInt() == 0,
            "saved nonpositive max visits should become disabled");
        require(
            noisySaveReload.value("analysis/maxPlayouts").toInt() == 100000000,
            "saved oversized max playouts should clamp to the maximum");
        require(
            near(noisySaveReload.value("analysis/maxTimeSeconds").toDouble(), 0.0),
            "saved non-finite max time should become disabled");
        require(
            near(noisySaveReload.value("analysis/playoutDoublingAdvantage").toDouble(), 0.0),
            "saved non-finite PDA should become disabled");
        require(
            near(noisySaveReload.value("analysis/analysisWideRootNoise").toDouble(), 10.0),
            "saved oversized WRN should clamp to the maximum");
        require(
            near(noisySaveReload.value("appearance/fontScale").toDouble(), 1.0),
            "saved non-finite font scale should use the default");
        require(
            near(noisySaveReload.value("board/ownershipOpacity").toDouble(), 0.45),
            "saved non-finite ownership opacity should use the default");
    } catch (const std::exception& error) {
        std::cerr << "app_settings_model_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "app_settings_model_tests passed\n";
    return EXIT_SUCCESS;
}
