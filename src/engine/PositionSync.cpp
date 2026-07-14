#include "PositionSync.h"

#include "KataGoRules.h"

#include <cmath>
#include <sstream>

namespace lizzie::engine {

namespace {

std::string formatDouble(double value)
{
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

std::string colorArgument(lizzie::core::Color color)
{
    return color == lizzie::core::Color::Black ? "B" : "W";
}

bool hasPositiveInt(std::optional<int> value)
{
    return value.has_value() && *value > 0;
}

bool hasPositiveFiniteValue(std::optional<double> value)
{
    return value.has_value() && std::isfinite(*value) && *value > 0.0;
}

bool hasNonZeroFiniteValue(std::optional<double> value)
{
    return value.has_value() && std::isfinite(*value) && *value != 0.0;
}

int effectiveAnalyzeIntervalCentiseconds(int intervalCentiseconds)
{
    constexpr int kDefaultIntervalCentiseconds = 50;
    constexpr int kMaxIntervalCentiseconds = 10000;
    if (intervalCentiseconds <= 0) {
        return kDefaultIntervalCentiseconds;
    }
    if (intervalCentiseconds > kMaxIntervalCentiseconds) {
        return kMaxIntervalCentiseconds;
    }
    return intervalCentiseconds;
}

std::string moveArgument(const lizzie::core::Move& move, lizzie::core::BoardSize boardSize)
{
    switch (move.type) {
    case lizzie::core::MoveType::Play:
        return formatPointForGtp(move.point, boardSize);
    case lizzie::core::MoveType::Pass:
        return "pass";
    case lizzie::core::MoveType::Resign:
        return "resign";
    }
    return "pass";
}

void appendPlayCommand(PositionSyncPlan* plan, const lizzie::core::Move& move, lizzie::core::BoardSize boardSize)
{
    if (move.type == lizzie::core::MoveType::Resign) {
        plan->warnings.push_back("resign nodes are not replayed to KataGo GTP");
        return;
    }
    const std::string point = moveArgument(move, boardSize);
    if (point.empty()) {
        plan->warnings.push_back("move cannot be represented as a GTP coordinate");
        plan->fatal = true;
        return;
    }
    plan->commands.push_back(GtpCommand{std::nullopt, "play", {colorArgument(move.color), point}});
}

void appendSetupCommands(
    PositionSyncPlan* plan,
    const lizzie::core::SetupStones& setup,
    lizzie::core::BoardSize boardSize,
    bool ignoreEmptySetup)
{
    const lizzie::core::SetupStones normalized = lizzie::core::normalizedSetupStones(setup);
    for (lizzie::core::Point point : normalized.black) {
        const std::string gtpPoint = formatPointForGtp(point, boardSize);
        if (gtpPoint.empty()) {
            plan->warnings.push_back("black setup stone cannot be represented as a GTP coordinate");
            plan->fatal = true;
            continue;
        }
        plan->commands.push_back(GtpCommand{std::nullopt, "play", {"B", gtpPoint}});
    }
    for (lizzie::core::Point point : normalized.white) {
        const std::string gtpPoint = formatPointForGtp(point, boardSize);
        if (gtpPoint.empty()) {
            plan->warnings.push_back("white setup stone cannot be represented as a GTP coordinate");
            plan->fatal = true;
            continue;
        }
        plan->commands.push_back(GtpCommand{std::nullopt, "play", {"W", gtpPoint}});
    }
    if (!ignoreEmptySetup && !normalized.empty.empty()) {
        plan->warnings.push_back("AE setup points cannot be represented by basic GTP replay");
        plan->fatal = true;
    }
}

std::optional<lizzie::core::NodeId> firstResignNodeInPath(
    const lizzie::core::GameModel& model,
    const std::vector<lizzie::core::NodeId>& path)
{
    for (lizzie::core::NodeId pathNodeId : path) {
        const lizzie::core::GameNode& node = model.node(pathNodeId);
        if (node.move.has_value() && node.move->type == lizzie::core::MoveType::Resign) {
            return pathNodeId;
        }
    }
    return std::nullopt;
}

std::optional<lizzie::core::NodeId> firstUnsupportedSetupNodeInPath(
    const lizzie::core::GameModel& model,
    const std::vector<lizzie::core::NodeId>& path)
{
    for (std::size_t index = 1; index < path.size(); ++index) {
        const lizzie::core::NodeId pathNodeId = path[index];
        if (!model.node(pathNodeId).setupStones.isEmpty()) {
            return pathNodeId;
        }
    }
    return std::nullopt;
}

}  // namespace

PositionSyncPlan buildPositionSyncPlan(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId,
    PositionSyncOptions options)
{
    PositionSyncPlan plan;
    const lizzie::core::BoardSize boardSize = model.boardSize();
    if (!isGtpBoardSizeSupported(boardSize)) {
        plan.fatal = true;
        plan.warnings.push_back("board width cannot be represented by GTP coordinates; engine sync was skipped");
        return plan;
    }

    const std::vector<lizzie::core::NodeId> path = model.pathTo(nodeId);
    if (path.empty()) {
        plan.fatal = true;
        plan.warnings.push_back("cannot build GTP sync for missing game node");
        return plan;
    }

    std::string positionError;
    static_cast<void>(model.positionAt(nodeId, &positionError));
    if (!positionError.empty()) {
        plan.fatal = true;
        plan.warnings.push_back("cannot build GTP sync for invalid game position: " + positionError);
        return plan;
    }
    if (const std::optional<lizzie::core::NodeId> resignNode = firstResignNodeInPath(model, path)) {
        plan.fatal = true;
        plan.warnings.push_back(
            "cannot build GTP sync for node after resign: " + std::to_string(*resignNode));
        return plan;
    }
    if (const std::optional<lizzie::core::NodeId> setupNode = firstUnsupportedSetupNodeInPath(model, path)) {
        plan.fatal = true;
        plan.warnings.push_back(
            "cannot build GTP sync for node with mid-game setup stones: " + std::to_string(*setupNode));
        return plan;
    }

    plan.commands.push_back(GtpCommand{std::nullopt, "clear_board", {}});
    if (boardSize.isSquare()) {
        plan.commands.push_back(GtpCommand{std::nullopt, "boardsize", {std::to_string(boardSize.width)}});
    } else {
        plan.commands.push_back(GtpCommand{
            std::nullopt,
            "rectangular_boardsize",
            {std::to_string(boardSize.width), std::to_string(boardSize.height)}});
    }

    const lizzie::core::GameInfo& info = model.gameInfo();
    if (info.komi.has_value() && std::isfinite(*info.komi)) {
        plan.commands.push_back(GtpCommand{std::nullopt, "komi", {formatDouble(*info.komi)}});
    } else if (info.komi.has_value()) {
        plan.warnings.push_back("non-finite komi was not sent to KataGo GTP");
    }
    const std::string rules = canonicalRulesForKataGo(info.rules);
    if (!rules.empty() && options.supportsKataSetRules) {
        plan.commands.push_back(GtpCommand{std::nullopt, "kata-set-rules", {rules}});
    } else if (!rules.empty()) {
        plan.warnings.push_back("KataGo does not report kata-set-rules support; SGF rules were not sent");
    }

    for (std::size_t index = 0; index < path.size(); ++index) {
        const lizzie::core::NodeId id = path[index];
        const lizzie::core::GameNode& node = model.node(id);
        appendSetupCommands(&plan, node.setupStones, boardSize, index == 0);
        if (node.move.has_value()) {
            appendPlayCommand(&plan, *node.move, boardSize);
        }
    }
    return plan;
}

GtpCommand buildKataAnalyzeCommand(RealtimeAnalysisOptions options)
{
    std::vector<std::string> arguments;
    if (options.player.has_value()) {
        arguments.push_back(colorArgument(*options.player));
    }
    arguments.push_back(std::to_string(effectiveAnalyzeIntervalCentiseconds(options.intervalCentiseconds)));
    arguments.push_back("ownership");
    arguments.push_back(options.includeOwnership ? "true" : "false");
    arguments.push_back("movesOwnership");
    arguments.push_back(options.includeOwnership ? "true" : "false");
    arguments.push_back("pvVisits");
    arguments.push_back("true");
    arguments.push_back("rootInfo");
    arguments.push_back("true");
    return GtpCommand{std::nullopt, "kata-analyze", std::move(arguments)};
}

std::vector<GtpCommand> buildSearchParamCommands(RealtimeAnalysisOptions options)
{
    std::vector<GtpCommand> commands;
    if (hasPositiveInt(options.maxVisits)) {
        commands.push_back(GtpCommand{std::nullopt, "kata-set-param", {"maxVisits", std::to_string(*options.maxVisits)}});
    }
    if (hasPositiveInt(options.maxPlayouts)) {
        commands.push_back(
            GtpCommand{std::nullopt, "kata-set-param", {"maxPlayouts", std::to_string(*options.maxPlayouts)}});
    }
    if (hasPositiveFiniteValue(options.maxTimeSeconds)) {
        commands.push_back(
            GtpCommand{std::nullopt, "kata-set-param", {"maxTime", formatDouble(*options.maxTimeSeconds)}});
    }
    if (hasNonZeroFiniteValue(options.playoutDoublingAdvantage)) {
        commands.push_back(GtpCommand{
            std::nullopt,
            "kata-set-param",
            {"playoutDoublingAdvantage", formatDouble(*options.playoutDoublingAdvantage)}});
    }
    if (hasPositiveFiniteValue(options.analysisWideRootNoise)) {
        commands.push_back(GtpCommand{
            std::nullopt,
            "kata-set-param",
            {"analysisWideRootNoise", formatDouble(*options.analysisWideRootNoise)}});
    }
    return commands;
}

GtpCommand buildStopAnalyzeCommand()
{
    return GtpCommand{std::nullopt, "kata-stop", {}};
}

GtpCommand buildGenMoveCommand(lizzie::core::Color color)
{
    return GtpCommand{std::nullopt, "genmove", {colorArgument(color)}};
}

}  // namespace lizzie::engine
