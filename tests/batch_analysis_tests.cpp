#include "BatchAnalysis.h"
#include "Sgf.h"

#include <QJsonArray>
#include <QJsonObject>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace lizzie::core;
using namespace lizzie::engine;

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::size_t countOccurrences(const std::string& text, const std::string& needle)
{
    std::size_t count = 0;
    std::size_t position = 0;
    while ((position = text.find(needle, position)) != std::string::npos) {
        ++count;
        position += needle.size();
    }
    return count;
}

bool containsInitialStone(const std::vector<InitialStone>& stones, Color color, Point point)
{
    for (const InitialStone& stone : stones) {
        if (stone.color == color && stone.point == point) {
            return true;
        }
    }
    return false;
}

std::vector<double> ownershipValues(BoardSize boardSize, double blackValue = 0.25, double whiteValue = -0.25)
{
    std::vector<double> values;
    values.reserve(static_cast<std::size_t>(boardSize.pointCount()));
    for (int index = 0; index < boardSize.pointCount(); ++index) {
        values.push_back(index % 2 == 0 ? blackValue : whiteValue);
    }
    return values;
}

GameModel sampleModel(NodeId* first, NodeId* second)
{
    GameModel model(BoardSize::square(9));
    model.gameInfo().komi = 6.5;
    model.gameInfo().rules = "Chinese";
    model.root().setupStones.black.push_back(Point{0, 8});
    *first = model.addMove(Move::play(Color::Black, Point{3, 5}));
    *second = model.addMove(Move::pass(Color::White));
    return model;
}

void testCollectAndBuildPlan()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    const std::vector<NodeId> nodes = collectAnalysisNodeIds(model, model.rootId());
    require(nodes.size() == 3, "collector should include root and descendants");
    require(nodes[0] == model.rootId() && nodes[1] == first && nodes[2] == second, "collector should use depth-first order");

    BatchAnalysisOptions options;
    options.maxVisits = 200;
    options.maxPlayouts = 400;
    options.maxTime = 3.25;
    options.playoutDoublingAdvantage = -0.25;
    options.wideRootNoise = 0.2;
    options.includeOwnership = false;
    const BatchAnalysisPlan plan = buildBatchAnalysisPlan(model, {second, 999}, options);
    require(plan.items.size() == 1, "planner should skip missing nodes");
    require(plan.warnings.size() == 1, "planner should warn for missing nodes");
    require(plan.items.front().nodeId == second, "planner should retain requested node id");

    const AnalysisRequest& request = plan.items.front().request;
    require(request.id == analysisRequestId(second), "request id should encode node id");
    require(request.boardSize.width == 9 && request.boardSize.height == 9, "request should use board size");
    require(request.komi == 6.5, "request should use game komi");
    require(request.rules == "chinese", "request should use canonical game rules");
    require(request.maxVisits == 200, "request should use max visits");
    require(request.maxPlayouts == 400, "request should use max playouts");
    require(request.maxTime == 3.25, "request should use max time");
    require(request.playoutDoublingAdvantage == -0.25, "request should use PDA");
    require(request.wideRootNoise == 0.2, "request should use wide root noise");
    require(!request.includeOwnership, "request should use the batch ownership option");
    require(request.initialStones.size() == 1, "root setup stones should become initialStones");
    require(request.moves.size() == 2, "path moves should become request moves");

    const QJsonObject json = request.toJson();
    require(json.value("initialStones").toArray().size() == 1, "initialStones should serialize");
    require(json.value("moves").toArray().size() == 2, "moves should serialize");
    require(!json.value("includeOwnership").toBool(), "batch request should serialize disabled ownership");
    require(json.value("includePVVisits").toBool(), "batch analysis should request PV visit counts");
    require(json.value("maxVisits").toInt() == 200, "batch request should serialize max visits");
    require(json.value("maxPlayouts").toInt() == 400, "batch request should serialize max playouts");
    require(json.value("maxTime").toDouble() == 3.25, "batch request should serialize max time");
    require(
        json.value("overrideSettings").toObject().value("playoutDoublingAdvantage").toDouble() == -0.25,
        "batch request should serialize PDA override");
    require(
        json.value("overrideSettings").toObject().value("wideRootNoise").toDouble() == 0.2,
        "batch request should serialize WRN override");
    require(
        json.value("overrideSettings").toObject().value("reportAnalysisWinratesAs").toString() == "BLACK",
        "batch request should force black-perspective winrate output");
}

void testBatchAnalysisUsesFiniteKomi()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    BatchAnalysisOptions invalidOverride;
    invalidOverride.komiOverride = std::numeric_limits<double>::infinity();
    const BatchAnalysisPlan fallbackToGame = buildBatchAnalysisPlan(model, {second}, invalidOverride);
    require(fallbackToGame.items.size() == 1, "planner should build a request with invalid komi override");
    require(fallbackToGame.items.front().request.komi == 6.5, "invalid komi override should fall back to game komi");

    model.gameInfo().komi = -std::numeric_limits<double>::infinity();
    BatchAnalysisOptions invalidGameKomi;
    invalidGameKomi.komiOverride = std::numeric_limits<double>::quiet_NaN();
    const BatchAnalysisPlan fallbackToDefault = buildBatchAnalysisPlan(model, {second}, invalidGameKomi);
    require(fallbackToDefault.items.size() == 1, "planner should build a request with invalid game komi");
    require(fallbackToDefault.items.front().request.komi == 7.5, "invalid game komi should fall back to default");
}

void testDuplicateRequestedNodesAreDeduplicated()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    const BatchAnalysisPlan plan = buildBatchAnalysisPlan(model, {second, second, first, 999, second, 999});
    require(plan.items.size() == 2, "planner should deduplicate repeated requested nodes");
    require(plan.items[0].nodeId == second, "planner should preserve the first duplicate occurrence");
    require(plan.items[1].nodeId == first, "planner should keep later unique nodes");
    require(plan.items[0].request.id == analysisRequestId(second), "first request id should encode second node");
    require(plan.items[1].request.id == analysisRequestId(first), "second request id should encode first node");
    require(plan.items[0].request.id != plan.items[1].request.id, "planner should emit unique request ids");
    require(plan.warnings.size() == 1, "planner should warn once for repeated missing nodes");
    require(
        plan.warnings.front().find("does not exist and was skipped") != std::string::npos,
        "planner warning should explain the skipped missing node");
}

void testRootInitialStonesAreCanonicalized()
{
    GameModel model(BoardSize::square(9));
    model.root().setupStones.black = {Point{0, 0}, Point{0, 0}, Point{2, 2}};
    model.root().setupStones.white = {Point{2, 2}, Point{8, 8}, Point{8, 8}};

    const BatchAnalysisPlan plan = buildBatchAnalysisPlan(model, {model.rootId()});
    require(plan.items.size() == 1, "canonicalized root setup should produce an analysis request");
    const AnalysisRequest& request = plan.items.front().request;
    require(request.initialStones.size() == 3, "duplicate root setup stones should be removed from initialStones");
    require(request.initialStones[0].color == Color::Black, "first unique root setup stone should remain black");
    require(request.initialStones[0].point == Point{0, 0}, "first unique root setup point should remain");
    require(request.initialStones[1].color == Color::White, "conflicting root setup point should use the final white color");
    require(request.initialStones[1].point == Point{2, 2}, "conflicting root setup point should remain at the same point");
    require(request.initialStones[2].color == Color::White, "duplicate white root setup stone should remain white once");
    require(request.initialStones[2].point == Point{8, 8}, "duplicate white root setup point should remain once");

    const QJsonArray initialStones = request.toJson().value("initialStones").toArray();
    require(initialStones.size() == 3, "canonicalized initialStones should serialize without duplicates");
    require(initialStones.at(0).toArray().at(1).toString() == "A9", "black setup point should serialize once");
    require(initialStones.at(1).toArray().at(0).toString() == "W", "overlapping setup point should serialize as white");
    require(initialStones.at(1).toArray().at(1).toString() == "C7", "overlapping setup point should serialize once");
    require(initialStones.at(2).toArray().at(1).toString() == "J1", "duplicate lower-edge setup point should serialize once");
}

void testMidGameSetupUsesSnapshotRequest()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);
    model.node(first).setupStones.white.push_back(Point{1, 1});
    model.node(first).setupStones.empty.push_back(Point{0, 8});

    const BatchAnalysisPlan plan = buildBatchAnalysisPlan(model, {second});
    require(plan.items.size() == 1, "planner should analyze nodes with mid-game setup");
    require(plan.warnings.empty(), "mid-game setup snapshot should not warn");

    const AnalysisRequest& request = plan.items.front().request;
    require(request.id == analysisRequestId(second), "snapshot request should keep the target node id");
    require(request.initialPlayer == Color::White, "snapshot request should start from setup-node side to move");
    require(request.moves.size() == 1, "snapshot request should replay moves after the setup node");
    require(request.moves.front().type == MoveType::Pass, "snapshot request should replay the descendant pass");
    require(request.moves.front().color == Color::White, "snapshot request should preserve descendant move color");
    require(request.initialStones.size() == 2, "snapshot request should encode the setup-node board state");
    require(
        containsInitialStone(request.initialStones, Color::Black, Point{3, 5}),
        "snapshot request should include the move played on the setup node");
    require(
        containsInitialStone(request.initialStones, Color::White, Point{1, 1}),
        "snapshot request should include mid-game AW stones");
    require(
        !containsInitialStone(request.initialStones, Color::Black, Point{0, 8}),
        "snapshot request should apply mid-game AE clears");

    const QJsonObject json = request.toJson();
    require(json.value("initialStones").toArray().size() == 2, "snapshot initialStones should serialize");
    require(json.value("moves").toArray().size() == 1, "snapshot descendant moves should serialize");
}

void testRootAeSetupIsCanonicalized()
{
    GameModel model(BoardSize::square(9));
    model.root().setupStones.black.push_back(Point{0, 0});
    model.root().setupStones.white.push_back(Point{1, 1});
    model.root().setupStones.empty.push_back(Point{0, 0});

    const BatchAnalysisPlan plan = buildBatchAnalysisPlan(model, {model.rootId()});
    require(plan.items.size() == 1, "planner should analyze root AE setup after canonicalization");
    require(plan.warnings.empty(), "canonical root AE setup should not warn");
    const AnalysisRequest& request = plan.items.front().request;
    require(request.initialStones.size() == 1, "root AE should remove emptied stones from initialStones");
    require(request.initialStones.front().color == Color::White, "remaining root setup stone should keep its color");
    require(request.initialStones.front().point == Point{1, 1}, "remaining root setup point should keep its point");

    const QJsonArray initialStones = request.toJson().value("initialStones").toArray();
    require(initialStones.size() == 1, "canonical root AE initialStones should serialize");
    require(initialStones.at(0).toArray().at(0).toString() == "W", "canonical root AE stone should serialize color");
    require(initialStones.at(0).toArray().at(1).toString() == "B8", "canonical root AE stone should serialize point");
}

void testInitialPlayerFromRootSideToMove()
{
    SgfParser parser;
    GameModel rootPlayer = parser.parseGame("(;GM[1]FF[4]SZ[9]PL[W])");
    const BatchAnalysisPlan rootPlan = buildBatchAnalysisPlan(rootPlayer, {rootPlayer.rootId()});
    require(rootPlan.items.size() == 1, "root PL should produce an analysis request");
    require(rootPlan.items.front().request.moves.empty(), "root PL request should not invent moves");
    require(rootPlan.items.front().request.initialPlayer == Color::White, "root PL should set request initial player");
    require(
        rootPlan.items.front().request.toJson().value("initialPlayer").toString() == "W",
        "root PL should serialize initialPlayer as white");

    GameModel handicap = parser.parseGame("(;GM[1]FF[4]SZ[9]HA[2]AB[dd][fd])");
    const BatchAnalysisPlan handicapPlan = buildBatchAnalysisPlan(handicap, {handicap.rootId()});
    require(handicapPlan.items.size() == 1, "handicap root should produce an analysis request");
    require(handicapPlan.items.front().request.initialPlayer == Color::White, "handicap root should analyze white to move");
    require(
        analysisPositionKey(rootPlayer, rootPlayer.rootId()) != analysisPositionKey(GameModel(BoardSize::square(9)), 1),
        "position key should include root player to move");
    require(
        analysisPositionKey(handicap, handicap.rootId()).find("|P:W") != std::string::npos,
        "position key should include inferred handicap player to move");

    GameModel handicapMove = parser.parseGame("(;GM[1]FF[4]SZ[9]HA[2]AB[dd][fd];W[ee])");
    const NodeId moveNode = handicapMove.root().children.front();
    const BatchAnalysisPlan movePlan = buildBatchAnalysisPlan(handicapMove, {moveNode});
    require(movePlan.items.size() == 1, "handicap child should produce an analysis request");
    require(movePlan.items.front().request.initialPlayer == Color::White, "handicap child should keep root initial player");
    require(movePlan.items.front().request.moves.size() == 1, "handicap child should replay the white move");
    require(movePlan.items.front().request.moves.front().color == Color::White, "handicap child move color should stay white");
}

void testFinalPlayerToMoveOverrideUsesSnapshotRequest()
{
    SgfParser parser;
    GameModel model = parser.parseGame("(;GM[1]FF[4]SZ[9];B[dd]PL[B])");
    const NodeId child = model.root().children.front();

    const BatchAnalysisPlan plan = buildBatchAnalysisPlan(model, {child});
    require(plan.items.size() == 1, "planner should analyze final PL overrides with a snapshot request");
    require(plan.warnings.empty(), "final PL snapshot should not warn");

    const AnalysisRequest& request = plan.items.front().request;
    require(request.moves.empty(), "final PL snapshot should not replay moves");
    require(request.initialPlayer == Color::Black, "final PL snapshot should preserve the side to move");
    require(request.initialStones.size() == 1, "final PL snapshot should encode the final board state");
    require(
        containsInitialStone(request.initialStones, Color::Black, Point{3, 3}),
        "final PL snapshot should include the played stone");

    const QJsonObject json = request.toJson();
    require(json.value("initialPlayer").toString() == "B", "final PL snapshot should serialize initialPlayer");
    require(json.value("moves").toArray().empty(), "final PL snapshot should serialize no moves");
}

void testWideBoardAnalysisJsonUsesExtendedCoordinates()
{
    GameModel model(BoardSize{26, 9});
    model.addMove(Move::play(Color::Black, Point{25, 8}));

    const BatchAnalysisPlan plan = buildBatchAnalysisPlan(model, {});
    require(plan.items.size() == 2, "wide board should produce batch analysis requests");
    require(plan.warnings.empty(), "wide board should not be rejected as a GTP-only board");
    require(
        plan.items.back().request.toJson().value("moves").toArray().at(0).toArray().at(1).toString() == "(25,8)",
        "wide board moves should serialize with KataGo analysis x,y coordinates");
}

void testInvalidPositionSkipped()
{
    GameModel model(BoardSize::square(5));
    const NodeId first = model.addMove(Move::play(Color::Black, Point{1, 1}));
    const NodeId illegal = model.addChild(first, Move::play(Color::White, Point{1, 1}));

    const BatchAnalysisPlan explicitPlan = buildBatchAnalysisPlan(model, {illegal});
    require(explicitPlan.items.empty(), "explicit invalid node should not produce an analysis request");
    require(!explicitPlan.warnings.empty(), "explicit invalid node should warn");
    require(
        explicitPlan.warnings.front().find("point is already occupied") != std::string::npos,
        "explicit invalid node warning should include local rule error");

    const BatchAnalysisPlan fullPlan = buildBatchAnalysisPlan(model, {});
    require(fullPlan.items.size() == 2, "full batch plan should retain valid root and first move");
    require(fullPlan.items.front().nodeId == model.rootId(), "full batch plan should include root first");
    require(fullPlan.items.back().nodeId == first, "full batch plan should include valid first move");
    require(fullPlan.warnings.size() == 1, "full batch plan should warn once for invalid descendant");
}

void testInvalidSetupPositionSkipped()
{
    GameModel model(BoardSize::square(5));
    model.root().setupStones.white.push_back(Point{0, 5});

    const BatchAnalysisPlan plan = buildBatchAnalysisPlan(model, {model.rootId()});
    require(plan.items.empty(), "invalid setup position should not produce an analysis request");
    require(!plan.warnings.empty(), "invalid setup position should warn");
    require(
        plan.warnings.front().find("setup stone is outside the board") != std::string::npos,
        "invalid setup batch warning should include local setup error");
}

void testResignNodesSkipped()
{
    GameModel model(BoardSize::square(9));
    const NodeId first = model.addMove(Move::play(Color::Black, Point{3, 3}));
    const NodeId resign = model.addMove(Move::resign(Color::White));
    const NodeId afterResign = model.addChild(resign, Move::pass(Color::Black));

    const BatchAnalysisPlan explicitPlan = buildBatchAnalysisPlan(model, {resign});
    require(explicitPlan.items.empty(), "explicit resign node should not produce an analysis request");
    require(!explicitPlan.warnings.empty(), "explicit resign node should warn");
    require(
        explicitPlan.warnings.front().find("resign") != std::string::npos,
        "explicit resign warning should mention resign");

    const BatchAnalysisPlan fullPlan = buildBatchAnalysisPlan(model, {});
    require(fullPlan.items.size() == 2, "full plan should retain root and moves before resign");
    require(fullPlan.items.front().nodeId == model.rootId(), "full plan should include root before resign");
    require(fullPlan.items.back().nodeId == first, "full plan should include first move before resign");
    require(fullPlan.warnings.size() == 2, "full plan should warn for resign node and descendants");
    require(
        fullPlan.warnings.back().find(std::to_string(afterResign)) != std::string::npos,
        "full plan should warn for descendant after resign");
}

