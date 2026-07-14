#pragma once

#include <unordered_set>

namespace lizzie::app {

enum class AnalysisStreamGateEvent {
    Ignored,
    Waiting,
    Activated,
    Failed,
};

class AnalysisStreamGate {
public:
    void beginSync();
    void recordSyncCommand(int commandId);
    [[nodiscard]] AnalysisStreamGateEvent handleSyncResponse(int commandId, bool success);
    void stop();

    [[nodiscard]] bool isPending() const;
    [[nodiscard]] bool isActive() const;
    [[nodiscard]] bool acceptsAnalysis() const;

private:
    std::unordered_set<int> pendingCommandIds_;
    bool pending_ = false;
    bool active_ = false;
};

}  // namespace lizzie::app
