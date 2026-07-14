#pragma once

#include "Types.h"

namespace lizzie::app {

struct EngineAutomationState {
    bool selfPlayActive = false;
    bool engineGameActive = false;
    bool humanVsAiActive = false;
    bool previousMoveWasPass = false;
};

struct EngineAutomationPlan {
    bool stopHumanVsAi = false;
    bool stopSelfPlay = false;
    bool stopEngineGame = false;
    bool requestSelfPlayMove = false;
    bool requestEngineGameMove = false;
};

struct HumanMoveAutomationPlan {
    bool stopHumanVsAi = false;
    bool requestHumanVsAiReply = false;
};

struct AutomatedPlayTogglePlan {
    bool stopHumanVsAi = false;
    bool stopSelfPlay = false;
    bool stopEngineGame = false;
    bool stopRealtimeAnalysis = false;
    bool stopCompareAnalysis = false;
    bool clearCompareMoveRequest = false;
    bool startHumanVsAi = false;
    bool startSelfPlay = false;
    bool startEngineGame = false;
    bool requestSelfPlayMove = false;
    bool requestEngineGameMove = false;
};

[[nodiscard]] EngineAutomationPlan planAfterEngineMove(
    const lizzie::core::Move& move,
    EngineAutomationState state);
[[nodiscard]] HumanMoveAutomationPlan planAfterHumanMove(
    const lizzie::core::Move& move,
    EngineAutomationState state);

[[nodiscard]] AutomatedPlayTogglePlan planToggleHumanVsAi(EngineAutomationState state);
[[nodiscard]] AutomatedPlayTogglePlan planToggleSelfPlay(EngineAutomationState state);
[[nodiscard]] AutomatedPlayTogglePlan planToggleEngineGame(EngineAutomationState state);

}  // namespace lizzie::app
