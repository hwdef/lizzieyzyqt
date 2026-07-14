#include "SettingsRuntime.h"

#include <QGuiApplication>
#include <QByteArray>
#include <QKeySequence>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

lizzie::app::EngineUiSettings baseSettings()
{
    lizzie::app::EngineUiSettings settings;
    settings.config.executable = "/opt/katago";
    settings.config.model = "/models/main.bin.gz";
    settings.config.gtpConfig = "/configs/main-gtp.cfg";
    settings.config.analysisConfig = "/configs/main-analysis.cfg";
    settings.config.workingDirectory = "/work/main";
    settings.config.extraArgs = {"--override-config", "logDir=/tmp/main"};
    settings.comparisonConfig.gtpConfig = "/configs/compare-gtp.cfg";
    settings.comparisonConfig.analysisConfig = "/configs/compare-analysis.cfg";
    return settings;
}

void testInactiveSettingsDoNotTouchEngines()
{
    const lizzie::app::EngineUiSettings previous = baseSettings();
    lizzie::app::EngineUiSettings next = previous;
    next.config.executable = "/opt/new-katago";

    const lizzie::app::SettingsRuntimePlan plan =
        lizzie::app::planSettingsRuntimeUpdate(previous, next, lizzie::app::SettingsRuntimeState{});
    require(!plan.startMainEngine, "inactive realtime should not start main engine");
    require(!plan.restartMainEngine, "inactive realtime should not restart main engine");
    require(!plan.resyncRealtime, "inactive realtime should not resync");
}

void testIdleRunningMainConfigChangeStopsWithoutStartingAnalysis()
{
    const lizzie::app::EngineUiSettings previous = baseSettings();
    lizzie::app::EngineUiSettings next = previous;
    next.config.model = "/models/new-main.bin.gz";

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.mainEngineRunning = true});
    require(plan.restartMainEngine, "idle running main engine should stop after a GTP config change");
    require(!plan.startMainEngine, "idle running main engine should not start analysis after a GTP config change");
    require(!plan.resyncRealtime, "idle running main engine should not resync realtime analysis");
}

void testIdleRunningMainSearchLimitClearStopsWithoutStartingAnalysis()
{
    lizzie::app::EngineUiSettings previous = baseSettings();
    previous.realtimeOptions.maxVisits = 512;
    lizzie::app::EngineUiSettings next = previous;
    next.realtimeOptions.maxVisits.reset();

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.mainEngineRunning = true});
    require(plan.restartMainEngine, "idle running main engine should stop when a stale search limit is cleared");
    require(!plan.startMainEngine, "idle running main engine should not start analysis after clearing a search limit");
    require(!plan.resyncRealtime, "idle running main engine should not resync after clearing a search limit");
}

void testRealtimeOptionChangeResyncsRunningEngine()
{
    const lizzie::app::EngineUiSettings previous = baseSettings();
    lizzie::app::EngineUiSettings next = previous;
    next.realtimeOptions.maxVisits = 512;

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.realtimeRequested = true, .mainEngineRunning = true});
    require(plan.resyncRealtime, "analysis option change should resync running realtime analysis");
    require(!plan.restartMainEngine, "analysis option change should not restart unchanged GTP engine");
    require(!plan.startMainEngine, "analysis option change should not start an already running engine");
}

void testClearingRealtimeSearchLimitRestartsRunningEngine()
{
    lizzie::app::EngineUiSettings previous = baseSettings();
    previous.realtimeOptions.maxVisits = 512;
    lizzie::app::EngineUiSettings next = previous;
    next.realtimeOptions.maxVisits.reset();

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.realtimeRequested = true, .mainEngineRunning = true});
    require(plan.restartMainEngine, "clearing a realtime search limit should restart the running main engine");
    require(plan.startMainEngine, "clearing a realtime search limit should start the main engine again");
    require(!plan.resyncRealtime, "clearing a realtime search limit should not reuse stale engine params");
}

