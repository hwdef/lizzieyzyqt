#include "PositionSync.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace lizzie::core;
using namespace lizzie::engine;

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void testSquareReplay()
{
    GameModel model(BoardSize::square(9));
    model.gameInfo().komi = 6.5;
    model.gameInfo().rules = "Chinese";
    model.root().setupStones.black.push_back(Point{0, 8});
    const NodeId first = model.addMove(Move::play(Color::Black, Point{3, 5}));
    const NodeId second = model.addChild(first, Move::pass(Color::White));

    const PositionSyncPlan plan = buildPositionSyncPlan(model, second);
    require(plan.warnings.empty(), "simple replay should not warn");
    require(plan.commands.size() == 7, "sync should contain setup and two moves");
    require(plan.commands[0].toLine() == "clear_board\n", "sync should clear board first");
    require(plan.commands[1].toLine() == "boardsize 9\n", "sync should set square board size");
    require(plan.commands[2].toLine() == "komi 6.5\n", "sync should set komi");
    require(plan.commands[3].toLine() == "kata-set-rules chinese\n", "sync should set canonical rules");
    require(plan.commands[4].toLine() == "play B A1\n", "sync should replay setup stones");
    require(plan.commands[5].toLine() == "play B D4\n", "sync should replay moves");
    require(plan.commands[6].toLine() == "play W pass\n", "sync should replay pass");
}

void testRectangularReplay()
{
    GameModel model(BoardSize{9, 13});
    const PositionSyncPlan plan = buildPositionSyncPlan(model, model.rootId());
    require(plan.commands.size() == 2, "rectangular root sync should contain clear and size");
    require(plan.commands[1].toLine() == "rectangular_boardsize 9 13\n", "sync should set rectangular board size");
    require(plan.warnings.empty(), "rectangular board without setup clears should not warn");
}

void testConflictingSetupReplayIsCanonicalized()
{
    GameModel model(BoardSize::square(9));
    model.root().setupStones.black = {Point{0, 0}, Point{1, 1}, Point{1, 1}};
    model.root().setupStones.white = {Point{1, 1}, Point{2, 2}};

    const PositionSyncPlan plan = buildPositionSyncPlan(model, model.rootId());
    require(!plan.fatal, "conflicting setup without AE should still sync");
    require(plan.warnings.empty(), "conflicting setup without AE should not warn after canonicalization");
    require(plan.commands.size() == 5, "canonicalized setup sync should emit clear, size, and three setup plays");
    require(plan.commands[2].toLine() == "play B A9\n", "canonicalized setup should keep unique black stone");
    require(plan.commands[3].toLine() == "play W B8\n", "canonicalized setup should replay overwritten point as white");
    require(plan.commands[4].toLine() == "play W C7\n", "canonicalized setup should replay unique white stone");
}

void testRootAeSetupReplayIsCanonicalized()
{
    GameModel model(BoardSize::square(9));
    model.root().setupStones.black.push_back(Point{0, 0});
    model.root().setupStones.white.push_back(Point{1, 1});
    model.root().setupStones.empty.push_back(Point{0, 0});

    const PositionSyncPlan plan = buildPositionSyncPlan(model, model.rootId());
    require(!plan.fatal, "root AE setup should be safe after clear_board");
    require(plan.warnings.empty(), "canonical root AE setup should not warn");
    require(plan.commands.size() == 3, "root AE sync should emit clear, size, and remaining setup stones");
    require(plan.commands[0].toLine() == "clear_board\n", "root AE sync should clear board first");
    require(plan.commands[1].toLine() == "boardsize 9\n", "root AE sync should set board size");
    require(plan.commands[2].toLine() == "play W B8\n", "root AE sync should replay only remaining setup stones");
}

void testMidGameAeSetupIsFatal()
{
    GameModel model(BoardSize::square(9));
    const NodeId child = model.addMove(Move::play(Color::Black, Point{3, 3}));
    model.node(child).setupStones.empty.push_back(Point{0, 0});

    const PositionSyncPlan plan = buildPositionSyncPlan(model, child);
    require(plan.fatal, "mid-game AE setup should be fatal for GTP sync");
    require(plan.commands.empty(), "mid-game AE setup should not emit partial GTP sync commands");
    require(!plan.warnings.empty(), "mid-game AE setup should produce a diagnostic");
    require(
        plan.warnings.front().find("mid-game setup stones") != std::string::npos,
        "mid-game AE diagnostic should mention setup stones");
}

