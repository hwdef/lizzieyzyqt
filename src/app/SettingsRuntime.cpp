#include "SettingsRuntime.h"

namespace lizzie::app {

namespace {

bool sameGtpRuntimeConfig(const lizzie::engine::KataGoConfig& left, const lizzie::engine::KataGoConfig& right)
{
    return left.executable == right.executable && left.model == right.model && left.gtpConfig == right.gtpConfig &&
        left.workingDirectory == right.workingDirectory && left.extraArgs == right.extraArgs;
}

template <typename T>
bool optionalCleared(const std::optional<T>& previous, const std::optional<T>& next)
{
    return previous.has_value() && !next.has_value();
}

bool searchParameterCleared(
    const lizzie::engine::RealtimeAnalysisOptions& previous,
    const lizzie::engine::RealtimeAnalysisOptions& next)
{
    return optionalCleared(previous.maxVisits, next.maxVisits) ||
        optionalCleared(previous.maxPlayouts, next.maxPlayouts) ||
        optionalCleared(previous.maxTimeSeconds, next.maxTimeSeconds) ||
        optionalCleared(previous.playoutDoublingAdvantage, next.playoutDoublingAdvantage) ||
        optionalCleared(previous.analysisWideRootNoise, next.analysisWideRootNoise);
}

bool sameRealtimeOptions(
    const lizzie::engine::RealtimeAnalysisOptions& left,
    const lizzie::engine::RealtimeAnalysisOptions& right)
{
    return left.intervalCentiseconds == right.intervalCentiseconds &&
        left.includeOwnership == right.includeOwnership &&
        left.maxVisits == right.maxVisits &&
        left.maxPlayouts == right.maxPlayouts &&
        left.maxTimeSeconds == right.maxTimeSeconds &&
        left.playoutDoublingAdvantage == right.playoutDoublingAdvantage &&
        left.analysisWideRootNoise == right.analysisWideRootNoise;
}

}  // namespace

lizzie::engine::KataGoConfig resolvedComparisonGtpConfig(const EngineUiSettings& settings)
{
    lizzie::engine::KataGoConfig config = settings.comparisonConfig;
    if (config.executable.empty()) {
        config.executable = settings.config.executable;
    }
    if (config.model.empty()) {
        config.model = settings.config.model;
    }
    if (config.gtpConfig.empty()) {
        config.gtpConfig = settings.config.gtpConfig;
    }
    if (config.workingDirectory.empty()) {
        config.workingDirectory = settings.config.workingDirectory;
    }
    if (config.extraArgs.empty()) {
        config.extraArgs = settings.config.extraArgs;
    }
    return config;
}

SettingsRuntimePlan planSettingsRuntimeUpdate(
    const EngineUiSettings& previous,
    const EngineUiSettings& next,
    SettingsRuntimeState state)
{
    SettingsRuntimePlan plan;
    const bool mainGtpRuntimeConfigChanged = !sameGtpRuntimeConfig(previous.config, next.config);
    const bool realtimeSearchParametersCleared = searchParameterCleared(previous.realtimeOptions, next.realtimeOptions);
    if (state.mainEngineRunning && (mainGtpRuntimeConfigChanged || realtimeSearchParametersCleared)) {
        plan.restartMainEngine = true;
    }

    if (state.realtimeRequested) {
        if (!next.config.hasGtpMinimum()) {
            plan.disableRealtime = true;
        } else if (mainGtpRuntimeConfigChanged) {
            plan.startMainEngine = true;
        } else if (state.mainEngineRunning && realtimeSearchParametersCleared) {
            plan.startMainEngine = true;
        } else if (state.mainEngineRunning && !sameRealtimeOptions(previous.realtimeOptions, next.realtimeOptions)) {
            plan.resyncRealtime = true;
        } else if (!state.mainEngineRunning) {
            plan.startMainEngine = true;
        }
    }

    const lizzie::engine::KataGoConfig previousCompare = resolvedComparisonGtpConfig(previous);
    const lizzie::engine::KataGoConfig nextCompare = resolvedComparisonGtpConfig(next);
    const bool compareGtpRuntimeConfigChanged = !sameGtpRuntimeConfig(previousCompare, nextCompare);
    if (state.compareEngineRunning && (compareGtpRuntimeConfigChanged || realtimeSearchParametersCleared)) {
        plan.restartCompareEngine = true;
    }

    if (state.compareRequested) {
        if (!nextCompare.hasGtpMinimum()) {
            plan.disableCompare = true;
        } else if (compareGtpRuntimeConfigChanged) {
            plan.startCompareEngine = true;
        } else if (state.compareEngineRunning && realtimeSearchParametersCleared) {
            plan.startCompareEngine = true;
        } else if (state.compareEngineRunning && !sameRealtimeOptions(previous.realtimeOptions, next.realtimeOptions)) {
            plan.resyncCompare = true;
        } else if (!state.compareEngineRunning) {
            plan.startCompareEngine = true;
        }
    }
    return plan;
}

}  // namespace lizzie::app