void testApplyAnalysisResponse()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    AnalysisResponse response;
    response.id = analysisRequestId(second);
    response.rootVisits = 80;
    response.rootWinrate = 0.54;
    response.rootScoreMean = 1.7;
    response.ownership = ownershipValues(model.boardSize(), 0.1, -0.1);
    MoveCandidate responseCandidate{Move::play(Color::Black, Point{4, 4}), 80, 0.54, 1.7};
    responseCandidate.ownership = ownershipValues(model.boardSize(), 0.2, -0.2);
    response.moveInfos.push_back(responseCandidate);

    require(applyAnalysisResponse(&model, response), "analysis response should apply to existing node");
    const GameNode& node = model.node(second);
    require(node.analysis.has_value(), "node should receive analysis snapshot");
    require(node.analysis->rootVisits == 80, "root visits should be written");
    require(node.analysis->candidates.size() == 1, "candidate list should be written");
    require(
        node.analysis->ownership.size() == static_cast<std::size_t>(model.boardSize().pointCount()),
        "full-board ownership should be written");
    require(
        node.analysis->candidates.front().ownership.size() ==
            static_cast<std::size_t>(model.boardSize().pointCount()),
        "full-board candidate ownership should be written");

    response.ownership = {0.1, -0.1};
    response.moveInfos.front().ownership = {0.2, -0.2};
    response.rootVisits = 80;
    require(
        applyAnalysisResponse(&model, response),
        "analysis response with incomplete ownership should still apply non-ownership fields");
    require(model.node(second).analysis->ownership.empty(), "incomplete root ownership should be discarded");
    require(
        model.node(second).analysis->candidates.front().ownership.empty(),
        "incomplete candidate ownership should be discarded");

    response.ownership = ownershipValues(model.boardSize(), 0.1, -0.1);
    response.ownership[0] = std::numeric_limits<double>::infinity();
    response.moveInfos.front().ownership = ownershipValues(model.boardSize(), 0.2, -0.2);
    response.moveInfos.front().ownership[1] = std::numeric_limits<double>::quiet_NaN();
    response.rootVisits = 80;
    require(
        applyAnalysisResponse(&model, response),
        "analysis response with non-finite ownership should still apply non-ownership fields");
    require(model.node(second).analysis->ownership.empty(), "non-finite root ownership should be discarded");
    require(
        model.node(second).analysis->candidates.front().ownership.empty(),
        "non-finite candidate ownership should be discarded");

    response.rootVisits = -5;
    response.rootWinrate = std::numeric_limits<double>::infinity();
    response.rootScoreMean = -std::numeric_limits<double>::infinity();
    response.moveInfos.front().visits = -7;
    response.moveInfos.front().winrate = std::numeric_limits<double>::infinity();
    response.moveInfos.front().scoreMean = std::numeric_limits<double>::quiet_NaN();
    response.moveInfos.front().scoreStdev = -std::numeric_limits<double>::infinity();
    response.moveInfos.front().policy = std::numeric_limits<double>::infinity();
    response.moveInfos.front().pv = {
        Move::play(Color::Black, Point{4, 4}),
        Move::pass(Color::White),
    };
    response.moveInfos.front().pvVisits = {-3, 5};
    require(
        applyAnalysisResponse(&model, response),
        "analysis response with non-finite scalars should still apply sanitized fields");
    require(model.node(second).analysis->rootVisits == 0, "negative root visits should be clamped on apply");
    require(model.node(second).analysis->rootWinrate == 0.0, "non-finite root winrate should be sanitized on apply");
    require(model.node(second).analysis->rootScoreMean == 0.0, "non-finite root score should be sanitized on apply");
    const MoveCandidate& sanitizedCandidate = model.node(second).analysis->candidates.front();
    require(sanitizedCandidate.visits == 0, "negative candidate visits should be clamped on apply");
    require(sanitizedCandidate.winrate == 0.0, "non-finite candidate winrate should be sanitized on apply");
    require(sanitizedCandidate.scoreMean == 0.0, "non-finite candidate score should be sanitized on apply");
    require(sanitizedCandidate.scoreStdev == 0.0, "non-finite candidate stdev should be sanitized on apply");
    require(sanitizedCandidate.policy == 0.0, "non-finite candidate policy should be sanitized on apply");
    require(
        sanitizedCandidate.pvVisits.size() == 2 && sanitizedCandidate.pvVisits[0] == 0 &&
            sanitizedCandidate.pvVisits[1] == 5,
        "negative candidate PV visits should be clamped on apply");
    response.ownership = ownershipValues(model.boardSize(), 0.1, -0.1);
    response.moveInfos.front().ownership = ownershipValues(model.boardSize(), 0.2, -0.2);
    response.rootVisits = 80;
    response.rootWinrate = 0.54;
    response.rootScoreMean = 1.7;
    response.moveInfos.front().visits = 80;
    response.moveInfos.front().winrate = 0.54;
    response.moveInfos.front().scoreMean = 1.7;
    response.moveInfos.front().scoreStdev = 0.0;
    response.moveInfos.front().policy = 0.0;
    response.moveInfos.front().pv.clear();
    response.moveInfos.front().pvVisits.clear();

    AnalysisResponse unorderedResponse;
    unorderedResponse.id = analysisRequestId(second);
    unorderedResponse.rootVisits = 0;
    unorderedResponse.rootWinrate = std::numeric_limits<double>::infinity();
    unorderedResponse.rootScoreMean = std::numeric_limits<double>::quiet_NaN();
    unorderedResponse.moveInfos.push_back(MoveCandidate{Move::play(Color::Black, Point{4, 4}), 12, 0.44, -1.0});
    unorderedResponse.moveInfos.push_back(MoveCandidate{Move::play(Color::Black, Point{3, 3}), 40, 0.57, 2.4});
    require(
        applyAnalysisResponse(&model, unorderedResponse),
        "analysis response should sort candidates and fill invalid root fields");
    const AnalysisSnapshot& sortedAnalysis = *model.node(second).analysis;
    require(sortedAnalysis.candidates.size() == 2, "direct response should keep both candidates");
    require(
        sortedAnalysis.candidates.front().move.point == Point{3, 3},
        "direct response candidates should sort by visits");
    require(sortedAnalysis.rootVisits == 40, "direct response root visits should fall back to best candidate");
    require(sortedAnalysis.rootWinrate == 0.57, "direct response root winrate should fall back to best candidate");
    require(sortedAnalysis.rootScoreMean == 2.4, "direct response root score should fall back to best candidate");

    response.id = "bad-id";
    require(!applyAnalysisResponse(&model, response), "invalid response id should not apply");

    response.id = analysisRequestId(second);
    response.errorMessage = "analysis failed";
    require(!applyAnalysisResponse(&model, response), "error response should not apply an empty analysis snapshot");

    response.errorMessage.clear();
    response.rootVisits = 81;
    const std::string originalPositionKey = analysisPositionKey(model, second);
    require(
        applyAnalysisResponse(&model, response, originalPositionKey),
        "analysis response should apply when expected position key matches");
    require(model.node(second).analysis->rootVisits == 81, "matching position-key response should update analysis");

    model.node(first).move = Move::play(Color::Black, Point{4, 4});
    response.rootVisits = 82;
    require(
        !applyAnalysisResponse(&model, response, originalPositionKey),
        "analysis response should not apply after the node position changes");
    require(
        model.node(second).analysis->rootVisits == 81,
        "stale position-key response should not overwrite existing analysis");
    require(
        applyAnalysisResponse(&model, response, analysisPositionKey(model, second)),
        "analysis response should apply again with the updated position key");
}

void testAnalysisSidecarRoundTrip()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    AnalysisResponse response;
    response.id = analysisRequestId(second);
    response.rootVisits = 96;
    response.rootWinrate = 0.61;
    response.rootScoreMean = 3.2;
    response.winratePerspective = Color::Black;
    response.ownership = ownershipValues(model.boardSize(), 0.25, -0.25);
    MoveCandidate candidate{Move::play(Color::Black, Point{4, 4}), 96, 0.61, 3.2};
    candidate.pv = {Move::play(Color::Black, Point{4, 4}), Move::pass(Color::White)};
    candidate.pvVisits = {96, 42};
    candidate.ownership = ownershipValues(model.boardSize(), 0.1, -0.1);
    response.moveInfos.push_back(candidate);
    require(applyAnalysisResponse(&model, response), "analysis response should apply before sidecar export");

    const QJsonObject sidecar = analysisSidecarToJson(model, "game.sgf");
    require(sidecar.value("format").toString() == "lizzieyzy-analysis-sidecar-v1", "sidecar format should be written");
    require(!sidecar.contains("collectionGameIndex"), "single-game sidecars should not include a collection index");
    require(sidecar.value("nodes").toArray().size() == 1, "sidecar should include analyzed nodes");
    require(
        !sidecar.value("nodes").toArray().at(0).toObject().value("positionKey").toString().isEmpty(),
        "sidecar should include a position key for analyzed nodes");
    require(
        sidecar.value("nodes")
                .toArray()
                .at(0)
                .toObject()
                .value("analysis")
                .toObject()
                .value("winratePerspective")
                .toString() == "B",
        "sidecar should write the analysis winrate perspective");

    NodeId freshFirst = 0;
    NodeId freshSecond = 0;
    GameModel fresh = sampleModel(&freshFirst, &freshSecond);
    fresh.node(freshSecond).analysisError = "old sidecar failure";
    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&fresh, sidecar, &warnings);
    require(applied == 1, "sidecar should apply one node");
    require(warnings.empty(), "round-trip sidecar should not warn");

    const GameNode& restored = fresh.node(freshSecond);
    require(restored.analysis.has_value(), "sidecar should restore node analysis");
    require(restored.analysis->rootVisits == 96, "sidecar should restore root visits");
    require(
        restored.analysis->rootWinrate > 0.60 && restored.analysis->rootWinrate < 0.62,
        "sidecar should restore root winrate");
    require(
        restored.analysis->winratePerspective == Color::Black,
        "sidecar should restore the winrate perspective");
    require(restored.analysis->candidates.size() == 1, "sidecar should restore candidates");
    require(restored.analysis->candidates.front().move.point == Point{4, 4}, "sidecar should restore candidate move");
    require(restored.analysis->candidates.front().pv.size() == 2, "sidecar should restore PV");
    require(restored.analysis->candidates.front().pvVisits.size() == 2, "sidecar should restore PV visits");
    require(restored.analysis->candidates.front().pvVisits.front() == 96, "sidecar should restore PV visit values");
    require(
        restored.analysis->ownership.size() == static_cast<std::size_t>(model.boardSize().pointCount()),
        "sidecar should restore full-board ownership");
    require(restored.analysisError.empty(), "sidecar should clear stale node analysis errors on success");

    NodeId changedFirst = 0;
    NodeId changedSecond = 0;
    GameModel changed = sampleModel(&changedFirst, &changedSecond);
    changed.node(changedFirst).move = Move::play(Color::Black, Point{4, 4});
    std::vector<std::string> changedWarnings;
    const int changedApplied = applyAnalysisSidecarJson(&changed, sidecar, &changedWarnings);
    require(changedApplied == 0, "sidecar should skip nodes whose position key does not match");
    require(!changedWarnings.empty(), "sidecar position mismatch should warn");
    require(
        changedWarnings.front().find("does not match current position") != std::string::npos,
        "sidecar position mismatch warning should explain the mismatch");
    require(!changed.node(changedSecond).analysis.has_value(), "mismatched sidecar analysis should not be applied");

    QJsonObject legacySidecar = sidecar;
    QJsonArray legacyNodes = legacySidecar.value("nodes").toArray();
    QJsonObject legacyNode = legacyNodes.at(0).toObject();
    legacyNode.remove("positionKey");
    legacyNodes.replace(0, legacyNode);
    legacySidecar.insert("nodes", legacyNodes);
    NodeId legacyFirst = 0;
    NodeId legacySecond = 0;
    GameModel legacyTarget = sampleModel(&legacyFirst, &legacySecond);
    std::vector<std::string> legacyWarnings;
    const int legacyApplied = applyAnalysisSidecarJson(&legacyTarget, legacySidecar, &legacyWarnings);
    require(legacyApplied == 1, "legacy sidecars without position keys should remain importable");
    require(legacyWarnings.empty(), "legacy sidecar compatibility import should not warn");
    require(legacyTarget.node(legacySecond).analysis.has_value(), "legacy sidecar should restore analysis");

    QJsonObject legacyKeyedSidecar = sidecar;
    QJsonArray legacyKeyedNodes = legacyKeyedSidecar.value("nodes").toArray();
    QJsonObject legacyKeyedNode = legacyKeyedNodes.at(0).toObject();
    QString legacyKey = legacyKeyedNode.value("positionKey").toString();
    legacyKey.remove("|P:B");
    legacyKey.remove("|P:W");
    legacyKeyedNode.insert("positionKey", legacyKey);
    legacyKeyedNodes.replace(0, legacyKeyedNode);
    legacyKeyedSidecar.insert("nodes", legacyKeyedNodes);
    NodeId legacyKeyedFirst = 0;
    NodeId legacyKeyedSecond = 0;
    GameModel legacyKeyedTarget = sampleModel(&legacyKeyedFirst, &legacyKeyedSecond);
    std::vector<std::string> legacyKeyedWarnings;
    const int legacyKeyedApplied =
        applyAnalysisSidecarJson(&legacyKeyedTarget, legacyKeyedSidecar, &legacyKeyedWarnings);
    require(legacyKeyedApplied == 1, "legacy keyed sidecars without player-to-move should remain importable");
    require(legacyKeyedWarnings.empty(), "legacy keyed sidecar compatibility import should not warn");
}

void testAnalysisSidecarExportRejectsNonFiniteValues()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);
    const double infinity = std::numeric_limits<double>::infinity();
    const double quietNaN = std::numeric_limits<double>::quiet_NaN();

    AnalysisSnapshot analysis;
    analysis.rootVisits = 44;
    analysis.rootWinrate = infinity;
    analysis.rootScoreMean = -infinity;
    analysis.ownership = ownershipValues(model.boardSize(), 0.1, -0.1);
    analysis.ownership[7] = infinity;

    MoveCandidate candidate{Move::play(Color::Black, Point{3, 3}), 44, infinity, quietNaN};
    candidate.scoreStdev = infinity;
    candidate.policy = infinity;
    candidate.ownership = ownershipValues(model.boardSize(), 0.2, -0.2);
    candidate.ownership[9] = -infinity;
    analysis.candidates.push_back(candidate);
    model.node(second).analysis = analysis;

    const QJsonObject sidecar = analysisSidecarToJson(model, "game.sgf");
    const QJsonObject analysisJson =
        sidecar.value("nodes").toArray().at(0).toObject().value("analysis").toObject();
    require(analysisJson.value("rootWinrate").toDouble(-1.0) == 0.0, "sidecar should sanitize root winrate");
    require(analysisJson.value("rootScoreMean").toDouble(-1.0) == 0.0, "sidecar should sanitize root score");
    require(analysisJson.value("ownership").toArray().isEmpty(), "sidecar should drop non-finite root ownership");

    const QJsonObject candidateJson = analysisJson.value("candidates").toArray().at(0).toObject();
    require(candidateJson.value("winrate").toDouble(-1.0) == 0.0, "sidecar should sanitize candidate winrate");
    require(candidateJson.value("scoreMean").toDouble(-1.0) == 0.0, "sidecar should sanitize candidate score");
    require(candidateJson.value("scoreStdev").toDouble(-1.0) == 0.0, "sidecar should sanitize candidate stdev");
    require(candidateJson.value("policy").toDouble(-1.0) == 0.0, "sidecar should sanitize candidate policy");
    require(candidateJson.value("ownership").toArray().isEmpty(), "sidecar should drop non-finite candidate ownership");
}

void testAnalysisSidecarExportFiltersIncompleteOwnership()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    AnalysisSnapshot analysis;
    analysis.rootVisits = 10;
    analysis.rootWinrate = 0.55;
    analysis.rootScoreMean = 1.1;
    analysis.ownership = {0.1, -0.1};
    MoveCandidate candidate{Move::play(Color::Black, Point{4, 4}), 10, 0.55, 1.1};
    candidate.ownership = {0.2, -0.2};
    analysis.candidates.push_back(candidate);
    model.node(second).analysis = analysis;

    const QJsonObject incompleteSidecar = analysisSidecarToJson(model, "game.sgf");
    const QJsonObject incompleteAnalysis =
        incompleteSidecar.value("nodes").toArray().at(0).toObject().value("analysis").toObject();
    require(
        incompleteAnalysis.value("ownership").toArray().isEmpty(),
        "sidecar should not write incomplete root ownership");
    const QJsonObject incompleteCandidate = incompleteAnalysis.value("candidates").toArray().at(0).toObject();
    require(
        incompleteCandidate.value("ownership").toArray().isEmpty(),
        "sidecar should not write incomplete candidate ownership");

    model.node(second).analysis->ownership = ownershipValues(model.boardSize(), 0.1, -0.1);
    model.node(second).analysis->candidates.front().ownership = ownershipValues(model.boardSize(), 0.2, -0.2);

    const QJsonObject completeSidecar = analysisSidecarToJson(model, "game.sgf");
    const QJsonObject completeAnalysis =
        completeSidecar.value("nodes").toArray().at(0).toObject().value("analysis").toObject();
    require(
        completeAnalysis.value("ownership").toArray().size() == model.boardSize().pointCount(),
        "sidecar should write complete root ownership");
    const QJsonObject completeCandidate = completeAnalysis.value("candidates").toArray().at(0).toObject();
    require(
        completeCandidate.value("ownership").toArray().size() == model.boardSize().pointCount(),
        "sidecar should write complete candidate ownership");
}

void testAnalysisSidecarImportRejectsMalformedOwnershipArrays()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    AnalysisSnapshot analysis;
    analysis.rootVisits = 42;
    analysis.rootWinrate = 0.56;
    analysis.rootScoreMean = 1.7;
    analysis.ownership = ownershipValues(model.boardSize(), 0.1, -0.1);
    MoveCandidate candidate{Move::play(Color::Black, Point{4, 4}), 42, 0.56, 1.7};
    candidate.ownership = ownershipValues(model.boardSize(), 0.2, -0.2);
    analysis.candidates.push_back(candidate);
    model.node(second).analysis = analysis;

    QJsonObject sidecar = analysisSidecarToJson(model, "game.sgf");
    QJsonArray nodes = sidecar.value("nodes").toArray();
    QJsonObject nodeObject = nodes.at(0).toObject();
    QJsonObject analysisObject = nodeObject.value("analysis").toObject();

    QJsonArray rootOwnership = analysisObject.value("ownership").toArray();
    rootOwnership.push_back("bad");
    analysisObject.insert("ownership", rootOwnership);

    QJsonArray candidates = analysisObject.value("candidates").toArray();
    QJsonObject candidateObject = candidates.at(0).toObject();
    QJsonArray candidateOwnership = candidateObject.value("ownership").toArray();
    candidateOwnership.push_back("bad");
    candidateObject.insert("ownership", candidateOwnership);
    candidates.replace(0, candidateObject);
    analysisObject.insert("candidates", candidates);
    nodeObject.insert("analysis", analysisObject);
    nodes.replace(0, nodeObject);
    sidecar.insert("nodes", nodes);

    NodeId freshFirst = 0;
    NodeId freshSecond = 0;
    GameModel fresh = sampleModel(&freshFirst, &freshSecond);
    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&fresh, sidecar, &warnings);

    require(applied == 1, "sidecar with malformed ownership should still apply scalar analysis");
    require(warnings.empty(), "malformed sidecar ownership should not warn when analysis remains usable");
    const GameNode& restored = fresh.node(freshSecond);
    require(restored.analysis.has_value(), "sidecar with malformed ownership should restore analysis");
    require(restored.analysis->rootVisits == 42, "sidecar with malformed ownership should restore root visits");
    require(restored.analysis->ownership.empty(), "malformed root ownership should be discarded on import");
    require(restored.analysis->candidates.size() == 1, "sidecar with malformed ownership should restore candidates");
    require(
        restored.analysis->candidates.front().ownership.empty(),
        "malformed candidate ownership should be discarded on import");
}