void testClearingRealtimeWideRootNoiseRestartsRunningEngine()
{
    lizzie::app::EngineUiSettings previous = baseSettings();
    previous.realtimeOptions.analysisWideRootNoise = 0.15;
    lizzie::app::EngineUiSettings next = previous;
    next.realtimeOptions.analysisWideRootNoise.reset();

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.realtimeRequested = true, .mainEngineRunning = true});
    require(plan.restartMainEngine, "clearing WRN should restart the running main engine");
    require(plan.startMainEngine, "clearing WRN should start the main engine again");
    require(!plan.resyncRealtime, "clearing WRN should not reuse stale engine search params");
}

void testGtpRuntimeConfigChangeRestartsRunningEngine()
{
    const lizzie::app::EngineUiSettings previous = baseSettings();
    lizzie::app::EngineUiSettings next = previous;
    next.config.gtpConfig = "/configs/new-main-gtp.cfg";

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.realtimeRequested = true, .mainEngineRunning = true});
    require(plan.restartMainEngine, "GTP config change should stop the running main engine");
    require(plan.startMainEngine, "GTP config change should start the main engine again");
    require(!plan.resyncRealtime, "GTP config change should not reuse old running process");
}

void testAnalysisOnlyConfigChangeDoesNotTouchRunningGtpAnalysis()
{
    const lizzie::app::EngineUiSettings previous = baseSettings();
    lizzie::app::EngineUiSettings next = previous;
    next.config.analysisConfig = "/configs/new-analysis.cfg";

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.realtimeRequested = true, .mainEngineRunning = true});
    require(!plan.resyncRealtime, "analysis-only config change should not resync running GTP analysis");
    require(!plan.restartMainEngine, "analysis-only config change should not restart GTP engine");
    require(!plan.startMainEngine, "analysis-only config change should not start a new GTP engine");
}

void testCompareAnalysisOnlyConfigChangeDoesNotTouchRunningGtpCompare()
{
    const lizzie::app::EngineUiSettings previous = baseSettings();
    lizzie::app::EngineUiSettings next = previous;
    next.comparisonConfig.analysisConfig = "/configs/new-compare-analysis.cfg";

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.compareRequested = true, .compareEngineRunning = true});
    require(!plan.resyncCompare, "compare analysis-only config change should not resync running GTP compare");
    require(!plan.restartCompareEngine, "compare analysis-only config change should not restart GTP compare engine");
    require(!plan.startCompareEngine, "compare analysis-only config change should not start a new GTP compare engine");
}

void testUiOnlySettingsDoNotResyncRunningRealtime()
{
    const lizzie::app::EngineUiSettings previous = baseSettings();
    lizzie::app::EngineUiSettings next = previous;
    next.appearance.theme = lizzie::app::ThemeMode::Dark;
    next.appearance.language = lizzie::app::LanguageMode::Chinese;
    next.appearance.fontScale = 1.25;
    next.boardDisplay.showOwnership = false;
    next.boardDisplay.ownershipOpacity = 0.7;
    next.fileBehavior.loadAnalysisSidecar = false;
    next.shortcuts.analyze = QKeySequence("Ctrl+Alt+A");

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.realtimeRequested = true, .mainEngineRunning = true});
    require(!plan.disableRealtime, "UI-only settings should not disable realtime analysis");
    require(!plan.restartMainEngine, "UI-only settings should not restart the running main engine");
    require(!plan.startMainEngine, "UI-only settings should not start a replacement main engine");
    require(!plan.resyncRealtime, "UI-only settings should not replay the board to main KataGo");
}

void testUiOnlySettingsDoNotResyncRunningCompare()
{
    const lizzie::app::EngineUiSettings previous = baseSettings();
    lizzie::app::EngineUiSettings next = previous;
    next.appearance.theme = lizzie::app::ThemeMode::Light;
    next.boardDisplay.ownershipDisplayMode = lizzie::app::OwnershipDisplayMode::BothBoards;
    next.fileBehavior.writeAnalysisSidecarAfterBatch = false;
    next.shortcuts.compare = QKeySequence("Ctrl+Alt+D");

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.compareRequested = true, .compareEngineRunning = true});
    require(!plan.disableCompare, "UI-only settings should not disable compare analysis");
    require(!plan.restartCompareEngine, "UI-only settings should not restart the running compare engine");
    require(!plan.startCompareEngine, "UI-only settings should not start a replacement compare engine");
    require(!plan.resyncCompare, "UI-only settings should not replay the board to compare KataGo");
}