void testMidGameAbAwSetupIsFatal()
{
    GameModel model(BoardSize::square(9));
    const NodeId first = model.addMove(Move::play(Color::Black, Point{3, 3}));
    const NodeId setupNode = model.addChild(first);
    model.node(setupNode).setupStones.black.push_back(Point{0, 0});
    model.node(setupNode).setupStones.white.push_back(Point{1, 1});

    const PositionSyncPlan plan = buildPositionSyncPlan(model, setupNode);
    require(plan.fatal, "mid-game AB/AW setup should be fatal for GTP sync");
    require(plan.commands.empty(), "mid-game AB/AW setup should not emit partial GTP sync commands");
    require(!plan.warnings.empty(), "mid-game AB/AW setup should produce a diagnostic");
    require(
        plan.warnings.front().find("mid-game setup stones") != std::string::npos,
        "mid-game AB/AW diagnostic should mention setup stones");
}

void testRulesCapabilityFallback()
{
    GameModel model(BoardSize::square(9));
    model.gameInfo().rules = "Japanese";

    const PositionSyncPlan plan =
        buildPositionSyncPlan(model, model.rootId(), PositionSyncOptions{.supportsKataSetRules = false});
    require(plan.commands.size() == 2, "rules command should be skipped when capability is missing");
    require(plan.commands[0].toLine() == "clear_board\n", "fallback sync should still clear board");
    require(plan.commands[1].toLine() == "boardsize 9\n", "fallback sync should still set board size");
    require(plan.warnings.size() == 1, "missing kata-set-rules support should warn");
    require(
        plan.warnings.front().find("kata-set-rules") != std::string::npos,
        "missing rules support warning should name kata-set-rules");
}

void testAllPlanRulesMapToKataGoCanonicalNames()
{
    const std::vector<std::pair<std::string, std::string>> rules{
        {"Chinese", "chinese"},
        {"Japanese", "japanese"},
        {"Tromp-Taylor", "tromp-taylor"},
        {"AGA", "aga"},
        {"New Zealand", "new-zealand"},
        {" NZ ", "new-zealand"},
        {"Korean", "korean"},
    };
    for (const auto& [input, expected] : rules) {
        GameModel model(BoardSize::square(9));
        model.gameInfo().rules = input;

        const PositionSyncPlan plan = buildPositionSyncPlan(model, model.rootId());
        require(!plan.fatal, "canonical rules test should not produce fatal sync plan");
        require(plan.commands.size() == 3, "canonical rules test should include clear, size, and rules command");
        require(
            plan.commands[2].toLine() == "kata-set-rules " + expected + "\n",
            "rules should map to KataGo canonical value: " + input);
    }
}

void testUnsupportedBoardWidth()
{
    GameModel model(BoardSize{26, 9});
    model.root().setupStones.black.push_back(Point{25, 8});
    model.addMove(Move::play(Color::Black, Point{25, 7}));

    const PositionSyncPlan plan = buildPositionSyncPlan(model, model.currentId());
    require(plan.fatal, "unsupported GTP board width should be fatal");
    require(plan.commands.empty(), "unsupported GTP board width should not emit sync commands");
    require(!plan.warnings.empty(), "unsupported GTP board width should warn");
}

void testInvalidGamePositionIsFatal()
{
    GameModel model(BoardSize::square(5));
    const NodeId first = model.addMove(Move::play(Color::Black, Point{1, 1}));
    const NodeId illegal = model.addChild(first, Move::play(Color::White, Point{1, 1}));

    const PositionSyncPlan plan = buildPositionSyncPlan(model, illegal);
    require(plan.fatal, "invalid game position should be fatal for GTP sync");
    require(plan.commands.empty(), "invalid game position should not emit partial GTP sync commands");
    require(!plan.warnings.empty(), "invalid game position should produce a diagnostic");
    require(
        plan.warnings.front().find("point is already occupied") != std::string::npos,
        "invalid game position diagnostic should include local rule error");
}

