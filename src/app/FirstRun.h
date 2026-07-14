#pragma once

#include "AppSettings.h"

namespace lizzie::app {

enum class FirstRunStartupAction {
    Skip,
    MarkComplete,
    ShowWizard,
};

[[nodiscard]] FirstRunStartupAction planFirstRunStartup(
    bool firstRunAlreadyComplete,
    const EngineUiSettings& settings);

}  // namespace lizzie::app