void testAnalysisSidecarExportSortsCandidates()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    AnalysisSnapshot analysis;
    analysis.rootVisits = 120;
    analysis.rootWinrate = 0.55;
    analysis.rootScoreMean = 2.1;
    analysis.candidates.push_back(MoveCandidate{Move::pass(Color::Black), 40, 0.47, -1.0});
    analysis.candidates.push_back(MoveCandidate{Move::play(Color::Black, Point{4, 4}), 50, 0.48, -0.6});
    analysis.candidates.push_back(MoveCandidate{Move::play(Color::Black, Point{3, 3}), 120, 0.55, 2.1});
    analysis.candidates.push_back(MoveCandidate{Move::play(Color::Black, Point{2, 2}), 120, 0.53, 1.8});
    model.node(second).analysis = analysis;

    const QJsonObject sidecar = analysisSidecarToJson(model, "game.sgf");
    const QJsonArray candidates = sidecar.value("nodes")
                                      .toArray()
                                      .at(0)
                                      .toObject()
                                      .value("analysis")
                                      .toObject()
                                      .value("candidates")
                                      .toArray();
    require(candidates.size() == 4, "sidecar export should keep all candidates");
    require(candidates.at(0).toObject().value("move").toString() == "cc", "sidecar export should sort tied leaders");
    require(candidates.at(1).toObject().value("move").toString() == "dd", "sidecar export should sort second tie");
    require(
        candidates.at(2).toObject().value("move").toString() == "ee",
        "sidecar export should sort lower visits next");
    require(candidates.at(3).toObject().value("move").toString() == "pass", "sidecar export should sort pass last");
    require(
        model.node(second).analysis->candidates.front().move.type == MoveType::Pass,
        "sidecar export sorting should not mutate the in-memory candidate order");
}

void testAnalysisSidecarExportClampsNegativeVisits()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    AnalysisSnapshot analysis;
    analysis.rootVisits = -12;
    analysis.rootWinrate = 0.5;
    analysis.rootScoreMean = 0.0;
    MoveCandidate candidate{Move::play(Color::Black, Point{3, 3}), -7, 0.5, 0.0};
    candidate.pvVisits = {-3, 5};
    analysis.candidates.push_back(candidate);
    model.node(second).analysis = analysis;

    const QJsonObject sidecar = analysisSidecarToJson(model, "game.sgf");
    const QJsonObject analysisJson =
        sidecar.value("nodes").toArray().at(0).toObject().value("analysis").toObject();
    require(analysisJson.value("rootVisits").toInt(-1) == 0, "sidecar should clamp root visits");

    const QJsonObject candidateJson = analysisJson.value("candidates").toArray().at(0).toObject();
    require(candidateJson.value("visits").toInt(-1) == 0, "sidecar should clamp candidate visits");
    const QJsonArray pvVisits = candidateJson.value("pvVisits").toArray();
    require(pvVisits.size() == 2, "sidecar should keep PV visit positions while clamping values");
    require(pvVisits.at(0).toInt(-1) == 0 && pvVisits.at(1).toInt(-1) == 5, "sidecar should clamp PV visits");
}

void testAnalysisSidecarTakesPrecedenceOverLegacySgfAnalysis()
{
    const std::string sgf = R"SGF((;GM[1]FF[4]SZ[9]KM[6.5]RU[Chinese];B[df]LZ[KataGo 45.0 12 0.4 1.5
move D4 visits 12 winrate 5500 prior 1200 scoreMean 0.40 pv D4 E4]))SGF";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    const NodeId childId = model.root().children.front();

    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 1, "legacy SGF analysis should import before sidecar load");
    require(warnings.empty(), "valid legacy SGF analysis should not warn");
    require(model.node(childId).analysis.has_value(), "legacy SGF analysis should populate the node");
    require(model.node(childId).analysis->rootVisits == 12, "legacy SGF analysis should set the old visit count");

    QJsonObject sidecarAnalysis;
    sidecarAnalysis.insert("rootVisits", 88);
    sidecarAnalysis.insert("rootWinrate", 0.61);
    sidecarAnalysis.insert("rootScoreMean", 3.5);
    sidecarAnalysis.insert("winratePerspective", "B");
    QJsonObject sidecarCandidate;
    sidecarCandidate.insert("move", "ee");
    sidecarCandidate.insert("visits", 88);
    sidecarCandidate.insert("winrate", 0.61);
    sidecarCandidate.insert("scoreMean", 3.5);
    sidecarCandidate.insert("policy", 0.2);
    QJsonArray sidecarCandidates;
    sidecarCandidates.push_back(sidecarCandidate);
    sidecarAnalysis.insert("candidates", sidecarCandidates);

    QJsonObject sidecarNode;
    sidecarNode.insert("nodeId", QString::number(childId));
    sidecarNode.insert("positionKey", QString::fromStdString(analysisPositionKey(model, childId)));
    sidecarNode.insert("analysis", sidecarAnalysis);
    QJsonArray sidecarNodes;
    sidecarNodes.push_back(sidecarNode);

    QJsonObject sidecar;
    sidecar.insert("format", "lizzieyzy-analysis-sidecar-v1");
    sidecar.insert("nodes", sidecarNodes);

    warnings.clear();
    const int applied = applyAnalysisSidecarJson(&model, sidecar, &warnings);
    require(applied == 1, "sidecar should apply after legacy SGF analysis import");
    require(warnings.empty(), "matching sidecar should not warn after legacy import");
    const AnalysisSnapshot& restored = *model.node(childId).analysis;
    require(restored.rootVisits == 88, "sidecar should overwrite legacy SGF root visits");
    require(restored.rootWinrate > 0.609 && restored.rootWinrate < 0.611, "sidecar should overwrite legacy SGF winrate");
    require(restored.rootScoreMean > 3.49 && restored.rootScoreMean < 3.51, "sidecar should overwrite legacy SGF score");
    require(restored.candidates.size() == 1, "sidecar should replace legacy SGF candidates");
    require(restored.candidates.front().move.point == Point{4, 4}, "sidecar should replace legacy SGF candidate moves");
    require(model.node(childId).analysisError.empty(), "sidecar success should leave no stale analysis error");
}

void testAnalysisSidecarNormalizesRateScales()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    QJsonObject candidate;
    candidate.insert("move", "ee");
    candidate.insert("visits", 77);
    candidate.insert("winrate", 61.23);
    candidate.insert("scoreMean", 2.5);
    candidate.insert("scoreStdev", 4.0);
    candidate.insert("policy", 2222);
    QJsonArray candidates;
    candidates.push_back(candidate);

    QJsonObject analysis;
    analysis.insert("rootVisits", 77);
    analysis.insert("rootWinrate", 6345);
    analysis.insert("rootScoreMean", 2.5);
    analysis.insert("candidates", candidates);

    QJsonObject node;
    node.insert("nodeId", QString::number(second));
    node.insert("positionKey", QString::fromStdString(analysisPositionKey(model, second)));
    node.insert("analysis", analysis);
    QJsonArray nodes;
    nodes.push_back(node);

    QJsonObject sidecar;
    sidecar.insert("format", "lizzieyzy-analysis-sidecar-v1");
    sidecar.insert("nodes", nodes);

    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&model, sidecar, &warnings);
    require(applied == 1, "legacy-scaled sidecar should apply");
    require(warnings.empty(), "legacy-scaled sidecar should not warn");
    require(model.node(second).analysis.has_value(), "legacy-scaled sidecar should restore analysis");

    const AnalysisSnapshot& restored = *model.node(second).analysis;
    require(restored.rootWinrate > 0.6344 && restored.rootWinrate < 0.6346, "sidecar root winrate should normalize");
    require(restored.candidates.size() == 1, "legacy-scaled sidecar should restore one candidate");
    require(
        restored.candidates.front().winrate > 0.6122 && restored.candidates.front().winrate < 0.6124,
        "sidecar candidate winrate should normalize");
    require(
        restored.candidates.front().policy > 0.2221 && restored.candidates.front().policy < 0.2223,
        "sidecar candidate policy should normalize");
}

void testAnalysisSidecarImportsNumericStringFields()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    QJsonObject candidate;
    candidate.insert("move", "dd");
    candidate.insert("visits", QStringLiteral(" 44 "));
    candidate.insert("winrate", QStringLiteral("61.23"));
    candidate.insert("scoreLead", QStringLiteral("1.25"));
    candidate.insert("scoreStdev", QStringLiteral("4.5"));
    candidate.insert("prior", QStringLiteral("12.5"));
    QJsonArray pv;
    pv.push_back(QStringLiteral("dd"));
    pv.push_back(QStringLiteral("ee"));
    candidate.insert("pv", pv);
    QJsonArray pvVisits;
    pvVisits.push_back(QStringLiteral(" 44 "));
    pvVisits.push_back(QStringLiteral(" 22 "));
    candidate.insert("pvVisits", pvVisits);
    QJsonArray candidateOwnership;
    for (int index = 0; index < model.boardSize().pointCount(); ++index) {
        candidateOwnership.push_back(index % 2 == 0 ? QStringLiteral("0.3") : QStringLiteral("-0.3"));
    }
    candidate.insert("ownership", candidateOwnership);
    QJsonArray candidates;
    candidates.push_back(candidate);

    QJsonObject analysis;
    analysis.insert("rootVisits", QStringLiteral(" 44 "));
    analysis.insert("rootWinrate", QStringLiteral("63.45"));
    analysis.insert("rootScoreLead", QStringLiteral("2.75"));
    QJsonArray rootOwnership;
    for (int index = 0; index < model.boardSize().pointCount(); ++index) {
        rootOwnership.push_back(index % 2 == 0 ? QStringLiteral("0.1") : QStringLiteral("-0.1"));
    }
    analysis.insert("ownership", rootOwnership);
    analysis.insert("candidates", candidates);

    QJsonObject node;
    node.insert("nodeId", QStringLiteral(" %1 ").arg(second));
    node.insert("positionKey", QString::fromStdString(analysisPositionKey(model, second)));
    node.insert("analysis", analysis);
    QJsonArray nodes;
    nodes.push_back(node);

    QJsonObject sidecar;
    sidecar.insert("format", "lizzieyzy-analysis-sidecar-v1");
    sidecar.insert("nodes", nodes);

    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&model, sidecar, &warnings);
    require(applied == 1, "numeric-string sidecar should apply");
    require(warnings.empty(), "numeric-string sidecar should not warn");
    require(model.node(second).analysis.has_value(), "numeric-string sidecar should restore analysis");

    const AnalysisSnapshot& restored = *model.node(second).analysis;
    require(restored.rootVisits == 44, "numeric-string sidecar root visits should parse");
    require(
        restored.rootWinrate > 0.6344 && restored.rootWinrate < 0.6346,
        "numeric-string sidecar root winrate should normalize");
    require(
        restored.rootScoreMean > 2.74 && restored.rootScoreMean < 2.76,
        "numeric-string sidecar root score should parse");
    require(
        restored.ownership.size() == static_cast<std::size_t>(model.boardSize().pointCount()) &&
            restored.ownership[0] == 0.1 && restored.ownership[1] == -0.1,
        "numeric-string sidecar root ownership should parse");
    require(restored.candidates.size() == 1, "numeric-string sidecar should restore one candidate");
    const MoveCandidate& restoredCandidate = restored.candidates.front();
    require(restoredCandidate.visits == 44, "numeric-string sidecar candidate visits should parse");
    require(
        restoredCandidate.pvVisits.size() == 2 && restoredCandidate.pvVisits[0] == 44 &&
            restoredCandidate.pvVisits[1] == 22,
        "numeric-string sidecar PV visits should parse");
    require(
        restoredCandidate.winrate > 0.6122 && restoredCandidate.winrate < 0.6124,
        "numeric-string sidecar candidate winrate should normalize");
    require(
        restoredCandidate.scoreMean > 1.24 && restoredCandidate.scoreMean < 1.26,
        "numeric-string sidecar candidate scoreLead should parse");
    require(
        restoredCandidate.scoreStdev > 4.49 && restoredCandidate.scoreStdev < 4.51,
        "numeric-string sidecar candidate scoreStdev should parse");
    require(
        restoredCandidate.policy > 0.124 && restoredCandidate.policy < 0.126,
        "numeric-string sidecar candidate prior should normalize");
    require(
        restoredCandidate.ownership.size() == static_cast<std::size_t>(model.boardSize().pointCount()) &&
            restoredCandidate.ownership[0] == 0.3 && restoredCandidate.ownership[1] == -0.3,
        "numeric-string sidecar candidate ownership should parse");
}

void testAnalysisSidecarImportsKataGoAliasFields()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    QJsonObject candidate;
    candidate.insert("move", "ee");
    candidate.insert("visits", 77);
    candidate.insert("winrate", 0.61);
    candidate.insert("scoreLead", 2.4);
    candidate.insert("scoreStdev", 4.0);
    candidate.insert("prior", 1234);
    QJsonArray candidates;
    candidates.push_back(candidate);

    QJsonObject analysis;
    analysis.insert("rootVisits", 77);
    analysis.insert("rootWinrate", 0.63);
    analysis.insert("rootScoreLead", 2.5);
    analysis.insert("candidates", candidates);

    QJsonObject node;
    node.insert("nodeId", QString::number(second));
    node.insert("positionKey", QString::fromStdString(analysisPositionKey(model, second)));
    node.insert("analysis", analysis);
    QJsonArray nodes;
    nodes.push_back(node);

    QJsonObject sidecar;
    sidecar.insert("format", "lizzieyzy-analysis-sidecar-v1");
    sidecar.insert("nodes", nodes);

    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&model, sidecar, &warnings);
    require(applied == 1, "KataGo-alias sidecar should apply");
    require(warnings.empty(), "KataGo-alias sidecar should not warn");
    require(model.node(second).analysis.has_value(), "KataGo-alias sidecar should restore analysis");

    const AnalysisSnapshot& restored = *model.node(second).analysis;
    require(restored.rootScoreMean > 2.49 && restored.rootScoreMean < 2.51, "sidecar root scoreLead should import");
    require(restored.candidates.size() == 1, "KataGo-alias sidecar should restore one candidate");
    require(
        restored.candidates.front().scoreMean > 2.39 && restored.candidates.front().scoreMean < 2.41,
        "sidecar candidate scoreLead should import");
    require(
        restored.candidates.front().policy > 0.1233 && restored.candidates.front().policy < 0.1235,
        "sidecar candidate prior should normalize");
}

void testAnalysisSidecarDiscardsIncompleteOwnership()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    QJsonObject candidate;
    candidate.insert("move", "ee");
    candidate.insert("visits", 77);
    candidate.insert("winrate", 0.61);
    candidate.insert("scoreMean", 2.5);
    QJsonArray candidateOwnership;
    candidateOwnership.push_back(0.2);
    candidateOwnership.push_back(-0.2);
    candidate.insert("ownership", candidateOwnership);
    QJsonArray candidates;
    candidates.push_back(candidate);

    QJsonArray rootOwnership;
    rootOwnership.push_back(0.1);
    rootOwnership.push_back(-0.1);
    QJsonObject analysis;
    analysis.insert("rootVisits", 77);
    analysis.insert("rootWinrate", 0.61);
    analysis.insert("rootScoreMean", 2.5);
    analysis.insert("ownership", rootOwnership);
    analysis.insert("candidates", candidates);

    QJsonObject node;
    node.insert("nodeId", QString::number(second));
    node.insert("positionKey", QString::fromStdString(analysisPositionKey(model, second)));
    node.insert("analysis", analysis);
    QJsonArray nodes;
    nodes.push_back(node);

    QJsonObject sidecar;
    sidecar.insert("format", "lizzieyzy-analysis-sidecar-v1");
    sidecar.insert("nodes", nodes);

    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&model, sidecar, &warnings);
    require(applied == 1, "sidecar with incomplete ownership should still apply non-ownership fields");
    require(warnings.empty(), "sidecar with incomplete ownership should not warn");
    require(model.node(second).analysis.has_value(), "sidecar with incomplete ownership should restore analysis");
    require(model.node(second).analysis->ownership.empty(), "sidecar incomplete root ownership should be discarded");
    require(
        model.node(second).analysis->candidates.front().ownership.empty(),
        "sidecar incomplete candidate ownership should be discarded");
}

