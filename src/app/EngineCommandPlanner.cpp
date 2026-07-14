#include "EngineCommandPlanner.h"

#include "BatchAnalysis.h"

namespace lizzie::app {

namespace {

void appendPositionSync(
    EngineCommandPlan* plan,
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId,
    const lizzie::engine::EngineCapabilities& capabilities)
{
    const lizzie::engine::PositionSyncPlan syncPlan = lizzie::engine::buildPositionSyncPlan(
        model,
        nodeId,
        lizzie::engine::PositionSyncOptions{capabilities.kataSetRules});
    plan->warnings.insert(plan->warnings.end(), syncPlan.warnings.begin(), syncPlan.warnings.end());
    plan->fatal = syncPlan.fatal;
    if (!syncPlan.fatal) {
        plan->preparationCommands.insert(
            plan->preparationCommands.end(),
            syncPlan.commands.begin(),
            syncPlan.commands.end());
    }
}

void appendSearchParams(
    EngineCommandPlan* plan,
    const lizzie::engine::EngineCapabilities& capabilities,
    lizzie::engine::RealtimeAnalysisOptions options)
{
    const std::vector<lizzie::engine::GtpCommand> commands = lizzie::engine::buildSearchParamCommands(options);
    if (!capabilities.kataSetParam) {
        if (!commands.empty()) {
            plan->warnings.push_back("KataGo does not report kata-set-param support; search limit settings were not sent");
        }
        return;
    }
    plan->preparationCommands.insert(plan->preparationCommands.end(), commands.begin(), commands.end());
}

EngineCommandPlan basePlan(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId,
    const lizzie::engine::EngineCapabilities& capabilities,
    lizzie::engine::RealtimeAnalysisOptions options,
    bool includeStopOnPositionError)
{
    EngineCommandPlan plan;
    std::string positionError;
    const lizzie::core::BoardPosition position = model.positionAt(nodeId, &positionError);
    plan.positionError = positionError;
    plan.sideToMove = position.sideToMove();
    plan.positionKey = lizzie::engine::analysisPositionKey(model, nodeId);

    if (!positionError.empty()) {
        plan.fatal = true;
        if (!includeStopOnPositionError) {
            return plan;
        }
    }

    if (capabilities.kataStop) {
        plan.preparationCommands.push_back(lizzie::engine::buildStopAnalyzeCommand());
    }
    appendPositionSync(&plan, model, nodeId, capabilities);
    if (!plan.fatal) {
        appendSearchParams(&plan, capabilities, options);
    }
    return plan;
}

}  // namespace

EngineCommandPlan buildRealtimeAnalysisCommandPlan(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId,
    const lizzie::engine::EngineCapabilities& capabilities,
    lizzie::engine::RealtimeAnalysisOptions options)
{
    EngineCommandPlan plan = basePlan(model, nodeId, capabilities, options, true);
    if (!plan.fatal) {
        options.player = plan.sideToMove;
        plan.finalCommand = lizzie::engine::buildKataAnalyzeCommand(options);
    }
    return plan;
}

EngineCommandPlan buildGenMoveCommandPlan(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId,
    const lizzie::engine::EngineCapabilities& capabilities,
    lizzie::engine::RealtimeAnalysisOptions options)
{
    EngineCommandPlan plan = basePlan(model, nodeId, capabilities, options, false);
    if (!plan.fatal) {
        plan.finalCommand = lizzie::engine::buildGenMoveCommand(plan.sideToMove);
    }
    return plan;
}

}  // namespace lizzie::app
