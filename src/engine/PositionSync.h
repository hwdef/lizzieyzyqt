#pragma once

#include "GameModel.h"
#include "GtpProtocol.h"

#include <optional>
#include <string>
#include <vector>

namespace lizzie::engine {

struct PositionSyncPlan {
    std::vector<GtpCommand> commands;
    std::vector<std::string> warnings;
    bool fatal = false;
};

struct PositionSyncOptions {
    bool supportsKataSetRules = true;
};

struct RealtimeAnalysisOptions {
    int intervalCentiseconds = 50;
    bool includeOwnership = true;
    std::optional<lizzie::core::Color> player;
    std::optional<int> maxVisits;
    std::optional<int> maxPlayouts;
    std::optional<double> maxTimeSeconds;
    std::optional<double> playoutDoublingAdvantage;
    std::optional<double> analysisWideRootNoise;
};

[[nodiscard]] PositionSyncPlan buildPositionSyncPlan(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId,
    PositionSyncOptions options = {});

[[nodiscard]] GtpCommand buildKataAnalyzeCommand(RealtimeAnalysisOptions options = {});
[[nodiscard]] std::vector<GtpCommand> buildSearchParamCommands(RealtimeAnalysisOptions options = {});
[[nodiscard]] GtpCommand buildStopAnalyzeCommand();
[[nodiscard]] GtpCommand buildGenMoveCommand(lizzie::core::Color color);

}  // namespace lizzie::engine