void testAnalysisSidecarSortsCandidatesAndFallsBackToBest()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    const auto makeCandidate = [](QString move, int visits, double winrate, double scoreMean) {
        QJsonObject candidate;
        candidate.insert("move", move);
        candidate.insert("visits", visits);
        candidate.insert("winrate", winrate);
        candidate.insert("scoreMean", scoreMean);
        candidate.insert("scoreStdev", 4.0);
        candidate.insert("policy", 0.1);
        return candidate;
    };

    QJsonArray candidates;
    candidates.push_back(makeCandidate("ee", 50, 0.48, -0.6));
    candidates.push_back(makeCandidate("dd", 120, 0.55, 2.1));
    candidates.push_back(makeCandidate("cc", 120, 0.53, 1.8));
    candidates.push_back(makeCandidate("pass", 40, 0.47, -1.0));

    QJsonObject analysis;
    analysis.insert("candidates", candidates);

    QJsonObject node;
    node.insert("nodeId", QString::number(second));
    node.insert("positionKey", QString::fromStdString(analysisPositionKey(model, second)));
    node.insert("analysis", analysis);
    QJsonArray nodes;
    nodes.push_back(node);

    QJsonObject sidecar;
    sidecar.insert("format", "lizzieyzy-analysis-sidecar-v1");
    sidecar.insert("nodes", nodes);

    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&model, sidecar, &warnings);
    require(applied == 1, "candidate-only sidecar should apply");
    require(warnings.empty(), "candidate-only sidecar should not warn");
    require(model.node(second).analysis.has_value(), "candidate-only sidecar should restore analysis");

    const AnalysisSnapshot& restored = *model.node(second).analysis;
    require(restored.candidates.size() == 4, "sidecar should retain all valid candidates");
    require(
        restored.candidates.at(0).move.type == MoveType::Play && restored.candidates.at(0).move.point == Point{2, 2},
        "sidecar candidate ties should sort by move key after visits");
    require(
        restored.candidates.at(1).move.type == MoveType::Play && restored.candidates.at(1).move.point == Point{3, 3},
        "sidecar second tied candidate should follow move-key order");
    require(
        restored.candidates.at(2).move.type == MoveType::Play && restored.candidates.at(2).move.point == Point{4, 4},
        "sidecar lower-visit play candidate should follow tied leaders");
    require(restored.candidates.at(3).move.type == MoveType::Pass, "sidecar lowest-visit pass candidate should sort last");
    require(restored.rootVisits == 120, "candidate-only sidecar should fall back to the best candidate visits");
    require(restored.rootWinrate == 0.53, "candidate-only sidecar should fall back to the best candidate winrate");
    require(restored.rootScoreMean == 1.8, "candidate-only sidecar should fall back to the best candidate score");
}

void testAnalysisSidecarRejectsInvalidVisitCounts()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    QJsonObject invalidCandidate;
    invalidCandidate.insert("move", "ee");
    invalidCandidate.insert("visits", 120.5);
    invalidCandidate.insert("winrate", 0.61);
    invalidCandidate.insert("scoreMean", 2.5);

    QJsonArray pvVisits;
    pvVisits.push_back(80);
    pvVisits.push_back(40.5);
    pvVisits.push_back(-1);
    pvVisits.push_back(QStringLiteral("20"));
    QJsonObject validCandidate;
    validCandidate.insert("move", "dd");
    validCandidate.insert("visits", 80);
    validCandidate.insert("winrate", 0.55);
    validCandidate.insert("scoreMean", 1.8);
    validCandidate.insert("pvVisits", pvVisits);

    QJsonArray candidates;
    candidates.push_back(invalidCandidate);
    candidates.push_back(validCandidate);

    QJsonObject analysis;
    analysis.insert("rootVisits", 12.5);
    analysis.insert("candidates", candidates);

    QJsonObject node;
    node.insert("nodeId", QString::number(second));
    node.insert("positionKey", QString::fromStdString(analysisPositionKey(model, second)));
    node.insert("analysis", analysis);
    QJsonArray nodes;
    nodes.push_back(node);

    QJsonObject sidecar;
    sidecar.insert("format", "lizzieyzy-analysis-sidecar-v1");
    sidecar.insert("nodes", nodes);

    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&model, sidecar, &warnings);
    require(applied == 1, "sidecar should apply when at least one candidate has valid integer visits");
    require(warnings.empty(), "sidecar with recoverable invalid visit counts should not warn");
    require(model.node(second).analysis.has_value(), "sidecar should restore recoverable analysis");

    const AnalysisSnapshot& restored = *model.node(second).analysis;
    require(restored.candidates.size() == 1, "sidecar should skip candidates with fractional visits");
    require(restored.candidates.front().move.point == Point{3, 3}, "valid sidecar candidate should remain");
    require(restored.rootVisits == 80, "invalid rootVisits should fall back to the valid candidate visits");
    require(
        restored.candidates.front().pvVisits.empty(),
        "sidecar pvVisits with invalid entries should be discarded");

    NodeId invalidFirst = 0;
    NodeId invalidSecond = 0;
    GameModel invalidOnlyModel = sampleModel(&invalidFirst, &invalidSecond);
    QJsonObject invalidOnlyAnalysis;
    invalidOnlyAnalysis.insert("rootVisits", 12.5);
    QJsonObject invalidOnlyNode;
    invalidOnlyNode.insert("nodeId", QString::number(invalidSecond));
    invalidOnlyNode.insert("positionKey", QString::fromStdString(analysisPositionKey(invalidOnlyModel, invalidSecond)));
    invalidOnlyNode.insert("analysis", invalidOnlyAnalysis);
    QJsonArray invalidOnlyNodes;
    invalidOnlyNodes.push_back(invalidOnlyNode);
    QJsonObject invalidOnlySidecar;
    invalidOnlySidecar.insert("format", "lizzieyzy-analysis-sidecar-v1");
    invalidOnlySidecar.insert("nodes", invalidOnlyNodes);

    std::vector<std::string> invalidOnlyWarnings;
    const int invalidOnlyApplied =
        applyAnalysisSidecarJson(&invalidOnlyModel, invalidOnlySidecar, &invalidOnlyWarnings);
    require(invalidOnlyApplied == 0, "sidecar with only fractional rootVisits should not apply");
    require(!invalidOnlyWarnings.empty(), "unusable sidecar visit counts should warn");
    require(
        invalidOnlyWarnings.front().find("has no usable analysis") != std::string::npos,
        "unusable sidecar visit-count warning should explain the unusable analysis");
    require(!invalidOnlyModel.node(invalidSecond).analysis.has_value(), "unusable sidecar should not restore analysis");
}

void testAnalysisSidecarDiscardsMismatchedPvVisits()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    QJsonArray pv;
    pv.push_back(QStringLiteral("dd"));
    pv.push_back(QStringLiteral("ee"));
    QJsonArray pvVisits;
    pvVisits.push_back(80);

    QJsonObject candidate;
    candidate.insert("move", "dd");
    candidate.insert("visits", 80);
    candidate.insert("winrate", 0.55);
    candidate.insert("scoreMean", 1.8);
    candidate.insert("pv", pv);
    candidate.insert("pvVisits", pvVisits);

    QJsonArray candidates;
    candidates.push_back(candidate);
    QJsonObject analysis;
    analysis.insert("rootVisits", 80);
    analysis.insert("candidates", candidates);

    QJsonObject node;
    node.insert("nodeId", QString::number(second));
    node.insert("positionKey", QString::fromStdString(analysisPositionKey(model, second)));
    node.insert("analysis", analysis);
    QJsonArray nodes;
    nodes.push_back(node);

    QJsonObject sidecar;
    sidecar.insert("format", "lizzieyzy-analysis-sidecar-v1");
    sidecar.insert("nodes", nodes);

    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&model, sidecar, &warnings);
    require(applied == 1, "sidecar with mismatched PV visits should still apply usable analysis");
    require(warnings.empty(), "sidecar mismatched PV visits should not warn");

    const MoveCandidate& restored = model.node(second).analysis->candidates.front();
    require(restored.pv.size() == 2, "sidecar mismatched PV visits should keep PV");
    require(restored.pvVisits.empty(), "sidecar mismatched PV visits should be discarded");
}

void testAnalysisSidecarRejectsNonFiniteNumericFields()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);
    const double infinity = std::numeric_limits<double>::infinity();

    QJsonArray rootOwnership;
    QJsonArray candidateOwnership;
    for (int index = 0; index < model.boardSize().pointCount(); ++index) {
        rootOwnership.push_back(index == 10 ? infinity : (index % 2 == 0 ? 0.1 : -0.1));
        candidateOwnership.push_back(index == 20 ? infinity : (index % 2 == 0 ? 0.2 : -0.2));
    }

    QJsonObject candidate;
    candidate.insert("move", "dd");
    candidate.insert("visits", 44);
    candidate.insert("winrate", infinity);
    candidate.insert("scoreMean", infinity);
    candidate.insert("scoreLead", 1.25);
    candidate.insert("scoreStdev", infinity);
    candidate.insert("policy", infinity);
    candidate.insert("prior", 0.125);
    candidate.insert("ownership", candidateOwnership);
    QJsonArray candidates;
    candidates.push_back(candidate);

    QJsonObject analysis;
    analysis.insert("rootVisits", 44);
    analysis.insert("rootWinrate", infinity);
    analysis.insert("rootScoreMean", infinity);
    analysis.insert("rootScoreLead", 2.75);
    analysis.insert("ownership", rootOwnership);
    analysis.insert("candidates", candidates);

    QJsonObject node;
    node.insert("nodeId", QString::number(second));
    node.insert("positionKey", QString::fromStdString(analysisPositionKey(model, second)));
    node.insert("analysis", analysis);
    QJsonArray nodes;
    nodes.push_back(node);

    QJsonObject sidecar;
    sidecar.insert("format", "lizzieyzy-analysis-sidecar-v1");
    sidecar.insert("nodes", nodes);

    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&model, sidecar, &warnings);
    require(applied == 1, "sidecar with finite visits should apply despite non-finite numeric fields");
    require(warnings.empty(), "sidecar with ignored non-finite numeric fields should not warn");
    require(model.node(second).analysis.has_value(), "sidecar with finite visits should restore analysis");

    const AnalysisSnapshot& restored = *model.node(second).analysis;
    require(restored.rootVisits == 44, "sidecar finite root visits should remain usable");
    require(restored.rootWinrate == 0.0, "sidecar non-finite root winrate should be ignored");
    require(restored.rootScoreMean == 2.75, "sidecar root scoreLead fallback should apply after non-finite scoreMean");
    require(restored.ownership.empty(), "sidecar non-finite root ownership should be discarded as incomplete");
    require(restored.candidates.size() == 1, "sidecar candidate with finite visits should remain");
    require(restored.candidates.front().winrate == 0.0, "sidecar non-finite candidate winrate should be ignored");
    require(restored.candidates.front().scoreMean == 1.25, "sidecar candidate scoreLead fallback should apply");
    require(restored.candidates.front().scoreStdev == 0.0, "sidecar non-finite scoreStdev should be ignored");
    require(restored.candidates.front().policy > 0.124 && restored.candidates.front().policy < 0.126, "sidecar prior fallback should apply after non-finite policy");
    require(restored.candidates.front().ownership.empty(), "sidecar non-finite candidate ownership should be discarded");
}

void testAnalysisSidecarInvalidRootFieldsFallBackToBestCandidate()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    QJsonObject candidate;
    candidate.insert("move", "dd");
    candidate.insert("visits", 91);
    candidate.insert("winrate", 0.57);
    candidate.insert("scoreMean", 1.75);
    QJsonArray candidates;
    candidates.push_back(candidate);

    QJsonObject analysis;
    analysis.insert("rootVisits", 91);
    analysis.insert("rootWinrate", QStringLiteral("not-a-number"));
    analysis.insert("rootScoreMean", QStringLiteral("not-a-number"));
    analysis.insert("candidates", candidates);

    QJsonObject node;
    node.insert("nodeId", QString::number(second));
    node.insert("positionKey", QString::fromStdString(analysisPositionKey(model, second)));
    node.insert("analysis", analysis);
    QJsonArray nodes;
    nodes.push_back(node);

    QJsonObject sidecar;
    sidecar.insert("format", "lizzieyzy-analysis-sidecar-v1");
    sidecar.insert("nodes", nodes);

    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&model, sidecar, &warnings);
    require(applied == 1, "sidecar with invalid root fields should apply when candidates are usable");
    require(warnings.empty(), "sidecar invalid root fields should fall back without warning");
    require(model.node(second).analysis.has_value(), "sidecar invalid root fields should restore analysis");

    const AnalysisSnapshot& restored = *model.node(second).analysis;
    require(
        restored.rootWinrate > 0.569 && restored.rootWinrate < 0.571,
        "invalid sidecar root winrate should fall back to best candidate");
    require(
        restored.rootScoreMean > 1.74 && restored.rootScoreMean < 1.76,
        "invalid sidecar root score should fall back to best candidate");
}

void testAnalysisSidecarCollectionGameIndex()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    AnalysisResponse response;
    response.id = analysisRequestId(second);
    response.rootVisits = 72;
    response.rootWinrate = 0.63;
    response.rootScoreMean = 4.2;
    response.moveInfos.push_back(MoveCandidate{Move::play(Color::Black, Point{3, 3}), 72, 0.63, 4.2});
    require(applyAnalysisResponse(&model, response), "analysis response should apply before indexed sidecar export");

    const QJsonObject sidecar = analysisSidecarToJson(model, "collection.sgf", 1);
    require(sidecar.value("collectionGameIndex").toInt(-1) == 1, "collection sidecar should write the selected game index");

    NodeId matchingFirst = 0;
    NodeId matchingSecond = 0;
    GameModel matching = sampleModel(&matchingFirst, &matchingSecond);
    std::vector<std::string> matchingWarnings;
    const int matchingApplied = applyAnalysisSidecarJson(&matching, sidecar, &matchingWarnings, 1);
    require(matchingApplied == 1, "indexed sidecar should apply when the selected collection game matches");
    require(matchingWarnings.empty(), "matching indexed sidecar should not warn");
    require(matching.node(matchingSecond).analysis.has_value(), "matching indexed sidecar should restore analysis");

    NodeId mismatchFirst = 0;
    NodeId mismatchSecond = 0;
    GameModel mismatch = sampleModel(&mismatchFirst, &mismatchSecond);
    std::vector<std::string> mismatchWarnings;
    const int mismatchApplied = applyAnalysisSidecarJson(&mismatch, sidecar, &mismatchWarnings, 0);
    require(mismatchApplied == 0, "indexed sidecar should not apply to a different selected collection game");
    require(!mismatchWarnings.empty(), "mismatched indexed sidecar should warn");
    require(
        mismatchWarnings.front().find("does not match selected SGF game index") != std::string::npos,
        "mismatched indexed sidecar warning should explain the selected game mismatch");
    require(!mismatch.node(mismatchSecond).analysis.has_value(), "mismatched indexed sidecar should not restore analysis");

    QJsonObject unindexed = sidecar;
    unindexed.remove("collectionGameIndex");
    NodeId missingFirst = 0;
    NodeId missingSecond = 0;
    GameModel missing = sampleModel(&missingFirst, &missingSecond);
    std::vector<std::string> missingWarnings;
    const int missingApplied = applyAnalysisSidecarJson(&missing, unindexed, &missingWarnings, 1);
    require(missingApplied == 0, "unindexed sidecar should not apply when a collection game index is required");
    require(!missingWarnings.empty(), "unindexed collection sidecar should warn");
    require(
        missingWarnings.front().find("missing collectionGameIndex") != std::string::npos,
        "unindexed collection sidecar warning should name collectionGameIndex");
    require(!missing.node(missingSecond).analysis.has_value(), "unindexed collection sidecar should not restore analysis");

    NodeId singleFirst = 0;
    NodeId singleSecond = 0;
    GameModel single = sampleModel(&singleFirst, &singleSecond);
    std::vector<std::string> singleWarnings;
    const int singleApplied = applyAnalysisSidecarJson(&single, sidecar, &singleWarnings);
    require(singleApplied == 1, "indexed sidecar should remain importable when no collection index is required");
    require(singleWarnings.empty(), "indexed sidecar on a single-game load should not warn");
}

void testAnalysisSidecarErrorRoundTrip()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);
    model.node(second).analysisError = "analysis request failed: fake node\nraw: {\"id\":\"node:2\"}";

    const QJsonObject sidecar = analysisSidecarToJson(model, "failed.sgf");
    const QJsonArray nodes = sidecar.value("nodes").toArray();
    require(nodes.size() == 1, "sidecar should include nodes with analysis errors");
    const QJsonObject nodeObject = nodes.at(0).toObject();
    require(nodeObject.value("analysis").isUndefined(), "error-only sidecar node should not invent analysis");
    require(
        !nodeObject.value("positionKey").toString().isEmpty(),
        "error-only sidecar node should include a position key");
    require(
        nodeObject.value("analysisError").toString().contains("fake node"),
        "sidecar should write analysis error diagnostics");

    NodeId freshFirst = 0;
    NodeId freshSecond = 0;
    GameModel fresh = sampleModel(&freshFirst, &freshSecond);
    AnalysisSnapshot stale;
    stale.rootVisits = 7;
    fresh.node(freshSecond).analysis = stale;
    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&fresh, sidecar, &warnings);
    require(applied == 1, "sidecar should apply one analysis error node");
    require(warnings.empty(), "error-only sidecar should not warn");

    const GameNode& restored = fresh.node(freshSecond);
    require(!restored.analysis.has_value(), "error-only sidecar should clear stale analysis");
    require(
        restored.analysisError.find("fake node") != std::string::npos,
        "sidecar should restore analysis error diagnostics");
    require(restored.analysisError.find("raw:") != std::string::npos, "sidecar should preserve raw error payload");

    QJsonObject successSidecar = sidecar;
    QJsonArray successNodes = successSidecar.value("nodes").toArray();
    QJsonObject successNode = successNodes.at(0).toObject();
    QJsonObject analysis;
    analysis.insert("rootVisits", 11);
    analysis.insert("rootWinrate", 0.55);
    analysis.insert("rootScoreMean", 1.2);
    successNode.insert("analysis", analysis);
    successNode.insert("analysisError", "old failure");
    successNodes.replace(0, successNode);
    successSidecar.insert("nodes", successNodes);

    NodeId successFirst = 0;
    NodeId successSecond = 0;
    GameModel successTarget = sampleModel(&successFirst, &successSecond);
    std::vector<std::string> successWarnings;
    const int successApplied = applyAnalysisSidecarJson(&successTarget, successSidecar, &successWarnings);
    require(successApplied == 1, "sidecar analysis should apply when analysisError is also present");
    require(successWarnings.empty(), "sidecar analysis with stale error should not warn");
    require(successTarget.node(successSecond).analysis.has_value(), "sidecar analysis should win over stale error");
    require(successTarget.node(successSecond).analysisError.empty(), "sidecar analysis should clear stale sidecar error");
}

