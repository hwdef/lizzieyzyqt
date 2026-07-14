#include "FirstRun.h"

namespace lizzie::app {

FirstRunStartupAction planFirstRunStartup(bool firstRunAlreadyComplete, const EngineUiSettings& settings)
{
    if (firstRunAlreadyComplete) {
        return FirstRunStartupAction::Skip;
    }
    if (settings.config.hasGtpMinimum() && settings.config.hasAnalysisMinimum()) {
        return FirstRunStartupAction::MarkComplete;
    }
    return FirstRunStartupAction::ShowWizard;
}

}  // namespace lizzie::app