void testIncompleteRealtimeSettingsDisableAnalysis()
{
    const lizzie::app::EngineUiSettings previous = baseSettings();
    lizzie::app::EngineUiSettings next = previous;
    next.config.gtpConfig.clear();

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.realtimeRequested = true, .mainEngineRunning = true});
    require(plan.disableRealtime, "missing GTP minimum should disable realtime analysis");
    require(!plan.startMainEngine, "missing GTP minimum should not start main engine");
}

void testCompareFallbackTracksMainRuntimeConfig()
{
    const lizzie::app::EngineUiSettings previous = baseSettings();
    lizzie::app::EngineUiSettings next = previous;
    next.config.executable = "/opt/new-katago";

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.compareRequested = true, .compareEngineRunning = true});
    require(plan.restartCompareEngine, "compare engine should restart when fallback executable changes");
    require(plan.startCompareEngine, "compare engine should start again after fallback executable change");
    require(!plan.resyncCompare, "compare engine should not resync against stale fallback process");
}

void testIdleRunningCompareConfigChangeStopsWithoutStartingAnalysis()
{
    const lizzie::app::EngineUiSettings previous = baseSettings();
    lizzie::app::EngineUiSettings next = previous;
    next.comparisonConfig.gtpConfig = "/configs/new-compare-gtp.cfg";

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.compareEngineRunning = true});
    require(plan.restartCompareEngine, "idle running compare engine should stop after a GTP config change");
    require(!plan.startCompareEngine, "idle running compare engine should not start analysis after a GTP config change");
    require(!plan.resyncCompare, "idle running compare engine should not resync compare analysis");
}

void testClearingCompareSearchLimitRestartsRunningEngine()
{
    lizzie::app::EngineUiSettings previous = baseSettings();
    previous.realtimeOptions.maxPlayouts = 2048;
    lizzie::app::EngineUiSettings next = previous;
    next.realtimeOptions.maxPlayouts.reset();

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.compareRequested = true, .compareEngineRunning = true});
    require(plan.restartCompareEngine, "clearing a search limit should restart the running compare engine");
    require(plan.startCompareEngine, "clearing a search limit should start the compare engine again");
    require(!plan.resyncCompare, "clearing a search limit should not reuse stale compare engine params");
}

void testClearingCompareWideRootNoiseRestartsRunningEngine()
{
    lizzie::app::EngineUiSettings previous = baseSettings();
    previous.realtimeOptions.analysisWideRootNoise = 0.15;
    lizzie::app::EngineUiSettings next = previous;
    next.realtimeOptions.analysisWideRootNoise.reset();

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.compareRequested = true, .compareEngineRunning = true});
    require(plan.restartCompareEngine, "clearing WRN should restart the running compare engine");
    require(plan.startCompareEngine, "clearing WRN should start the compare engine again");
    require(!plan.resyncCompare, "clearing WRN should not reuse stale compare engine search params");
}

void testCompareFallbackCanReuseMainGtpConfig()
{
    lizzie::app::EngineUiSettings settings = baseSettings();
    settings.comparisonConfig.gtpConfig.clear();
    settings.comparisonConfig.model = "/models/compare.bin.gz";

    const lizzie::engine::KataGoConfig resolved = lizzie::app::resolvedComparisonGtpConfig(settings);
    require(resolved.hasGtpMinimum(), "compare fallback should be complete when only compare model is set");
    require(resolved.model == "/models/compare.bin.gz", "compare fallback should keep explicit compare model");
    require(resolved.gtpConfig == "/configs/main-gtp.cfg", "compare fallback should reuse main GTP config");
}

void testCompareFallbackCanReuseMainExtraArgs()
{
    lizzie::app::EngineUiSettings settings = baseSettings();
    settings.comparisonConfig.extraArgs.clear();

    const lizzie::engine::KataGoConfig resolved = lizzie::app::resolvedComparisonGtpConfig(settings);
    require(resolved.extraArgs == settings.config.extraArgs, "compare fallback should reuse main extra args");
}