void testAnalysisSidecarExportKeepsErrorsExclusive()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel model = sampleModel(&first, &second);

    AnalysisSnapshot staleSuccess;
    staleSuccess.rootVisits = 11;
    staleSuccess.rootWinrate = 0.55;
    staleSuccess.rootScoreMean = 1.2;
    model.node(second).analysis = staleSuccess;
    model.node(second).analysisError = "analysis request failed: fake node";

    const QJsonObject sidecar = analysisSidecarToJson(model, "failed.sgf");
    const QJsonArray nodes = sidecar.value("nodes").toArray();
    require(nodes.size() == 1, "sidecar should include a node with analysis error");
    const QJsonObject nodeObject = nodes.at(0).toObject();
    require(nodeObject.value("analysis").isUndefined(), "sidecar error export should not include stale analysis");
    require(
        nodeObject.value("analysisError").toString().contains("fake node"),
        "sidecar error export should preserve diagnostics");
}

void testAnalysisSidecarUnsupportedFormatIsRejected()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel target = sampleModel(&first, &second);
    target.node(second).analysisError = "keep existing failure";

    QJsonObject analysis;
    analysis.insert("rootVisits", 99);
    analysis.insert("rootWinrate", 0.66);
    analysis.insert("rootScoreMean", 2.4);
    QJsonObject node;
    node.insert("nodeId", QString::number(second));
    node.insert("positionKey", QString::fromStdString(analysisPositionKey(target, second)));
    node.insert("analysis", analysis);
    QJsonArray nodes;
    nodes.push_back(node);

    QJsonObject sidecar;
    sidecar.insert("format", "future-format-v9");
    sidecar.insert("nodes", nodes);

    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&target, sidecar, &warnings);
    require(applied == 0, "unsupported sidecar format should not apply any nodes");
    require(!warnings.empty(), "unsupported sidecar format should warn");
    require(
        warnings.front().find("unsupported analysis sidecar format") != std::string::npos,
        "unsupported sidecar warning should name the format problem");
    require(!target.node(second).analysis.has_value(), "unsupported sidecar format should not import analysis");
    require(
        target.node(second).analysisError == "keep existing failure",
        "unsupported sidecar format should leave existing node diagnostics untouched");
}

void testAnalysisSidecarInvalidNodesArrayWarns()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel target = sampleModel(&first, &second);
    AnalysisSnapshot stale;
    stale.rootVisits = 7;
    target.node(second).analysis = stale;

    QJsonObject missingNodes;
    missingNodes.insert("format", "lizzieyzy-analysis-sidecar-v1");
    std::vector<std::string> missingWarnings;
    const int missingApplied = applyAnalysisSidecarJson(&target, missingNodes, &missingWarnings);
    require(missingApplied == 0, "sidecar without nodes array should not apply");
    require(!missingWarnings.empty(), "sidecar without nodes array should warn");
    require(
        missingWarnings.front() == "analysis sidecar is missing a valid nodes array",
        "sidecar without nodes array warning should name the missing nodes array");
    require(
        target.node(second).analysis.has_value() && target.node(second).analysis->rootVisits == 7,
        "sidecar without nodes array should leave existing analysis untouched");

    QJsonObject invalidNodes;
    invalidNodes.insert("format", "lizzieyzy-analysis-sidecar-v1");
    invalidNodes.insert("nodes", "not-an-array");
    std::vector<std::string> invalidWarnings;
    const int invalidApplied = applyAnalysisSidecarJson(&target, invalidNodes, &invalidWarnings);
    require(invalidApplied == 0, "sidecar with non-array nodes should not apply");
    require(!invalidWarnings.empty(), "sidecar with non-array nodes should warn");
    require(
        invalidWarnings.front() == "analysis sidecar is missing a valid nodes array",
        "sidecar with non-array nodes warning should name the invalid nodes array");
}

void testAnalysisSidecarMalformedNodeEntryWarnsAndContinues()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel target = sampleModel(&first, &second);

    QJsonObject analysis;
    analysis.insert("rootVisits", 42);
    analysis.insert("rootWinrate", 0.56);
    analysis.insert("rootScoreMean", 1.7);

    QJsonObject validNode;
    validNode.insert("nodeId", QString::number(second));
    validNode.insert("positionKey", QString::fromStdString(analysisPositionKey(target, second)));
    validNode.insert("analysis", analysis);

    QJsonArray nodes;
    nodes.push_back("not-a-node-object");
    nodes.push_back(validNode);

    QJsonObject sidecar;
    sidecar.insert("format", "lizzieyzy-analysis-sidecar-v1");
    sidecar.insert("nodes", nodes);

    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&target, sidecar, &warnings);
    require(applied == 1, "sidecar should continue after malformed node entries");
    require(!warnings.empty(), "malformed sidecar node entries should warn");
    require(
        warnings.front() == "analysis sidecar node entry 0 is not an object",
        "malformed sidecar node warning should name the array entry");
    require(target.node(second).analysis.has_value(), "valid sidecar node after malformed entry should restore analysis");
    require(target.node(second).analysis->rootVisits == 42, "valid sidecar node after malformed entry should apply");
}

void testAnalysisSidecarRejectsFractionalNumericNodeId()
{
    NodeId first = 0;
    NodeId second = 0;
    GameModel target = sampleModel(&first, &second);

    QJsonObject analysis;
    analysis.insert("rootVisits", 42);
    analysis.insert("rootWinrate", 0.56);
    analysis.insert("rootScoreMean", 1.7);

    QJsonObject node;
    node.insert("nodeId", static_cast<double>(second) + 0.5);
    node.insert("positionKey", QString::fromStdString(analysisPositionKey(target, second)));
    node.insert("analysis", analysis);

    QJsonArray nodes;
    nodes.push_back(node);

    QJsonObject sidecar;
    sidecar.insert("format", "lizzieyzy-analysis-sidecar-v1");
    sidecar.insert("nodes", nodes);

    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&target, sidecar, &warnings);
    require(applied == 0, "sidecar should reject fractional numeric nodeId values");
    require(!warnings.empty(), "fractional numeric sidecar nodeId should warn");
    require(
        warnings.front() == "analysis sidecar node is missing a valid nodeId",
        "fractional numeric sidecar nodeId should use the invalid-nodeId warning");
    require(!target.node(second).analysis.has_value(), "fractional numeric sidecar nodeId should not apply by truncation");
}

void testAnalysisSidecarLegacyKeyRejectsAmbiguousPlayer()
{
    SgfParser parser;
    GameModel model = parser.parseGame("(;GM[1]FF[4]SZ[9]PL[W])");
    AnalysisSnapshot snapshot;
    snapshot.rootVisits = 16;
    snapshot.rootWinrate = 0.5;
    model.root().analysis = snapshot;

    QJsonObject sidecar = analysisSidecarToJson(model, "player.sgf");
    QJsonArray nodes = sidecar.value("nodes").toArray();
    QJsonObject node = nodes.at(0).toObject();
    QString legacyKey = node.value("positionKey").toString();
    legacyKey.remove("|P:W");
    node.insert("positionKey", legacyKey);
    nodes.replace(0, node);
    sidecar.insert("nodes", nodes);

    GameModel target = parser.parseGame("(;GM[1]FF[4]SZ[9]PL[W])");
    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&target, sidecar, &warnings);
    require(applied == 0, "legacy keyed sidecar should not apply when player-to-move is ambiguous");
    require(!warnings.empty(), "ambiguous legacy keyed sidecar should warn");
    require(!target.root().analysis.has_value(), "ambiguous legacy keyed sidecar should not restore analysis");
}

void testAnalysisPositionKeyCanonicalizesRuleAliases()
{
    GameModel aliasRule(BoardSize::square(9));
    aliasRule.gameInfo().rules = " jp ";
    AnalysisSnapshot snapshot;
    snapshot.rootVisits = 24;
    snapshot.rootWinrate = 0.56;
    aliasRule.root().analysis = snapshot;

    const QJsonObject sidecar = analysisSidecarToJson(aliasRule, "rules.sgf");
    const QJsonObject nodeObject = sidecar.value("nodes").toArray().at(0).toObject();
    const QString positionKey = nodeObject.value("positionKey").toString();
    require(
        positionKey.contains("|RU:japanese|"),
        "new sidecar position keys should canonicalize SGF rule aliases");
    require(!positionKey.contains("|RU: jp |"), "new sidecar position keys should not keep raw rule aliases");

    GameModel canonicalRule(BoardSize::square(9));
    canonicalRule.gameInfo().rules = "Japanese";
    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&canonicalRule, sidecar, &warnings);
    require(applied == 1, "canonical rule sidecar should apply to an equivalent rule spelling");
    require(warnings.empty(), "canonical rule sidecar should not warn for an equivalent rule spelling");
    require(canonicalRule.root().analysis.has_value(), "canonical rule sidecar should restore root analysis");

    QJsonObject legacySidecar = sidecar;
    QJsonArray legacyNodes = legacySidecar.value("nodes").toArray();
    QJsonObject legacyNode = legacyNodes.at(0).toObject();
    QString legacyPositionKey = positionKey;
    legacyPositionKey.replace("|RU:japanese|", "|RU: jp |");
    legacyNode.insert("positionKey", legacyPositionKey);
    legacyNodes.replace(0, legacyNode);
    legacySidecar.insert("nodes", legacyNodes);

    GameModel legacyTarget(BoardSize::square(9));
    legacyTarget.gameInfo().rules = "Japanese";
    std::vector<std::string> legacyWarnings;
    const int legacyApplied = applyAnalysisSidecarJson(&legacyTarget, legacySidecar, &legacyWarnings);
    require(legacyApplied == 1, "legacy raw-rule sidecar keys should remain importable");
    require(legacyWarnings.empty(), "legacy raw-rule sidecar keys should not warn");
    require(legacyTarget.root().analysis.has_value(), "legacy raw-rule sidecar should restore root analysis");

    GameModel responseTarget(BoardSize::square(9));
    responseTarget.gameInfo().rules = "Japanese";
    AnalysisResponse response;
    response.id = analysisRequestId(responseTarget.rootId());
    response.rootVisits = 25;
    response.rootWinrate = 0.57;
    require(
        applyAnalysisResponse(&responseTarget, response, legacyPositionKey.toStdString()),
        "analysis response position-key guard should accept legacy raw-rule keys");
    require(
        responseTarget.root().analysis.has_value() && responseTarget.root().analysis->rootVisits == 25,
        "legacy raw-rule response key should update root analysis");
}

void testAnalysisSidecarPositionKeyIgnoresSetupOrder()
{
    GameModel model(BoardSize::square(9));
    model.gameInfo().komi = 7.5;
    model.gameInfo().rules = "Chinese";
    model.root().setupStones.black = {Point{8, 8}, Point{0, 0}, Point{4, 4}, Point{0, 0}};
    const NodeId moveNode = model.addMove(Move::play(Color::White, Point{3, 3}));

    AnalysisResponse response;
    response.id = analysisRequestId(moveNode);
    response.rootVisits = 42;
    response.rootWinrate = 0.58;
    response.rootScoreMean = 1.1;
    response.moveInfos.push_back(MoveCandidate{Move::play(Color::Black, Point{5, 5}), 42, 0.58, 1.1});
    require(applyAnalysisResponse(&model, response), "analysis response should apply before setup-order sidecar export");

    const QJsonObject sidecar = analysisSidecarToJson(model, "setup-order.sgf");

    GameModel reordered(BoardSize::square(9));
    reordered.gameInfo().komi = 7.5;
    reordered.gameInfo().rules = "Chinese";
    reordered.root().setupStones.black = {Point{4, 4}, Point{0, 0}, Point{8, 8}};
    const NodeId reorderedMove = reordered.addMove(Move::play(Color::White, Point{3, 3}));
    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&reordered, sidecar, &warnings);

    require(applied == 1, "sidecar position key should ignore setup stone ordering");
    require(warnings.empty(), "setup-order equivalent sidecar should not warn");
    require(reordered.node(reorderedMove).analysis.has_value(), "setup-order equivalent sidecar should restore analysis");
    require(reordered.node(reorderedMove).analysis->rootVisits == 42, "setup-order sidecar should restore visits");
}

void testAnalysisSidecarPositionKeyIgnoresRedundantRootAe()
{
    GameModel model(BoardSize::square(9));
    model.gameInfo().komi = 7.5;
    model.gameInfo().rules = "Chinese";
    model.root().setupStones.black = {Point{0, 0}};
    model.root().setupStones.white = {Point{1, 1}};
    model.root().setupStones.empty = {Point{0, 0}};
    const NodeId moveNode = model.addMove(Move::play(Color::Black, Point{2, 2}));

    AnalysisResponse response;
    response.id = analysisRequestId(moveNode);
    response.rootVisits = 55;
    response.rootWinrate = 0.61;
    response.rootScoreMean = 2.2;
    response.moveInfos.push_back(MoveCandidate{Move::play(Color::White, Point{3, 3}), 55, 0.61, 2.2});
    require(applyAnalysisResponse(&model, response), "analysis response should apply before root-AE sidecar export");

    const QJsonObject sidecar = analysisSidecarToJson(model, "root-ae.sgf");

    GameModel equivalent(BoardSize::square(9));
    equivalent.gameInfo().komi = 7.5;
    equivalent.gameInfo().rules = "Chinese";
    equivalent.root().setupStones.white = {Point{1, 1}};
    const NodeId equivalentMove = equivalent.addMove(Move::play(Color::Black, Point{2, 2}));
    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&equivalent, sidecar, &warnings);

    require(applied == 1, "sidecar position key should ignore redundant root AE setup");
    require(warnings.empty(), "root-AE equivalent sidecar should not warn");
    require(equivalent.node(equivalentMove).analysis.has_value(), "root-AE equivalent sidecar should restore analysis");
    require(equivalent.node(equivalentMove).analysis->rootVisits == 55, "root-AE sidecar should restore visits");
}

void testAnalysisSidecarInvalidPvMoveDoesNotConsumeTurn()
{
    GameModel model(BoardSize::square(19));

    QJsonObject candidate;
    candidate.insert("move", "dd");
    candidate.insert("visits", 64);
    candidate.insert("winrate", 0.52);
    candidate.insert("scoreMean", 1.4);
    QJsonArray pv;
    pv.push_back("dd");
    pv.push_back("??");
    pv.push_back("pp");
    pv.push_back("pass");
    candidate.insert("pv", pv);

    QJsonArray candidates;
    candidates.push_back(candidate);
    QJsonObject analysis;
    analysis.insert("rootVisits", 64);
    analysis.insert("rootWinrate", 0.52);
    analysis.insert("rootScoreMean", 1.4);
    analysis.insert("candidates", candidates);

    QJsonObject nodeObject;
    nodeObject.insert("nodeId", QString::number(model.rootId()));
    nodeObject.insert("analysis", analysis);
    QJsonArray nodes;
    nodes.push_back(nodeObject);
    QJsonObject sidecar;
    sidecar.insert("format", "lizzieyzy-analysis-sidecar-v1");
    sidecar.insert("nodes", nodes);

    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&model, sidecar, &warnings);
    require(applied == 1, "sidecar with malformed PV token should still apply valid analysis");
    require(warnings.empty(), "sidecar malformed PV token should not warn when useful analysis remains");

    const std::vector<Move>& restoredPv = model.root().analysis->candidates.front().pv;
    require(restoredPv.empty(), "sidecar malformed PV should be discarded");
}

void testAnalysisSidecarRoundTripsPassAndResignCandidates()
{
    GameModel model(BoardSize::square(9));
    const NodeId child = model.addMove(Move::play(Color::Black, Point{3, 3}));

    AnalysisSnapshot analysis;
    analysis.rootVisits = 48;
    analysis.rootWinrate = 0.44;
    analysis.rootScoreMean = -2.1;
    MoveCandidate passCandidate{Move::pass(Color::White), 28, 0.44, -2.1};
    passCandidate.pv = {Move::pass(Color::White), Move::play(Color::Black, Point{4, 4})};
    passCandidate.pvVisits = {28, 11};
    MoveCandidate resignCandidate{Move::resign(Color::White), 20, 0.12, -18.5};
    resignCandidate.policy = 0.03;
    analysis.candidates = {passCandidate, resignCandidate};
    model.node(child).analysis = analysis;

    const QJsonObject sidecar = analysisSidecarToJson(model, "pass-resign.sgf");

    GameModel restored(BoardSize::square(9));
    const NodeId restoredChild = restored.addMove(Move::play(Color::Black, Point{3, 3}));
    std::vector<std::string> warnings;
    const int applied = applyAnalysisSidecarJson(&restored, sidecar, &warnings);
    require(applied == 1, "sidecar should apply pass and resign candidate analysis");
    require(warnings.empty(), "sidecar pass/resign candidate import should not warn");
    require(restored.node(restoredChild).analysis.has_value(), "sidecar should restore pass/resign analysis");

    const std::vector<MoveCandidate>& candidates = restored.node(restoredChild).analysis->candidates;
    require(candidates.size() == 2, "sidecar should restore both pass and resign candidates");
    require(candidates[0].move.type == MoveType::Pass, "sidecar should restore pass candidate type");
    require(candidates[0].move.color == Color::White, "sidecar should restore pass candidate color");
    require(candidates[0].pv.size() == 2, "sidecar should restore pass candidate PV");
    require(candidates[0].pv[0].type == MoveType::Pass, "sidecar should restore pass candidate PV pass");
    require(candidates[0].pv[1].color == Color::Black, "sidecar pass candidate PV should alternate colors");
    require(candidates[0].pvVisits.size() == 2 && candidates[0].pvVisits[1] == 11, "sidecar should restore pass PV visits");
    require(candidates[1].move.type == MoveType::Resign, "sidecar should restore resign candidate type");
    require(candidates[1].move.color == Color::White, "sidecar should restore resign candidate color");
    require(candidates[1].policy > 0.029 && candidates[1].policy < 0.031, "sidecar should restore resign policy");
}

