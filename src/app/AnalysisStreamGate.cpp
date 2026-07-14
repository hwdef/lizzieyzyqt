#include "AnalysisStreamGate.h"

namespace lizzie::app {

void AnalysisStreamGate::beginSync()
{
    pendingCommandIds_.clear();
    pending_ = true;
    active_ = false;
}

void AnalysisStreamGate::recordSyncCommand(int commandId)
{
    if (commandId > 0) {
        pendingCommandIds_.insert(commandId);
    }
}

AnalysisStreamGateEvent AnalysisStreamGate::handleSyncResponse(int commandId, bool success)
{
    if (!pending_ || pendingCommandIds_.erase(commandId) == 0) {
        return AnalysisStreamGateEvent::Ignored;
    }

    if (!success) {
        stop();
        return AnalysisStreamGateEvent::Failed;
    }

    if (pendingCommandIds_.empty()) {
        pending_ = false;
        active_ = true;
        return AnalysisStreamGateEvent::Activated;
    }
    return AnalysisStreamGateEvent::Waiting;
}

void AnalysisStreamGate::stop()
{
    pendingCommandIds_.clear();
    pending_ = false;
    active_ = false;
}

bool AnalysisStreamGate::isPending() const
{
    return pending_;
}

bool AnalysisStreamGate::isActive() const
{
    return active_;
}

bool AnalysisStreamGate::acceptsAnalysis() const
{
    return active_;
}

}  // namespace lizzie::app
