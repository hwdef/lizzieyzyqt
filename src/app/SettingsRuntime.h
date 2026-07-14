#pragma once

#include "AppSettings.h"

namespace lizzie::app {

struct SettingsRuntimeState {
    bool realtimeRequested = false;
    bool compareRequested = false;
    bool mainEngineRunning = false;
    bool compareEngineRunning = false;
};

struct SettingsRuntimePlan {
    bool disableRealtime = false;
    bool restartMainEngine = false;
    bool startMainEngine = false;
    bool resyncRealtime = false;
    bool disableCompare = false;
    bool restartCompareEngine = false;
    bool startCompareEngine = false;
    bool resyncCompare = false;
};

[[nodiscard]] lizzie::engine::KataGoConfig resolvedComparisonGtpConfig(const EngineUiSettings& settings);

[[nodiscard]] SettingsRuntimePlan planSettingsRuntimeUpdate(
    const EngineUiSettings& previous,
    const EngineUiSettings& next,
    SettingsRuntimeState state);

}  // namespace lizzie::app