void testCompareFallbackKeepsExplicitExtraArgs()
{
    lizzie::app::EngineUiSettings settings = baseSettings();
    settings.comparisonConfig.extraArgs = {"--override-config", "logDir=/tmp/compare"};

    const lizzie::engine::KataGoConfig resolved = lizzie::app::resolvedComparisonGtpConfig(settings);
    require(
        resolved.extraArgs == settings.comparisonConfig.extraArgs,
        "compare fallback should keep explicit compare extra args");
}

void testCompareFallbackTracksMainGtpConfig()
{
    lizzie::app::EngineUiSettings previous = baseSettings();
    previous.comparisonConfig.gtpConfig.clear();
    lizzie::app::EngineUiSettings next = previous;
    next.config.gtpConfig = "/configs/new-main-gtp.cfg";

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.compareRequested = true, .compareEngineRunning = true});
    require(plan.restartCompareEngine, "compare engine should restart when fallback GTP config changes");
    require(plan.startCompareEngine, "compare engine should start again after fallback GTP config change");
    require(!plan.resyncCompare, "compare engine should not resync after fallback GTP config change");
}

void testCompareFallbackTracksMainExtraArgs()
{
    lizzie::app::EngineUiSettings previous = baseSettings();
    previous.comparisonConfig.extraArgs.clear();
    lizzie::app::EngineUiSettings next = previous;
    next.config.extraArgs = {"--override-config", "logDir=/tmp/new-main"};

    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        previous,
        next,
        lizzie::app::SettingsRuntimeState{.compareRequested = true, .compareEngineRunning = true});
    require(plan.restartCompareEngine, "compare engine should restart when fallback extra args change");
    require(plan.startCompareEngine, "compare engine should start again after fallback extra args change");
    require(!plan.resyncCompare, "compare engine should not resync after fallback extra args change");
}

void testPendingCompareStartsWhenEngineIsNotRunning()
{
    const lizzie::app::EngineUiSettings settings = baseSettings();
    const lizzie::app::SettingsRuntimePlan plan = lizzie::app::planSettingsRuntimeUpdate(
        settings,
        settings,
        lizzie::app::SettingsRuntimeState{.compareRequested = true, .compareEngineRunning = false});
    require(plan.startCompareEngine, "pending compare analysis should start engine when it is not running");
    require(!plan.restartCompareEngine, "pending compare analysis should not restart a stopped engine");
}

}  // namespace

int main(int argc, char* argv[])
{
    if (qgetenv("QT_QPA_PLATFORM").isEmpty()) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    QGuiApplication app(argc, argv);

    try {
        testInactiveSettingsDoNotTouchEngines();
        testIdleRunningMainConfigChangeStopsWithoutStartingAnalysis();
        testIdleRunningMainSearchLimitClearStopsWithoutStartingAnalysis();
        testRealtimeOptionChangeResyncsRunningEngine();
        testClearingRealtimeSearchLimitRestartsRunningEngine();
        testClearingRealtimeWideRootNoiseRestartsRunningEngine();
        testGtpRuntimeConfigChangeRestartsRunningEngine();
        testAnalysisOnlyConfigChangeDoesNotTouchRunningGtpAnalysis();
        testCompareAnalysisOnlyConfigChangeDoesNotTouchRunningGtpCompare();
        testUiOnlySettingsDoNotResyncRunningRealtime();
        testUiOnlySettingsDoNotResyncRunningCompare();
        testIncompleteRealtimeSettingsDisableAnalysis();
        testCompareFallbackTracksMainRuntimeConfig();
        testIdleRunningCompareConfigChangeStopsWithoutStartingAnalysis();
        testClearingCompareSearchLimitRestartsRunningEngine();
        testClearingCompareWideRootNoiseRestartsRunningEngine();
        testCompareFallbackCanReuseMainGtpConfig();
        testCompareFallbackCanReuseMainExtraArgs();
        testCompareFallbackKeepsExplicitExtraArgs();
        testCompareFallbackTracksMainGtpConfig();
        testCompareFallbackTracksMainExtraArgs();
        testPendingCompareStartsWhenEngineIsNotRunning();
    } catch (const std::exception& error) {
        std::cerr << "settings_apply_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "settings_apply_tests passed\n";
    return EXIT_SUCCESS;
}
