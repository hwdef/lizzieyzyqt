#include "BatchAnalysis.h"
#include "GtpProtocol.h"
#include "RealtimeAnalysis.h"
#include "Sgf.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
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

int mainLineLength(const GameModel& model)
{
    int count = 0;
    NodeId nodeId = model.rootId();
    while (!model.node(nodeId).children.empty()) {
        nodeId = model.node(nodeId).children.front();
        ++count;
    }
    return count;
}

int firstChildLineLength(const GameModel& model, NodeId nodeId)
{
    int count = 1;
    while (!model.node(nodeId).children.empty()) {
        nodeId = model.node(nodeId).children.front();
        ++count;
    }
    return count;
}

void testLargeSgfRoundTrip()
{
    std::ostringstream sgf;
    sgf << "(;GM[1]FF[4]SZ[19]KM[7.5]";
    for (int index = 0; index < 300; ++index) {
        const Point point{index % 19, index / 19};
        sgf << ';' << (index % 2 == 0 ? 'B' : 'W') << '[' << pointToSgf(point) << ']';
    }
    sgf << ')';

    SgfParser parser;
    const GameModel model = parser.parseGame(sgf.str());
    require(mainLineLength(model) == 300, "large SGF should parse every main-line move");

    const std::string written = writeSgf(model);
    const GameModel reparsed = parser.parseGame(written);
    require(mainLineLength(reparsed) == 300, "large SGF should write and reparse every main-line move");
}

void testBranchHeavySgfRoundTrip()
{
    constexpr int kBranchCount = 72;
    constexpr int kBranchDepth = 8;
    std::ostringstream sgf;
    sgf << "(;GM[1]FF[4]SZ[19]KM[7.5];B[aa]";
    for (int branch = 0; branch < kBranchCount; ++branch) {
        sgf << '(';
        for (int ply = 0; ply < kBranchDepth; ++ply) {
            const char color = ply % 2 == 0 ? 'W' : 'B';
            const Point point{(branch + ply * 5) % 19, (1 + (branch / 19) * 4 + ply * 2) % 19};
            sgf << ';' << color << '[' << pointToSgf(point) << ']';
        }
        sgf << ')';
    }
    sgf << ')';

    SgfParser parser;
    const GameModel model = parser.parseGame(sgf.str());
    const NodeId first = model.root().children.front();
    require(
        model.node(first).children.size() == kBranchCount,
        "branch-heavy SGF should parse every first-move variation");
    require(
        firstChildLineLength(model, model.node(first).children.front()) == kBranchDepth,
        "branch-heavy SGF should parse the first variation depth");
    require(
        firstChildLineLength(model, model.node(first).children.back()) == kBranchDepth,
        "branch-heavy SGF should parse the final variation depth");

    const std::string written = writeSgf(model);
    const GameModel reparsed = parser.parseGame(written);
    const NodeId reparsedFirst = reparsed.root().children.front();
    require(
        reparsed.node(reparsedFirst).children.size() == kBranchCount,
        "branch-heavy SGF should preserve every variation after write");
    require(
        firstChildLineLength(reparsed, reparsed.node(reparsedFirst).children.front()) == kBranchDepth,
        "branch-heavy SGF should preserve first variation depth after write");
    require(
        firstChildLineLength(reparsed, reparsed.node(reparsedFirst).children.back()) == kBranchDepth,
        "branch-heavy SGF should preserve final variation depth after write");
}

std::string longPv(lizzie::core::BoardSize boardSize)
{
    std::string pv;
    for (int index = 0; index < 80; ++index) {
        const Point point{index % boardSize.width, (index / boardSize.width) % boardSize.height};
        pv += ' ';
        pv += formatPointForGtp(point, boardSize);
    }
    return pv;
}

void testHighFrequencyRealtimeAnalysis()
{
    const BoardSize boardSize = BoardSize::square(19);
    const std::string pv = longPv(boardSize);
    RealtimeAnalysisAccumulator accumulator(boardSize, Color::Black);

    std::optional<AnalysisSnapshot> snapshot;
    for (int index = 0; index < 200; ++index) {
        const Point point{index % boardSize.width, index / boardSize.width};
        std::ostringstream line;
        line << "info move " << formatPointForGtp(point, boardSize)
             << " visits " << (index + 1)
             << " winrate 0.5 scoreMean 0.0 scoreStdev 5.0 policy 0.01 pv" << pv;
        snapshot = accumulator.processLine(line.str());
    }

    require(snapshot.has_value(), "high-frequency analysis should produce snapshots");
    require(snapshot->candidates.size() == 200, "high-frequency analysis should retain distinct candidates");
    require(snapshot->candidates.front().visits == 200, "high-frequency analysis should keep candidates sorted");
    require(snapshot->candidates.front().pv.size() == 80, "long PV should be retained");
}

void testBatchPlanForHundredMoves()
{
    GameModel model(BoardSize::square(19));
    NodeId last = model.rootId();
    for (int index = 0; index < 100; ++index) {
        const Color color = index % 2 == 0 ? Color::Black : Color::White;
        last = model.addMove(Move::pass(color));
    }

    const BatchAnalysisPlan plan = buildBatchAnalysisPlan(model, {}, {});
    require(plan.warnings.empty(), "hundred-move batch plan should not warn");
    require(plan.items.size() == 101, "hundred-move batch plan should include root and every move");
    require(plan.items.back().nodeId == last, "hundred-move batch plan should preserve depth-first order");
    require(plan.items.back().request.moves.size() == 100, "hundred-move batch request should include full path");
}

}  // namespace

int main()
{
    try {
        testLargeSgfRoundTrip();
        testBranchHeavySgfRoundTrip();
        testHighFrequencyRealtimeAnalysis();
        testBatchPlanForHundredMoves();
    } catch (const std::exception& error) {
        std::cerr << "performance_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "performance_tests passed\n";
    return EXIT_SUCCESS;
}
