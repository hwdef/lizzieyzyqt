#include "EngineAutomation.h"

namespace lizzie::app {

EngineAutomationPlan planAfterEngineMove(
    const lizzie::core::Move& move,
    EngineAutomationState state)
{
    EngineAutomationPlan plan;
    const bool terminalMove = move.type == lizzie::core::MoveType::Resign ||
        (move.type == lizzie::core::MoveType::Pass && state.previousMoveWasPass);
    if (terminalMove) {
        plan.stopHumanVsAi = state.humanVsAiActive;
        plan.stopSelfPlay = state.selfPlayActive;
        plan.stopEngineGame = state.engineGameActive;
        return plan;
    }

    if (state.engineGameActive) {
        plan.requestEngineGameMove = true;
    } else if (state.selfPlayActive) {
        plan.requestSelfPlayMove = true;
    }
    return plan;
}

HumanMoveAutomationPlan planAfterHumanMove(
    const lizzie::core::Move& move,
    EngineAutomationState state)
{
    HumanMoveAutomationPlan plan;
    if (!state.humanVsAiActive) {
        return plan;
    }

    const bool terminalMove = move.type == lizzie::core::MoveType::Resign ||
        (move.type == lizzie::core::MoveType::Pass && state.previousMoveWasPass);
    if (terminalMove) {
        plan.stopHumanVsAi = true;
    } else {
        plan.requestHumanVsAiReply = true;
    }
    return plan;
}

AutomatedPlayTogglePlan planToggleHumanVsAi(EngineAutomationState state)
{
    AutomatedPlayTogglePlan plan;
    if (state.humanVsAiActive) {
        plan.stopHumanVsAi = true;
        return plan;
    }

    plan.stopEngineGame = state.engineGameActive;
    plan.stopSelfPlay = state.selfPlayActive;
    plan.startHumanVsAi = true;
    return plan;
}

AutomatedPlayTogglePlan planToggleSelfPlay(EngineAutomationState state)
{
    AutomatedPlayTogglePlan plan;
    if (state.selfPlayActive) {
        plan.stopSelfPlay = true;
        return plan;
    }

    plan.stopEngineGame = state.engineGameActive;
    plan.stopHumanVsAi = state.humanVsAiActive;
    plan.startSelfPlay = true;
    plan.requestSelfPlayMove = true;
    return plan;
}

AutomatedPlayTogglePlan planToggleEngineGame(EngineAutomationState state)
{
    AutomatedPlayTogglePlan plan;
    if (state.engineGameActive) {
        plan.stopEngineGame = true;
        return plan;
    }

    plan.stopHumanVsAi = state.humanVsAiActive;
    plan.stopSelfPlay = state.selfPlayActive;
    plan.stopRealtimeAnalysis = true;
    plan.stopCompareAnalysis = true;
    plan.clearCompareMoveRequest = true;
    plan.startEngineGame = true;
    plan.requestEngineGameMove = true;
    return plan;
}

}  // namespace lizzie::app