void testInvalidSetupPositionIsFatal()
{
    GameModel model(BoardSize::square(5));
    model.root().setupStones.black.push_back(Point{5, 0});

    const PositionSyncPlan plan = buildPositionSyncPlan(model, model.rootId());
    require(plan.fatal, "invalid setup position should be fatal for GTP sync");
    require(plan.commands.empty(), "invalid setup position should not emit partial GTP sync commands");
    require(!plan.warnings.empty(), "invalid setup position should produce a diagnostic");
    require(
        plan.warnings.front().find("setup stone is outside the board") != std::string::npos,
        "invalid setup diagnostic should include local setup error");
}

void testResignNodeIsFatal()
{
    GameModel model(BoardSize::square(9));
    model.addMove(Move::play(Color::Black, Point{3, 3}));
    const NodeId resign = model.addMove(Move::resign(Color::White));

    const PositionSyncPlan plan = buildPositionSyncPlan(model, resign);
    require(plan.fatal, "resign node should be fatal for GTP sync");
    require(plan.commands.empty(), "resign node should not emit partial GTP sync commands");
    require(!plan.warnings.empty(), "resign node should produce a diagnostic");
    require(plan.warnings.front().find("resign") != std::string::npos, "resign diagnostic should mention resign");
}

void testNonFiniteKomiIsSkipped()
{
    GameModel model(BoardSize::square(9));
    model.gameInfo().komi = std::numeric_limits<double>::infinity();

    const PositionSyncPlan plan = buildPositionSyncPlan(model, model.rootId());
    require(!plan.fatal, "non-finite komi should not make GTP sync fatal");
    require(plan.commands.size() == 2, "non-finite komi should not emit a komi command");
    require(plan.commands[0].toLine() == "clear_board\n", "sync should still clear the board");
    require(plan.commands[1].toLine() == "boardsize 9\n", "sync should still set board size");
    require(!plan.warnings.empty(), "non-finite komi should produce a diagnostic");
    require(
        plan.warnings.front().find("non-finite komi") != std::string::npos,
        "non-finite komi diagnostic should name komi");
}