void testLegacyLizzieAnalysisImport()
{
    const std::string sgf = R"SGF((;GM[1]FF[4]SZ[9]KM[7.5]LZOP[KataGo 48.0 12 0.4 1.5
move D4 visits 12 winrate 5200 prior 1234 scoreMean 0.40 pv D4 E4 pvVisits 12 5 ownership 0.1 -0.1 0.2];B[dd]LZYPERSP[W]LZ[KataGo 46.0 1.3k 2.5 4.1
move C3 visits 1300 winrate 5404 prior 1200 scoreMean 2.50 pv C3 pass E4 pvVisits 1300 600 100 ownership -0.1 0.0 0.1]))SGF";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 2, "legacy import should apply root and move node");
    require(warnings.empty(), "legacy import should not warn for valid data");

    const GameNode& root = model.root();
    require(root.analysis.has_value(), "legacy LZOP should import root analysis");
    require(root.analysis->rootVisits == 12, "legacy root visits should import");
    require(root.analysis->rootWinrate > 0.519 && root.analysis->rootWinrate < 0.521, "legacy root winrate should invert stored value");
    require(root.analysis->winratePerspective == Color::Black, "legacy root should default to black winrate perspective");
    require(root.analysis->candidates.size() == 1, "legacy root candidate should import");
    require(root.analysis->candidates.front().move.point == Point{3, 5}, "legacy GTP root coordinate should import");
    require(root.analysis->candidates.front().pvVisits.size() == 2, "legacy root PV visits should import");
    require(root.analysis->ownership.empty(), "legacy incomplete root ownership tail should be discarded");

    const NodeId childId = root.children.front();
    const GameNode& child = model.node(childId);
    require(child.analysis.has_value(), "legacy LZ should import move analysis");
    require(child.analysis->rootVisits == 1300, "legacy compact visits should import");
    require(child.analysis->rootScoreMean > 2.49 && child.analysis->rootScoreMean < 2.51, "legacy scoreMean should import");
    require(child.analysis->winratePerspective == Color::White, "legacy import should restore stored winrate perspective");
    require(child.analysis->candidates.size() == 1, "legacy candidate list should import");

    const MoveCandidate& candidate = child.analysis->candidates.front();
    require(candidate.move.color == Color::White, "legacy candidate should use node side-to-move");
    require(candidate.move.point == Point{2, 6}, "legacy GTP candidate coordinate should import");
    require(candidate.winrate > 0.5403 && candidate.winrate < 0.5405, "legacy candidate winrate should scale");
    require(candidate.policy > 0.119 && candidate.policy < 0.121, "legacy candidate prior should scale");
    require(candidate.scoreStdev > 4.09 && candidate.scoreStdev < 4.11, "legacy header stdev should apply to first candidate");
    require(candidate.pv.size() == 3, "legacy PV should import");
    require(candidate.pv[0].color == Color::White && candidate.pv[1].color == Color::Black, "legacy PV colors should alternate");
    require(candidate.pv[1].type == MoveType::Pass, "legacy PV pass should import");
    require(candidate.pvVisits.size() == 3 && candidate.pvVisits[2] == 100, "legacy PV visits should import");
    require(child.analysis->ownership.empty(), "legacy incomplete ownership should be discarded");
}

void testLegacyLizzieAnalysisImportAcceptsMixedCaseKeys()
{
    const std::string sgf = R"SGF((;GM[1]FF[4]SZ[9]KM[7.5]LZOP[KataGo 48.0 12 0.4 1.5
MOVE D4 VISITS 12 WINRATE 5200 PRIOR 1234 ScoreLead 0.40 PV D4 PaSs PVVisits 12 5 MovesOwnership 0.01 -0.01 OWNERSHIP 0.1 -0.1 0.2]))SGF";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 1, "mixed-case legacy keys should import root analysis");
    require(warnings.empty(), "mixed-case legacy keys should not warn for valid data");

    const GameNode& root = model.root();
    require(root.analysis.has_value(), "mixed-case legacy LZOP should restore analysis");
    require(root.analysis->rootVisits == 12, "mixed-case legacy visits should import");
    require(root.analysis->candidates.size() == 1, "mixed-case legacy candidate should import");
    require(root.analysis->ownership.empty(), "mixed-case incomplete legacy ownership tail should be discarded");

    const MoveCandidate& candidate = root.analysis->candidates.front();
    require(candidate.move.point == Point{3, 5}, "mixed-case legacy move key should import candidate coordinate");
    require(candidate.scoreMean > 0.39 && candidate.scoreMean < 0.41, "mixed-case scoreLead should import");
    require(candidate.pv.size() == 2, "mixed-case PV boundary should stop before PV visits");
    require(candidate.pv[1].type == MoveType::Pass, "mixed-case legacy pass PV should import");
    require(candidate.pvVisits.size() == 2 && candidate.pvVisits[1] == 5, "mixed-case PV visits should import");
    require(candidate.ownership.empty(), "mixed-case incomplete moves ownership should be discarded");
}

void testLegacyLizzieAnalysisImportCandidateOwnershipAliases()
{
    const std::string sgf = R"SGF((;GM[1]FF[4]SZ[3]LZOP[KataGo 50.0 21 0.0 1.0
move B2 visits 21 winrate 5000 prior 1000 scoreMean 0.00 pv B2 ownership 0.1 -0.1 0.05 -0.05 0.2 -0.2 0.3 -0.3 0.0 movesOwnership 0.6 -0.6 0.3 -0.3 0.2 -0.2 0.1 -0.1 0.0 info move C3 visits 20 winrate 4900 prior 500 scoreMean -0.20 pv C3 movesOwnership -0.4 0.4 -0.3 0.3 -0.2 0.2 -0.1 0.1 0.0 ownership 0.7 -0.7 0.5 -0.5 0.4 -0.4 0.3 -0.3 0.0 scoreSelfplay 0.0]))SGF";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 1, "legacy candidate ownership aliases should import");
    require(warnings.empty(), "legacy candidate ownership aliases should not warn");
    require(model.root().analysis.has_value(), "legacy candidate ownership aliases should restore analysis");

    const AnalysisSnapshot& restored = *model.root().analysis;
    require(restored.ownership.empty(), "candidate ownership aliases should not become root ownership");
    require(restored.candidates.size() == 2, "candidate ownership aliases should not truncate later candidates");
    const MoveCandidate& first = restored.candidates.at(0);
    const MoveCandidate& second = restored.candidates.at(1);
    require(first.ownership.size() == 9, "legacy ownership alias should restore complete candidate ownership");
    require(
        first.ownership[0] == 0.6 && first.ownership[1] == -0.6,
        "legacy movesOwnership should replace an earlier ownership alias");
    require(second.ownership.size() == 9, "legacy movesOwnership should restore second candidate ownership");
    require(
        second.ownership[0] == -0.4 && second.ownership[1] == 0.4,
        "legacy ownership alias should not overwrite an earlier movesOwnership field");
}

void testLegacyLizzieAnalysisImportRejectsMalformedCandidateOwnership()
{
    const std::string sgf = R"SGF((;GM[1]FF[4]SZ[3]LZOP[KataGo 50.0 21 0.0 1.0
move B2 visits 21 winrate 5000 prior 1000 scoreMean 0.00 pv B2 movesOwnership 0.6 -0.6 0.3 -0.3 0.2 -0.2 0.1 -0.1 0.0 bad-token info move C3 visits 20 winrate 4900 prior 500 scoreMean -0.20 pv C3 ownership 0.1 -0.1 0.05 -0.05 0.2 -0.2 0.3 -0.3 0.0 bad-token movesOwnership -0.4 0.4 -0.3 0.3 -0.2 0.2 -0.1 0.1 0.0]))SGF";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 1, "malformed legacy candidate ownership should still import usable analysis");
    require(warnings.empty(), "malformed legacy candidate ownership should not warn");
    require(model.root().analysis.has_value(), "malformed legacy candidate ownership should restore analysis");

    const AnalysisSnapshot& restored = *model.root().analysis;
    require(restored.candidates.size() == 2, "malformed legacy ownership should not drop candidates");

    const MoveCandidate& malformedMovesOwnership = restored.candidates.at(0);
    require(
        malformedMovesOwnership.move.point == Point{1, 1},
        "malformed legacy movesOwnership candidate should remain visits-sorted first");
    require(
        malformedMovesOwnership.ownership.empty(),
        "malformed legacy movesOwnership should be discarded even after a full board");

    const MoveCandidate& recoveredMovesOwnership = restored.candidates.at(1);
    require(
        recoveredMovesOwnership.move.point == Point{2, 0},
        "legacy candidate parser should resume at movesOwnership after a malformed ownership alias");
    require(
        recoveredMovesOwnership.ownership.size() == 9,
        "valid legacy movesOwnership after malformed ownership alias should import");
    require(
        recoveredMovesOwnership.ownership[0] == -0.4 && recoveredMovesOwnership.ownership[1] == 0.4,
        "valid legacy movesOwnership values should be preserved after malformed alias");
}

void testLegacyGeneratedAnalysisImportRejectsMalformedPvVisits()
{
    const std::string legacySgf = R"SGF((;GM[1]FF[4]SZ[3]LZOP[KataGo 50.0 21 0.0 1.0
move B2 visits 21 winrate 5000 prior 1000 scoreMean 0.00 pv B2 pass pvVisits 21 10 bad-token movesOwnership 0.6 -0.6 0.3 -0.3 0.2 -0.2 0.1 -0.1 0.0]))SGF";

    SgfParser parser;
    GameModel legacyModel = parser.parseGame(legacySgf);
    std::vector<std::string> warnings;
    const int legacyImported = importLegacyLizzieAnalysis(&legacyModel, &warnings);
    require(legacyImported == 1, "malformed legacy PV visits should still import usable analysis");
    require(warnings.empty(), "malformed legacy PV visits should not warn");
    require(legacyModel.root().analysis.has_value(), "malformed legacy PV visits should restore analysis");
    require(legacyModel.root().analysis->candidates.size() == 1, "malformed legacy PV visits should keep candidate");
    const MoveCandidate& legacyCandidate = legacyModel.root().analysis->candidates.front();
    require(legacyCandidate.pv.size() == 2, "malformed legacy PV visits should not affect PV import");
    require(legacyCandidate.pvVisits.empty(), "malformed legacy PV visits should be discarded");
    require(legacyCandidate.ownership.size() == 9, "legacy movesOwnership after malformed PV visits should import");

    const std::string generatedSgf = R"SGF((;GM[1]FF[4]SZ[3]C[Root note

LizzieYzy analysis
Visits: 21
Candidates:
B2 visits 21 winrate 50.0% score 0.00 pv B2 pass pvVisits 21 10 bad-token movesOwnership -0.6 0.6 -0.3 0.3 -0.2 0.2 -0.1 0.1 0.0]))SGF";

    GameModel generatedModel = parser.parseGame(generatedSgf);
    warnings.clear();
    const int generatedImported = importLegacyLizzieAnalysis(&generatedModel, &warnings);
    require(generatedImported == 1, "malformed generated PV visits should still import usable analysis");
    require(warnings.empty(), "malformed generated PV visits should not warn");
    require(generatedModel.root().analysis.has_value(), "malformed generated PV visits should restore analysis");
    require(
        generatedModel.root().analysis->candidates.size() == 1,
        "malformed generated PV visits should keep candidate");
    const MoveCandidate& generatedCandidate = generatedModel.root().analysis->candidates.front();
    require(generatedCandidate.pv.size() == 2, "malformed generated PV visits should not affect PV import");
    require(generatedCandidate.pvVisits.empty(), "malformed generated PV visits should be discarded");
    require(
        generatedCandidate.ownership.size() == 9,
        "generated movesOwnership after malformed PV visits should import");
}

void testLegacyGeneratedAnalysisImportRejectsMalformedPv()
{
    const std::string legacySgf = R"SGF((;GM[1]FF[4]SZ[3]LZOP[KataGo 50.0 21 0.0 1.0
move B2 visits 21 winrate 5000 prior 1000 scoreMean 0.00 pv B2 ?? pass pvVisits 21 10 movesOwnership 0.6 -0.6 0.3 -0.3 0.2 -0.2 0.1 -0.1 0.0]))SGF";

    SgfParser parser;
    GameModel legacyModel = parser.parseGame(legacySgf);
    std::vector<std::string> warnings;
    const int legacyImported = importLegacyLizzieAnalysis(&legacyModel, &warnings);
    require(legacyImported == 1, "malformed legacy PV should still import usable analysis");
    require(warnings.empty(), "malformed legacy PV should not warn");
    require(legacyModel.root().analysis.has_value(), "malformed legacy PV should restore analysis");
    require(legacyModel.root().analysis->candidates.size() == 1, "malformed legacy PV should keep candidate");
    const MoveCandidate& legacyCandidate = legacyModel.root().analysis->candidates.front();
    require(legacyCandidate.pv.empty(), "malformed legacy PV should be discarded instead of skipping tokens");
    require(legacyCandidate.pvVisits.empty(), "legacy PV visits after malformed PV should be discarded");
    require(legacyCandidate.ownership.size() == 9, "legacy movesOwnership after malformed PV should import");

    const std::string generatedSgf = R"SGF((;GM[1]FF[4]SZ[3]C[Root note

LizzieYzy analysis
Visits: 21
Candidates:
B2 visits 21 winrate 50.0% score 0.00 pv B2 ?? pass pvVisits 21 10 movesOwnership -0.6 0.6 -0.3 0.3 -0.2 0.2 -0.1 0.1 0.0]))SGF";

    GameModel generatedModel = parser.parseGame(generatedSgf);
    warnings.clear();
    const int generatedImported = importLegacyLizzieAnalysis(&generatedModel, &warnings);
    require(generatedImported == 1, "malformed generated PV should still import usable analysis");
    require(warnings.empty(), "malformed generated PV should not warn");
    require(generatedModel.root().analysis.has_value(), "malformed generated PV should restore analysis");
    require(generatedModel.root().analysis->candidates.size() == 1, "malformed generated PV should keep candidate");
    const MoveCandidate& generatedCandidate = generatedModel.root().analysis->candidates.front();
    require(generatedCandidate.pv.empty(), "malformed generated PV should be discarded instead of skipping tokens");
    require(generatedCandidate.pvVisits.empty(), "generated PV visits after malformed PV should be discarded");
    require(generatedCandidate.ownership.size() == 9, "generated movesOwnership after malformed PV should import");
}

void testLegacyLizzieAnalysisImportSortsCandidatesAndFallsBackToBest()
{
    const std::string sgf = R"SGF((;GM[1]FF[4]SZ[9]KM[7.5]LZOP[KataGo unknown 0
move E5 visits 50 winrate 4800 prior 1200 scoreMean -0.60 scoreStdev 2.0 pv E5
info move D4 visits 120 winrate 5500 prior 1234 scoreMean 2.10 scoreStdev 3.0 pv D4
info move C3 visits 120 winrate 5300 prior 1000 scoreMean 1.80 scoreStdev 4.0 pv C3
info move pass visits 40 winrate 4700 prior 500 scoreMean -1.00 pv pass]))SGF";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 1, "unsorted legacy candidates should import");
    require(warnings.empty(), "unsorted legacy candidates should not warn");
    require(model.root().analysis.has_value(), "unsorted legacy candidates should restore analysis");

    const AnalysisSnapshot& restored = *model.root().analysis;
    require(restored.candidates.size() == 4, "legacy import should retain all valid candidates");
    require(
        restored.candidates.at(0).move.type == MoveType::Play && restored.candidates.at(0).move.point == Point{2, 6},
        "legacy candidate ties should sort by move key after visits");
    require(
        restored.candidates.at(1).move.type == MoveType::Play && restored.candidates.at(1).move.point == Point{3, 5},
        "legacy second tied candidate should follow move-key order");
    require(
        restored.candidates.at(2).move.type == MoveType::Play && restored.candidates.at(2).move.point == Point{4, 4},
        "legacy lower-visit play candidate should follow tied leaders");
    require(restored.candidates.at(3).move.type == MoveType::Pass, "legacy lowest-visit pass candidate should sort last");
    require(restored.rootVisits == 120, "legacy import without root visits should fall back to best candidate visits");
    require(restored.rootWinrate > 0.529 && restored.rootWinrate < 0.531, "legacy import should fall back to best candidate winrate");
    require(restored.rootScoreMean == 1.8, "legacy import should fall back to best candidate score");
}

void testLegacyLizzieAnalysisImportExtensionFieldBoundaries()
{
    const std::string sgf =
        R"SGF((;GM[1]FF[4]SZ[9]KM[7.5]LZOP[KataGo 48.0 12 0.4 1.5
move D4 visits 12 winrate 5200 prior 1234 scoreLead 0.40 pv D4 E4 pvEdgeVisits 12 5 edgeVisits 8 utility -0.1 utilityLcb -0.2 scoreSelfplay 0.4 noResultValue 0.01 isSymmetryOf C3 movesOwnershipStdev 0.01 ownershipStdev 0.02 pvVisits 12 5 movesOwnership 0.1 -0.1 ownership 0.3 -0.3]))SGF";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 1, "legacy extension-field analysis should import");
    require(warnings.empty(), "legacy extension-field analysis should not warn");

    const GameNode& root = model.root();
    require(root.analysis.has_value(), "legacy extension-field LZOP should restore analysis");
    require(root.analysis->candidates.size() == 1, "legacy extension-field LZOP should restore candidates");
    require(root.analysis->ownership.empty(), "legacy extension-field incomplete tail ownership should be discarded");

    const MoveCandidate& candidate = root.analysis->candidates.front();
    require(
        candidate.pv.size() == 2 && candidate.pv[0].point == Point{3, 5} && candidate.pv[1].point == Point{4, 5},
        "legacy extension fields with coordinate-like values should not extend PV");
    require(
        candidate.pvVisits.size() == 2 && candidate.pvVisits[1] == 5,
        "legacy extension fields should still allow later PV visits import");
    require(candidate.ownership.empty(), "legacy extension-field incomplete moves ownership should be discarded");
}

void testLegacyLizzieAnalysisImportAcceptsMixedCasePropertyNames()
{
    const std::string sgf = R"SGF((;GM[1]FF[4]SZ[9]KM[7.5]lzop[KataGo 48.0 12 0.4 1.5
move D4 visits 12 winrate 5200 prior 1234 scoreMean 0.40 pv D4 E4 ownership 0.1 -0.1 0.2]lzyPersp[w]))SGF";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 1, "mixed-case legacy SGF property names should import");
    require(warnings.empty(), "mixed-case legacy property names should not warn for valid data");
    require(model.root().analysis.has_value(), "mixed-case lzop property should restore root analysis");
    require(model.root().analysis->winratePerspective == Color::White, "mixed-case lzyPersp property should import");
    require(model.root().analysis->candidates.size() == 1, "mixed-case lzop property should restore candidates");
    require(
        model.root().analysis->candidates.front().move.point == Point{3, 5},
        "mixed-case lzop property should parse candidate coordinates");

    const std::string written = writeSgf(model);
    require(written.find("LZOP[") != std::string::npos, "writer should export imported legacy data as canonical LZOP");
    require(written.find("LZYPERSP[W]") != std::string::npos, "writer should export canonical perspective property");
    require(written.find("lzop[") == std::string::npos, "writer should remove stale mixed-case lzop");
    require(written.find("lzyPersp[") == std::string::npos, "writer should remove stale mixed-case lzyPersp");
}

