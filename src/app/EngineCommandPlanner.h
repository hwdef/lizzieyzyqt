#pragma once

#include "GameModel.h"
#include "GtpProtocol.h"
#include "PositionSync.h"

#include <string>
#include <vector>

namespace lizzie::app {

struct EngineCommandPlan {
    std::vector<lizzie::engine::GtpCommand> preparationCommands;
    lizzie::engine::GtpCommand finalCommand;
    std::vector<std::string> warnings;
    std::string positionError;
    std::string positionKey;
    lizzie::core::Color sideToMove = lizzie::core::Color::Black;
    bool fatal = false;
};

[[nodiscard]] EngineCommandPlan buildRealtimeAnalysisCommandPlan(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId,
    const lizzie::engine::EngineCapabilities& capabilities,
    lizzie::engine::RealtimeAnalysisOptions options);

[[nodiscard]] EngineCommandPlan buildGenMoveCommandPlan(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId,
    const lizzie::engine::EngineCapabilities& capabilities,
    lizzie::engine::RealtimeAnalysisOptions options);

}  // namespace lizzie::app