void testAnalyzeCommands()
{
    RealtimeAnalysisOptions options;
    options.intervalCentiseconds = 25;
    options.includeOwnership = true;
    options.maxVisits = 300;
    options.maxPlayouts = 600;
    options.maxTimeSeconds = 1.5;
    options.playoutDoublingAdvantage = 0.25;
    options.analysisWideRootNoise = 0.1;

    require(
        buildKataAnalyzeCommand(options).toLine() ==
            "kata-analyze 25 ownership true movesOwnership true pvVisits true rootInfo true\n",
        "kata-analyze command should request root and per-move output options");
    options.player = Color::White;
    require(
        buildKataAnalyzeCommand(options).toLine() ==
            "kata-analyze W 25 ownership true movesOwnership true pvVisits true rootInfo true\n",
        "kata-analyze command should send explicit side to move when provided");
    options.includeOwnership = false;
    require(
        buildKataAnalyzeCommand(options).toLine() ==
            "kata-analyze W 25 ownership false movesOwnership false pvVisits true rootInfo true\n",
        "kata-analyze command should preserve disabled ownership while still requesting PV visits and rootInfo");

    RealtimeAnalysisOptions invalidIntervalOptions;
    invalidIntervalOptions.intervalCentiseconds = 0;
    require(
        buildKataAnalyzeCommand(invalidIntervalOptions).toLine() ==
            "kata-analyze 50 ownership true movesOwnership true pvVisits true rootInfo true\n",
        "non-positive kata-analyze intervals should fall back to the default interval");
    invalidIntervalOptions.intervalCentiseconds = -25;
    require(
        buildKataAnalyzeCommand(invalidIntervalOptions).toLine() ==
            "kata-analyze 50 ownership true movesOwnership true pvVisits true rootInfo true\n",
        "negative kata-analyze intervals should not be sent to KataGo");
    RealtimeAnalysisOptions oversizedIntervalOptions;
    oversizedIntervalOptions.intervalCentiseconds = 10001;
    require(
        buildKataAnalyzeCommand(oversizedIntervalOptions).toLine() ==
            "kata-analyze 10000 ownership true movesOwnership true pvVisits true rootInfo true\n",
        "oversized kata-analyze intervals should be clamped to the settings maximum");

    const std::vector<GtpCommand> params = buildSearchParamCommands(options);
    require(params.size() == 5, "configured search parameters should produce GTP commands");
    require(params[0].toLine() == "kata-set-param maxVisits 300\n", "maxVisits should use kata-set-param");
    require(params[1].toLine() == "kata-set-param maxPlayouts 600\n", "maxPlayouts should use kata-set-param");
    require(params[2].toLine() == "kata-set-param maxTime 1.5\n", "maxTime should use kata-set-param");
    require(
        params[3].toLine() == "kata-set-param playoutDoublingAdvantage 0.25\n",
        "PDA should use kata-set-param");
    require(
        params[4].toLine() == "kata-set-param analysisWideRootNoise 0.1\n",
        "WRN should use kata-set-param");

    RealtimeAnalysisOptions nonFiniteOptions;
    nonFiniteOptions.maxVisits = 300;
    nonFiniteOptions.maxTimeSeconds = std::numeric_limits<double>::infinity();
    nonFiniteOptions.playoutDoublingAdvantage = std::numeric_limits<double>::quiet_NaN();
    nonFiniteOptions.analysisWideRootNoise = -std::numeric_limits<double>::infinity();
    const std::vector<GtpCommand> nonFiniteParams = buildSearchParamCommands(nonFiniteOptions);
    require(nonFiniteParams.size() == 1, "non-finite realtime search parameters should be skipped");
    require(
        nonFiniteParams.front().toLine() == "kata-set-param maxVisits 300\n",
        "finite realtime search parameters should remain after skipping non-finite values");

    RealtimeAnalysisOptions disabledOptions;
    disabledOptions.maxVisits = 0;
    disabledOptions.maxPlayouts = -5;
    disabledOptions.maxTimeSeconds = 0.0;
    disabledOptions.playoutDoublingAdvantage = 0.0;
    disabledOptions.analysisWideRootNoise = 0.0;
    require(
        buildSearchParamCommands(disabledOptions).empty(),
        "non-positive realtime search parameters should be skipped");

    RealtimeAnalysisOptions invalidLimitOptions;
    invalidLimitOptions.maxVisits = -1;
    invalidLimitOptions.maxPlayouts = -2;
    invalidLimitOptions.maxTimeSeconds = -1.0;
    invalidLimitOptions.analysisWideRootNoise = -0.1;
    require(
        buildSearchParamCommands(invalidLimitOptions).empty(),
        "negative realtime limits should not be sent to KataGo");

    RealtimeAnalysisOptions negativePdaOptions;
    negativePdaOptions.playoutDoublingAdvantage = -0.25;
    const std::vector<GtpCommand> negativePdaParams = buildSearchParamCommands(negativePdaOptions);
    require(negativePdaParams.size() == 1, "negative PDA should remain a valid realtime search parameter");
    require(
        negativePdaParams.front().toLine() == "kata-set-param playoutDoublingAdvantage -0.25\n",
        "negative PDA should use kata-set-param");
    require(buildStopAnalyzeCommand().toLine() == "kata-stop\n", "stop analyze command should format");
    require(buildGenMoveCommand(Color::Black).toLine() == "genmove B\n", "black genmove command should format");
    require(buildGenMoveCommand(Color::White).toLine() == "genmove W\n", "white genmove command should format");
}

}  // namespace

int main()
{
    try {
        testSquareReplay();
        testRectangularReplay();
        testConflictingSetupReplayIsCanonicalized();
        testRootAeSetupReplayIsCanonicalized();
        testMidGameAeSetupIsFatal();
        testMidGameAbAwSetupIsFatal();
        testRulesCapabilityFallback();
        testAllPlanRulesMapToKataGoCanonicalNames();
        testUnsupportedBoardWidth();
        testInvalidGamePositionIsFatal();
        testInvalidSetupPositionIsFatal();
        testResignNodeIsFatal();
        testNonFiniteKomiIsSkipped();
        testAnalyzeCommands();
    } catch (const std::exception& error) {
        std::cerr << "position_sync_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "position_sync_tests passed\n";
    return EXIT_SUCCESS;
}