void testGeneratedAnalysisCommentImport()
{
    const std::string sgf = R"SGF((;GM[1]FF[4]SZ[9];B[dd]C[User note

LizzieYzy analysis
WinratePerspective: W
Winrate: 58.0%
ScoreMean: -0.70
Visits: 96
Candidates:
C3 visits 96 winrate 58.0% score -0.70 stdev 4.50 policy 12.3% pv C3 pass E4 pvVisits 96 50 12]))SGF";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    const NodeId childId = model.root().children.front();
    model.node(childId).analysisError = "old generated comment import failure";

    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 1, "generated analysis comment should import when LZ data is absent");
    require(warnings.empty(), "generated analysis comment import should not warn");

    const GameNode& child = model.node(childId);
    require(child.analysis.has_value(), "generated analysis comment should restore analysis");
    require(child.analysisError.empty(), "generated analysis comment import should clear stale analysis error");
    require(child.analysis->winratePerspective == Color::White, "generated analysis comment should restore perspective");
    require(child.analysis->rootVisits == 96, "generated analysis comment should restore visits");
    require(
        child.analysis->rootWinrate > 0.579 && child.analysis->rootWinrate < 0.581,
        "generated analysis comment should restore root winrate");
    require(
        child.analysis->rootScoreMean > -0.71 && child.analysis->rootScoreMean < -0.69,
        "generated analysis comment should restore root score");
    require(child.analysis->candidates.size() == 1, "generated analysis comment should restore candidate");
    const MoveCandidate& candidate = child.analysis->candidates.front();
    require(candidate.move.color == Color::White, "generated analysis comment candidate should use side to move");
    require(candidate.move.point == Point{2, 6}, "generated analysis comment should parse GTP coordinate");
    require(
        candidate.scoreStdev > 4.49 && candidate.scoreStdev < 4.51,
        "generated analysis comment should restore candidate score stdev");
    require(
        candidate.policy > 0.122 && candidate.policy < 0.124,
        "generated analysis comment should restore candidate policy");
    require(candidate.pv.size() == 3, "generated analysis comment should restore PV");
    require(candidate.pv[0].color == Color::White, "generated analysis comment PV should start with side to move");
    require(candidate.pv[1].color == Color::Black, "generated analysis comment PV should alternate after first move");
    require(candidate.pv[1].type == MoveType::Pass, "generated analysis comment should parse PV pass");
    require(
        candidate.pvVisits.size() == 3 && candidate.pvVisits[2] == 12,
        "generated analysis comment should restore PV visits");
}

void testGeneratedAnalysisCommentImportCandidateOwnership()
{
    const std::string sgf = R"SGF((;GM[1]FF[4]SZ[3];B[aa]C[User note

LizzieYzy analysis
WinratePerspective: W
Visits: 12
Candidates:
B2 visits 12 winrate 50.0% score 0.00 stdev 1.20 policy 10.0% pv B2 pass ownership 0.1 -0.1 0.05 -0.05 0.2 -0.2 0.3 -0.3 0.0 movesOwnership 0.6 -0.6 0.3 -0.3 0.2 -0.2 0.1 -0.1 0.0]))SGF";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);

    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 1, "generated analysis comment candidate ownership should import");
    require(warnings.empty(), "generated analysis comment ownership import should not warn");

    const NodeId childId = model.root().children.front();
    const GameNode& child = model.node(childId);
    require(child.analysis.has_value(), "generated analysis comment ownership should restore analysis");
    require(
        child.analysis->candidates.size() == 1,
        "generated analysis comment ownership should restore candidate");
    const MoveCandidate& candidate = child.analysis->candidates.front();
    require(
        candidate.ownership.size() == 9,
        "generated analysis comment should restore complete candidate ownership");
    require(
        candidate.ownership[0] == 0.6 && candidate.ownership[1] == -0.6,
        "generated analysis comment movesOwnership should take precedence over ownership alias");
}

void testGeneratedAnalysisCommentImportRootOwnership()
{
    const std::string sgf = R"SGF((;GM[1]FF[4]SZ[3]C[Root note

LizzieYzy analysis
Ownership: 0.25 -0.25 0.15 -0.15 0.05 -0.05 0.4 -0.4 0.0]))SGF";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);

    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 1, "generated analysis comment root ownership should import without candidates");
    require(warnings.empty(), "generated analysis comment root ownership import should not warn");
    require(model.root().analysis.has_value(), "generated analysis comment root ownership should restore analysis");
    require(
        model.root().analysis->ownership.size() == 9,
        "generated analysis comment should restore complete root ownership");
    require(
        model.root().analysis->ownership[0] == 0.25 && model.root().analysis->ownership[1] == -0.25,
        "generated analysis comment root ownership values should import");
}

void testGeneratedAnalysisCommentRejectsMalformedOwnership()
{
    const std::string sgf = R"SGF((;GM[1]FF[4]SZ[3]C[Root note

LizzieYzy analysis
Visits: 12
Ownership: 0.25 -0.25 0.15 -0.15 0.05 -0.05 0.4 -0.4 0.0 bad-token
Candidates:
B2 visits 12 winrate 50.0% score 0.00 pv B2 movesOwnership 0.6 -0.6 0.3 -0.3 0.2 -0.2 0.1 -0.1 0.0 bad-token
C3 visits 11 winrate 49.0% score -0.10 pv C3 ownership 0.1 -0.1 0.05 -0.05 0.2 -0.2 0.3 -0.3 0.0 bad-token movesOwnership -0.6 0.6 -0.3 0.3 -0.2 0.2 -0.1 0.1 0.0]))SGF";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);

    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 1, "malformed generated analysis ownership should still import usable fields");
    require(warnings.empty(), "malformed generated analysis ownership import should not warn");
    require(model.root().analysis.has_value(), "malformed generated analysis ownership should restore analysis");
    require(model.root().analysis->ownership.empty(), "malformed generated root ownership should be discarded");
    require(model.root().analysis->candidates.size() == 2, "malformed ownership should not drop candidates");

    const MoveCandidate& malformedMovesOwnership = model.root().analysis->candidates[0];
    require(
        malformedMovesOwnership.move.point == Point{1, 1},
        "malformed generated movesOwnership candidate should remain visits-sorted first");
    require(
        malformedMovesOwnership.ownership.empty(),
        "malformed generated movesOwnership should be discarded even after a full board");

    const MoveCandidate& recoveredMovesOwnership = model.root().analysis->candidates[1];
    require(
        recoveredMovesOwnership.move.point == Point{2, 0},
        "generated candidate parser should resume at movesOwnership after a malformed ownership alias");
    require(
        recoveredMovesOwnership.ownership.size() == 9,
        "valid generated movesOwnership after malformed ownership alias should import");
    require(
        recoveredMovesOwnership.ownership[0] == -0.6 && recoveredMovesOwnership.ownership[1] == 0.6,
        "valid generated movesOwnership values should be preserved after malformed alias");
}

void testGeneratedAnalysisCommentImportSortsCandidatesAndFallsBackToBest()
{
    const std::string sgf = R"SGF((;GM[1]FF[4]SZ[9]C[User note

LizzieYzy analysis
Candidates:
E5 visits 50 winrate 48.0% score -0.60 stdev 2.0 policy 12.0% pv E5
D4 visits 120 winrate 55.0% score 2.10 stdev 3.0 policy 12.3% pv D4
C3 visits 120 winrate 53.0% score 1.80 stdev 4.0 policy 10.0% pv C3
pass visits 40 winrate 47.0% score -1.00 stdev 1.0 policy 5.0% pv pass]))SGF";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 1, "unsorted generated analysis candidates should import");
    require(warnings.empty(), "unsorted generated analysis candidates should not warn");
    require(model.root().analysis.has_value(), "unsorted generated analysis candidates should restore analysis");

    const AnalysisSnapshot& restored = *model.root().analysis;
    require(restored.candidates.size() == 4, "generated analysis import should retain all valid candidates");
    require(
        restored.candidates.at(0).move.type == MoveType::Play && restored.candidates.at(0).move.point == Point{2, 6},
        "generated candidate ties should sort by move key after visits");
    require(
        restored.candidates.at(1).move.type == MoveType::Play && restored.candidates.at(1).move.point == Point{3, 5},
        "generated second tied candidate should follow move-key order");
    require(
        restored.candidates.at(2).move.type == MoveType::Play && restored.candidates.at(2).move.point == Point{4, 4},
        "generated lower-visit play candidate should follow tied leaders");
    require(restored.candidates.at(3).move.type == MoveType::Pass, "generated lowest-visit pass candidate should sort last");
    require(restored.rootVisits == 120, "generated import without root visits should fall back to best candidate visits");
    require(
        restored.rootWinrate > 0.529 && restored.rootWinrate < 0.531,
        "generated import should fall back to best candidate winrate");
    require(restored.rootScoreMean == 1.8, "generated import should fall back to best candidate score");
}

void testGeneratedAnalysisCommentImportAcceptsMixedCaseHeaderAndKeys()
{
    const std::string sgf = R"SGF((;GM[1]FF[4]SZ[9];B[dd]C[User note

lizzieyzy Analysis
WINRATEPERSPECTIVE: white
WINRATE: 58.0%
ScoreLead: -0.70
VISITS: 96
CANDIDATES:
C3 VISITS 96 WINRATE 58.0% SCORELEAD -0.70 SCORESTDEV 4.50 PRIOR 12.3% PV C3 PaSs E4 PVVISITS 96 50 12]))SGF";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 1, "mixed-case generated analysis comment should import");
    require(warnings.empty(), "mixed-case generated analysis comment should not warn");

    const GameNode& child = model.node(model.root().children.front());
    require(child.analysis.has_value(), "mixed-case generated comment should restore analysis");
    require(child.analysis->winratePerspective == Color::White, "mixed-case perspective field should import");
    require(child.analysis->rootVisits == 96, "mixed-case visits field should import");
    require(
        child.analysis->rootScoreMean > -0.71 && child.analysis->rootScoreMean < -0.69,
        "generated analysis ScoreLead field should restore root score");
    require(child.analysis->candidates.size() == 1, "mixed-case generated comment should restore candidate");
    const MoveCandidate& candidate = child.analysis->candidates.front();
    require(candidate.visits == 96, "mixed-case candidate visits should import");
    require(
        candidate.scoreMean > -0.71 && candidate.scoreMean < -0.69,
        "mixed-case candidate scoreLead should import");
    require(
        candidate.scoreStdev > 4.49 && candidate.scoreStdev < 4.51,
        "mixed-case generated comment scoreStdev should import");
    require(
        candidate.policy > 0.122 && candidate.policy < 0.124,
        "mixed-case generated comment prior should import");
    require(candidate.pv.size() == 3, "mixed-case generated comment should restore PV");
    require(candidate.pv[1].type == MoveType::Pass, "mixed-case generated comment PV pass should import");
    require(
        candidate.pvVisits.size() == 3 && candidate.pvVisits[1] == 50,
        "mixed-case generated comment PV visits should import");
}

void testGeneratedAnalysisCommentImportExtensionFieldBoundaries()
{
    const std::string sgf = R"SGF((;GM[1]FF[4]SZ[9];B[dd]C[User note

LizzieYzy analysis
WinratePerspective: W
Winrate: 58.0%
ScoreLead: -0.70
Visits: 96
Candidates:
C3 visits 96 winrate 58.0% scoreLead -0.70 scoreStdev 4.50 prior 12.3% pv C3 pass pvEdgeVisits 96 50 utilityLcb -0.2 isSymmetryOf D4 rawStWrError 0.01 rawStScoreError 0.2 rawVarTimeLeft 42 pvVisits 96 50]))SGF";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 1, "generated extension-field comment should import");
    require(warnings.empty(), "generated extension-field comment should not warn");

    const GameNode& child = model.node(model.root().children.front());
    require(child.analysis.has_value(), "generated extension-field comment should restore analysis");
    require(child.analysis->candidates.size() == 1, "generated extension-field comment should restore candidate");

    const MoveCandidate& candidate = child.analysis->candidates.front();
    require(
        candidate.pv.size() == 2 && candidate.pv[0].point == Point{2, 6} &&
            candidate.pv[1].type == MoveType::Pass,
        "generated extension fields with coordinate-like values should not extend PV");
    require(
        candidate.pvVisits.size() == 2 && candidate.pvVisits[1] == 50,
        "generated extension fields should still allow later PV visits import");
}

void testGeneratedAnalysisCommentImportRootAndIgnoresErrorComment()
{
    const std::string rootSgf = R"SGF((;GM[1]FF[4]SZ[9]C[Root note

LizzieYzy analysis
WinratePerspective: B
Winrate: 52.5%
ScoreMean: 1.25
Visits: 41
Candidates:
D4 visits 41 winrate 52.5% score 1.25 pv D4 E4]))SGF";

    SgfParser parser;
    GameModel rootModel = parser.parseGame(rootSgf);
    std::vector<std::string> warnings;
    const int rootImported = importLegacyLizzieAnalysis(&rootModel, &warnings);
    require(rootImported == 1, "root generated analysis comment should import when LZOP is absent");
    require(warnings.empty(), "root generated analysis comment should not warn");
    require(rootModel.root().analysis.has_value(), "root generated analysis comment should restore analysis");
    require(rootModel.root().analysis->rootVisits == 41, "root generated comment should restore visits");
    require(
        rootModel.root().analysis->rootWinrate > 0.524 && rootModel.root().analysis->rootWinrate < 0.526,
        "root generated comment should restore winrate");
    require(rootModel.root().analysis->candidates.size() == 1, "root generated comment should restore candidates");
    require(
        rootModel.root().analysis->candidates.front().move.color == Color::Black,
        "root generated comment candidate should use root side to move");
    require(
        rootModel.root().analysis->candidates.front().pv.size() == 2 &&
            rootModel.root().analysis->candidates.front().pv[1].color == Color::White,
        "root generated comment PV should alternate from root side to move");

    const std::string errorOnlySgf = R"SGF((;GM[1]FF[4]SZ[9];B[dd]C[User note

LizzieYzy analysis error
Visits: 999
Candidates:
C3 visits 999 winrate 99.9% score 99.9 pv C3]))SGF";
    GameModel errorOnlyModel = parser.parseGame(errorOnlySgf);
    warnings.clear();
    const int errorImported = importLegacyLizzieAnalysis(&errorOnlyModel, &warnings);
    require(errorImported == 0, "generated analysis error comments should not import as success analysis");
    require(warnings.empty(), "analysis error comments without LZ data should not warn as malformed success data");
    require(
        !errorOnlyModel.node(errorOnlyModel.root().children.front()).analysis.has_value(),
        "analysis error comments should not populate a success snapshot");
}

void testLegacyAnalysisImportRejectsNonFiniteNumericFields()
{
    const std::string legacySgf = R"SGF((;GM[1]FF[4]SZ[9]LZOP[KataGo nan 12 inf -inf
move D4 visits 12 winrate nan prior inf scoreMean nan scoreStdev -inf pv D4 E4 pvVisits 12 5 movesOwnership 0.1 nan 0.2 ownership 0.3 nan -0.3]))SGF";

    SgfParser parser;
    GameModel legacyModel = parser.parseGame(legacySgf);
    std::vector<std::string> warnings;
    const int legacyImported = importLegacyLizzieAnalysis(&legacyModel, &warnings);
    require(legacyImported == 1, "legacy analysis with finite visits should import despite non-finite scalars");
    require(warnings.empty(), "legacy analysis with ignored non-finite scalars should not warn");
    require(legacyModel.root().analysis.has_value(), "legacy analysis with finite visits should restore a snapshot");

    const AnalysisSnapshot& legacy = *legacyModel.root().analysis;
    require(legacy.rootVisits == 12, "legacy finite visits should remain usable");
    require(legacy.rootWinrate == 0.0, "legacy non-finite root winrate should be ignored");
    require(legacy.rootScoreMean == 0.0, "legacy non-finite root score should be ignored");
    require(legacy.ownership.empty(), "legacy non-finite ownership should not create persisted ownership");
    require(legacy.candidates.size() == 1, "legacy candidate with finite visits should remain");
    require(legacy.candidates.front().winrate == 0.0, "legacy non-finite candidate winrate should be ignored");
    require(legacy.candidates.front().policy == 0.0, "legacy non-finite candidate policy should be ignored");
    require(legacy.candidates.front().scoreMean == 0.0, "legacy non-finite candidate score should be ignored");
    require(legacy.candidates.front().scoreStdev == 0.0, "legacy non-finite candidate stdev should be ignored");
    require(legacy.candidates.front().ownership.empty(), "legacy non-finite candidate ownership should be discarded");

    const std::string generatedSgf = R"SGF((;GM[1]FF[4]SZ[9]C[Root note

LizzieYzy analysis
Winrate: nan
ScoreMean: inf
Visits: 64
Candidates:
D4 visits 64 winrate nan score inf stdev -inf policy nan pv D4]))SGF";

    GameModel generatedModel = parser.parseGame(generatedSgf);
    warnings.clear();
    const int generatedImported = importLegacyLizzieAnalysis(&generatedModel, &warnings);
    require(generatedImported == 1, "generated analysis with finite visits should import despite non-finite scalars");
    require(warnings.empty(), "generated analysis with ignored non-finite scalars should not warn");
    require(generatedModel.root().analysis.has_value(), "generated analysis with finite visits should restore a snapshot");

    const AnalysisSnapshot& generated = *generatedModel.root().analysis;
    require(generated.rootVisits == 64, "generated finite visits should remain usable");
    require(generated.rootWinrate == 0.0, "generated non-finite root winrate should be ignored");
    require(generated.rootScoreMean == 0.0, "generated non-finite root score should be ignored");
    require(generated.candidates.size() == 1, "generated candidate with finite visits should remain");
    require(generated.candidates.front().winrate == 0.0, "generated non-finite candidate winrate should be ignored");
    require(generated.candidates.front().scoreMean == 0.0, "generated non-finite candidate score should be ignored");
    require(generated.candidates.front().scoreStdev == 0.0, "generated non-finite candidate stdev should be ignored");
    require(generated.candidates.front().policy == 0.0, "generated non-finite candidate policy should be ignored");
}

void testLegacyAnalysisImportRejectsOverflowVisitCounts()
{
    const std::string overflowVisits = "999999999999999999999999m";
    const std::string legacySgf = "(;GM[1]FF[4]SZ[9]LZOP[KataGo 48.0 " + overflowVisits +
        " 0.4 1.5\nmove D4 visits " + overflowVisits +
        " winrate 5200 prior 1234 scoreMean 0.40 pv D4 E4 pvVisits " + overflowVisits + " 7])";

    SgfParser parser;
    GameModel legacyModel = parser.parseGame(legacySgf);
    std::vector<std::string> warnings;
    const int legacyImported = importLegacyLizzieAnalysis(&legacyModel, &warnings);
    require(legacyImported == 1, "legacy analysis with overflow visits but usable candidate should import");
    require(warnings.empty(), "legacy analysis with ignored overflow visits should not warn");
    require(legacyModel.root().analysis.has_value(), "legacy analysis with overflow visits should restore a snapshot");

    const AnalysisSnapshot& legacy = *legacyModel.root().analysis;
    require(legacy.rootVisits == 0, "legacy overflow root visits should be ignored");
    require(legacy.candidates.size() == 1, "legacy candidate with overflow visits should still import non-count fields");
    require(legacy.candidates.front().visits == 0, "legacy overflow candidate visits should be ignored");
    require(legacy.candidates.front().pvVisits.empty(), "legacy overflow PV visits should not be imported");
    require(legacy.candidates.front().winrate > 0.519 && legacy.candidates.front().winrate < 0.521, "legacy candidate winrate should remain usable");

    const std::string generatedSgf = "(;GM[1]FF[4]SZ[9]C[Root note\n\nLizzieYzy analysis\nVisits: " +
        overflowVisits + "\nCandidates:\nD4 visits " + overflowVisits +
        " winrate 52.0% score 0.40 pv D4 E4 pvVisits " + overflowVisits + " 7])";

    GameModel generatedModel = parser.parseGame(generatedSgf);
    warnings.clear();
    const int generatedImported = importLegacyLizzieAnalysis(&generatedModel, &warnings);
    require(generatedImported == 1, "generated analysis with overflow visits but usable candidate should import");
    require(warnings.empty(), "generated analysis with ignored overflow visits should not warn");
    require(generatedModel.root().analysis.has_value(), "generated analysis with overflow visits should restore a snapshot");

    const AnalysisSnapshot& generated = *generatedModel.root().analysis;
    require(generated.rootVisits == 0, "generated overflow root visits should be ignored");
    require(generated.candidates.size() == 1, "generated candidate with overflow visits should still import non-count fields");
    require(generated.candidates.front().visits == 0, "generated overflow candidate visits should be ignored");
    require(generated.candidates.front().pvVisits.empty(), "generated overflow PV visits should not be imported");
    require(generated.candidates.front().winrate > 0.519 && generated.candidates.front().winrate < 0.521, "generated candidate winrate should remain usable");
}

void testLz2DualAnalysisIsPreservedButNotImported()
{
    const std::string sgf =
        "(;GM[1]FF[4]SZ[9]LZ2[dual-root-keep];B[dd]LZ2[dual-child-keep])";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&model, &warnings);
    require(imported == 0, "LZ2 dual-engine data should not import as primary analysis");
    require(warnings.empty(), "LZ2-only nodes should not warn during primary legacy import");
    require(!model.root().analysis.has_value(), "LZ2-only root should not receive primary analysis");
    const NodeId childId = model.root().children.front();
    require(!model.node(childId).analysis.has_value(), "LZ2-only child should not receive primary analysis");

    const std::string preserved = writeSgf(model);
    require(preserved.find("LZ2[dual-root-keep]") != std::string::npos, "writer should preserve root LZ2");
    require(preserved.find("LZ2[dual-child-keep]") != std::string::npos, "writer should preserve child LZ2");

    AnalysisSnapshot rootAnalysis;
    rootAnalysis.rootVisits = 17;
    rootAnalysis.rootWinrate = 0.53;
    rootAnalysis.rootScoreMean = 0.8;
    model.root().analysis = rootAnalysis;

    AnalysisSnapshot childAnalysis;
    childAnalysis.rootVisits = 19;
    childAnalysis.rootWinrate = 0.47;
    childAnalysis.rootScoreMean = -0.6;
    model.node(childId).analysis = childAnalysis;

    const std::string writtenWithPrimary = writeSgf(model);
    require(writtenWithPrimary.find("LZOP[") != std::string::npos, "root primary analysis should still be written");
    require(writtenWithPrimary.find("LZ[") != std::string::npos, "child primary analysis should still be written");
    require(
        writtenWithPrimary.find("LZ2[dual-root-keep]") != std::string::npos,
        "writing root primary analysis should preserve LZ2");
    require(
        writtenWithPrimary.find("LZ2[dual-child-keep]") != std::string::npos,
        "writing child primary analysis should preserve LZ2");
}

void testSgfAnalysisExportFiltersIncompleteOwnership()
{
    GameModel model(BoardSize::square(9));
    AnalysisSnapshot analysis;
    analysis.rootVisits = 10;
    analysis.rootWinrate = 0.55;
    analysis.rootScoreMean = 1.1;
    analysis.ownership = {0.1, -0.1};
    MoveCandidate candidate{Move::play(Color::Black, Point{4, 4}), 10, 0.55, 1.1};
    candidate.ownership = {0.2, -0.2};
    analysis.candidates.push_back(candidate);
    model.root().analysis = analysis;

    const std::string incomplete = writeSgf(model);
    require(incomplete.find("LZOP[") != std::string::npos, "SGF analysis export should keep usable analysis");
    require(
        incomplete.find("movesOwnership") == std::string::npos,
        "SGF analysis export should not write incomplete candidate ownership");
    require(
        incomplete.find(" ownership") == std::string::npos,
        "SGF analysis export should not write incomplete root ownership");

    model.root().analysis->ownership = ownershipValues(model.boardSize(), 0.1, -0.1);
    model.root().analysis->candidates.front().ownership = ownershipValues(model.boardSize(), 0.2, -0.2);

    const std::string complete = writeSgf(model);
    require(
        complete.find("movesOwnership 0.200000 -0.200000") != std::string::npos,
        "SGF analysis export should write complete candidate ownership");
    require(
        complete.find(" ownership 0.100000 -0.100000") != std::string::npos,
        "SGF analysis export should write complete root ownership");

    SgfParser parser;
    GameModel restored = parser.parseGame(complete);
    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&restored, &warnings);
    require(imported == 1, "SGF analysis export with complete ownership should import");
    require(warnings.empty(), "SGF analysis export with complete ownership should not warn");
    require(restored.root().analysis.has_value(), "SGF analysis export should restore root analysis");
    require(
        restored.root().analysis->ownership.size() == static_cast<std::size_t>(model.boardSize().pointCount()),
        "SGF analysis export should restore complete root ownership");
    require(
        restored.root().analysis->candidates.front().ownership.size() ==
            static_cast<std::size_t>(model.boardSize().pointCount()),
        "SGF analysis export should restore complete candidate ownership");
}

void testSgfAnalysisExportForLegacyImport()
{
    GameModel model(BoardSize::square(9));
    model.gameInfo().komi = 7.5;
    model.root().comment = "User note";
    const NodeId first = model.addMove(Move::play(Color::Black, Point{3, 3}));

    AnalysisSnapshot rootAnalysis;
    rootAnalysis.rootVisits = 11;
    rootAnalysis.rootWinrate = 0.62;
    rootAnalysis.rootScoreMean = 1.25;
    rootAnalysis.ownership = {0.1, -0.1};
    rootAnalysis.candidates.push_back(MoveCandidate{Move::play(Color::Black, Point{4, 4}), 11, 0.62, 1.25});
    model.root().analysis = rootAnalysis;

    AnalysisSnapshot nodeAnalysis;
    nodeAnalysis.rootVisits = 96;
    nodeAnalysis.rootWinrate = 0.58;
    nodeAnalysis.rootScoreMean = -0.7;
    nodeAnalysis.winratePerspective = Color::White;
    MoveCandidate candidate{Move::play(Color::White, Point{2, 6}), 96, 0.58, -0.7};
    candidate.policy = 0.1234;
    candidate.scoreStdev = 4.5;
    candidate.pv = {Move::play(Color::White, Point{2, 6}), Move::pass(Color::Black), Move::play(Color::White, Point{4, 5})};
    candidate.pvVisits = {96, 50, 12};
    nodeAnalysis.candidates.push_back(candidate);
    model.node(first).analysis = nodeAnalysis;

    const std::string sgf = writeSgf(model);
    require(sgf.find("LZOP[") != std::string::npos, "writer should export root analysis as LZOP");
    require(sgf.find("LZ[") != std::string::npos, "writer should export node analysis as LZ");
    require(sgf.find("LZYPERSP[B]") != std::string::npos, "writer should export root winrate perspective");
    require(sgf.find("LZYPERSP[W]") != std::string::npos, "writer should export node winrate perspective");
    require(sgf.find("LizzieYzy analysis") != std::string::npos, "writer should append readable analysis comment");
    require(
        sgf.find("stdev 4.50 policy 12.3% pv C3 pass E4 pvVisits 96 50 12") != std::string::npos,
        "writer should include stdev, policy, and PV visits in readable analysis comments");
    require(sgf.find("User note") != std::string::npos, "writer should preserve user comment");

    SgfParser parser;
    GameModel restored = parser.parseGame(sgf);
    restored.root().analysisError = "old root import failure";
    restored.node(restored.root().children.front()).analysisError = "old node import failure";
    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&restored, &warnings);
    require(imported == 2, "exported LZ data should import into fresh model");
    require(warnings.empty(), "exported LZ data should not warn");

    require(restored.root().analysis.has_value(), "restored root should have analysis");
    require(restored.root().analysisError.empty(), "legacy root import should clear stale analysis error");
    require(restored.root().analysis->rootVisits == 11, "restored root visits should match");
    require(restored.root().analysis->rootWinrate > 0.619 && restored.root().analysis->rootWinrate < 0.621,
            "restored root winrate should match");
    require(restored.root().analysis->winratePerspective == Color::Black, "restored root perspective should match");
    const GameNode& restoredNode = restored.node(restored.root().children.front());
    require(restoredNode.analysis.has_value(), "restored node should have analysis");
    require(restoredNode.analysisError.empty(), "legacy node import should clear stale analysis error");
    require(
        restoredNode.analysis->rootWinrate > 0.579 && restoredNode.analysis->rootWinrate < 0.581,
        "restored white-perspective node winrate should match");
    require(
        restoredNode.analysis->rootScoreMean > -0.71 && restoredNode.analysis->rootScoreMean < -0.69,
        "restored white-perspective node score should match");
    require(restoredNode.analysis->candidates.size() == 1, "restored node candidate should match");
    require(restoredNode.analysis->winratePerspective == Color::White, "restored node perspective should match");
    require(restoredNode.analysis->candidates.front().move.color == Color::White, "restored candidate color should match");
    require(restoredNode.analysis->candidates.front().pvVisits.size() == 3, "restored candidate pvVisits should match");

    const std::string rewritten = writeSgf(restored);
    require(
        countOccurrences(rewritten, "LizzieYzy analysis") == countOccurrences(sgf, "LizzieYzy analysis"),
        "rewriter should not duplicate generated analysis comments");
}

void testWideBoardSgfAnalysisExportImportsSgfCoordinates()
{
    GameModel model(BoardSize{26, 9});
    AnalysisSnapshot analysis;
    analysis.rootVisits = 33;
    analysis.rootWinrate = 0.57;
    analysis.rootScoreMean = 1.1;
    MoveCandidate candidate{Move::play(Color::Black, Point{25, 0}), 33, 0.57, 1.1};
    candidate.policy = 0.2;
    candidate.scoreStdev = 3.5;
    candidate.pv = {
        Move::play(Color::Black, Point{25, 0}),
        Move::play(Color::White, Point{0, 8}),
        Move::pass(Color::Black),
    };
    candidate.pvVisits = {33, 12, 4};
    analysis.candidates.push_back(candidate);
    model.root().analysis = analysis;

    const std::string sgf = writeSgf(model);
    require(sgf.find("move za visits 33") != std::string::npos, "wide exported LZOP should use SGF move coordinates");
    require(sgf.find("pv za ai pass") != std::string::npos, "wide exported LZOP should use SGF PV coordinates");
    require(sgf.find("move  visits") == std::string::npos, "wide exported LZOP should not contain an empty move");

    SgfParser parser;
    GameModel restored = parser.parseGame(sgf);
    std::vector<std::string> warnings;
    const int imported = importLegacyLizzieAnalysis(&restored, &warnings);
    require(imported == 1, "wide exported LZOP should import into a fresh model");
    require(warnings.empty(), "wide exported LZOP import should not warn");
    require(restored.root().analysis.has_value(), "wide restored root should have analysis");
    require(restored.root().analysis->candidates.size() == 1, "wide restored root should keep one candidate");

    const MoveCandidate& restoredCandidate = restored.root().analysis->candidates.front();
    require(
        restoredCandidate.move.type == MoveType::Play && restoredCandidate.move.point == Point{25, 0},
        "wide exported SGF candidate coordinate should import");
    require(restoredCandidate.pv.size() == 3, "wide exported SGF PV should import");
    require(restoredCandidate.pv[0].point == Point{25, 0}, "wide exported SGF PV first move should import");
    require(restoredCandidate.pv[1].point == Point{0, 8}, "wide exported SGF PV second move should import");
    require(restoredCandidate.pv[2].type == MoveType::Pass, "wide exported SGF PV pass should import");
    require(restoredCandidate.pvVisits.size() == 3, "wide exported SGF PV visits should import");
}

}  // namespace

int main()
{
    try {
        testCollectAndBuildPlan();
        testBatchAnalysisUsesFiniteKomi();
        testDuplicateRequestedNodesAreDeduplicated();
        testRootInitialStonesAreCanonicalized();
        testMidGameSetupUsesSnapshotRequest();
        testRootAeSetupIsCanonicalized();
        testInitialPlayerFromRootSideToMove();
        testFinalPlayerToMoveOverrideUsesSnapshotRequest();
        testWideBoardAnalysisJsonUsesExtendedCoordinates();
        testInvalidPositionSkipped();
        testInvalidSetupPositionSkipped();
        testResignNodesSkipped();
        testApplyAnalysisResponse();
        testAnalysisSidecarRoundTrip();
        testAnalysisSidecarExportRejectsNonFiniteValues();
        testAnalysisSidecarExportFiltersIncompleteOwnership();
        testAnalysisSidecarImportRejectsMalformedOwnershipArrays();
        testAnalysisSidecarExportSortsCandidates();
        testAnalysisSidecarExportClampsNegativeVisits();
        testAnalysisSidecarNormalizesRateScales();
        testAnalysisSidecarImportsNumericStringFields();
        testAnalysisSidecarImportsKataGoAliasFields();
        testAnalysisSidecarDiscardsIncompleteOwnership();
        testAnalysisSidecarSortsCandidatesAndFallsBackToBest();
        testAnalysisSidecarRejectsInvalidVisitCounts();
        testAnalysisSidecarDiscardsMismatchedPvVisits();
        testAnalysisSidecarRejectsNonFiniteNumericFields();
        testAnalysisSidecarInvalidRootFieldsFallBackToBestCandidate();
        testAnalysisSidecarCollectionGameIndex();
        testAnalysisSidecarErrorRoundTrip();
        testAnalysisSidecarExportKeepsErrorsExclusive();
        testAnalysisSidecarUnsupportedFormatIsRejected();
        testAnalysisSidecarInvalidNodesArrayWarns();
        testAnalysisSidecarMalformedNodeEntryWarnsAndContinues();
        testAnalysisSidecarRejectsFractionalNumericNodeId();
        testAnalysisSidecarLegacyKeyRejectsAmbiguousPlayer();
        testAnalysisSidecarTakesPrecedenceOverLegacySgfAnalysis();
        testAnalysisPositionKeyCanonicalizesRuleAliases();
        testAnalysisSidecarPositionKeyIgnoresSetupOrder();
        testAnalysisSidecarPositionKeyIgnoresRedundantRootAe();
        testAnalysisSidecarInvalidPvMoveDoesNotConsumeTurn();
        testAnalysisSidecarRoundTripsPassAndResignCandidates();
        testLegacyLizzieAnalysisImport();
        testLegacyLizzieAnalysisImportAcceptsMixedCaseKeys();
        testLegacyLizzieAnalysisImportCandidateOwnershipAliases();
        testLegacyLizzieAnalysisImportRejectsMalformedCandidateOwnership();
        testLegacyGeneratedAnalysisImportRejectsMalformedPvVisits();
        testLegacyGeneratedAnalysisImportRejectsMalformedPv();
        testLegacyLizzieAnalysisImportSortsCandidatesAndFallsBackToBest();
        testLegacyLizzieAnalysisImportExtensionFieldBoundaries();
        testLegacyLizzieAnalysisImportAcceptsMixedCasePropertyNames();
        testGeneratedAnalysisCommentImport();
        testGeneratedAnalysisCommentImportCandidateOwnership();
        testGeneratedAnalysisCommentImportRootOwnership();
        testGeneratedAnalysisCommentRejectsMalformedOwnership();
        testGeneratedAnalysisCommentImportSortsCandidatesAndFallsBackToBest();
        testGeneratedAnalysisCommentImportAcceptsMixedCaseHeaderAndKeys();
        testGeneratedAnalysisCommentImportExtensionFieldBoundaries();
        testGeneratedAnalysisCommentImportRootAndIgnoresErrorComment();
        testLegacyAnalysisImportRejectsNonFiniteNumericFields();
        testLegacyAnalysisImportRejectsOverflowVisitCounts();
        testLz2DualAnalysisIsPreservedButNotImported();
        testSgfAnalysisExportFiltersIncompleteOwnership();
        testSgfAnalysisExportForLegacyImport();
        testWideBoardSgfAnalysisExportImportsSgfCoordinates();
    } catch (const std::exception& error) {
        std::cerr << "batch_analysis_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "batch_analysis_tests passed\n";
    return EXIT_SUCCESS;
}
