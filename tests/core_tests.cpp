#include "BoardPosition.h"
#include "GameModel.h"
#include "Sgf.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace lizzie::core;

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

int countOccurrences(const std::string& text, const std::string& needle)
{
    int count = 0;
    std::size_t position = 0;
    while ((position = text.find(needle, position)) != std::string::npos) {
        ++count;
        position += needle.size();
    }
    return count;
}

void testCoordinates()
{
    const BoardSize size = BoardSize::square(19);
    const auto point = pointFromSgf("dd", size);
    require(point.has_value(), "dd should parse as a point");
    require(point->x == 3 && point->y == 3, "dd should map to zero-based 3,3");
    require(pointToSgf(*point) == "dd", "point should round-trip to SGF");

    const auto pass = moveFromSgf(Color::White, "", size);
    require(pass.has_value() && pass->type == MoveType::Pass, "empty SGF move should be pass");
    const auto oldStylePass = moveFromSgf(Color::Black, "tt", size);
    require(oldStylePass.has_value() && oldStylePass->type == MoveType::Pass, "outside board move should be pass");
    require(rulesetFromString("New Zealand") == Ruleset::NewZealand, "rules parser should accept spaced rules names");
    require(rulesetFromString(" NZ ") == Ruleset::NewZealand, "rules parser should trim SGF rule aliases");
    require(rulesetFromString("\tjp\n") == Ruleset::Japanese, "rules parser should trim tabbed SGF rule aliases");
    require(rulesetFromString("Tromp_Taylor") == Ruleset::TrompTaylor, "rules parser should accept underscored rules names");
}

void testCaptures()
{
    BoardPosition board(BoardSize::square(3));
    board.playMove(Move::play(Color::Black, Point{0, 1}));
    board.playMove(Move::play(Color::White, Point{1, 1}));
    board.playMove(Move::play(Color::Black, Point{1, 0}));
    board.playMove(Move::play(Color::Black, Point{2, 1}));
    const PlayResult result = board.playMove(Move::play(Color::Black, Point{1, 2}));

    require(result.ok, "capturing move should be legal");
    require(result.captured.size() == 1, "one stone should be captured");
    require(board.at(Point{1, 1}) == Stone::Empty, "captured point should be empty");
    require(board.blackCaptures() == 1, "black capture count should increase");
}

void testSuicide()
{
    BoardPosition board(BoardSize::square(3));
    board.placeSetupStone(Color::Black, Point{0, 1});
    board.placeSetupStone(Color::Black, Point{1, 0});
    board.placeSetupStone(Color::Black, Point{2, 1});
    board.placeSetupStone(Color::Black, Point{1, 2});

    std::string error;
    require(!board.canPlay(Color::White, Point{1, 1}, &error), "suicide should be illegal");
    require(!error.empty(), "illegal suicide should report an error");
}

void testCaptureIsNotSuicide()
{
    BoardPosition board(BoardSize::square(3));
    board.placeSetupStone(Color::White, Point{0, 0});
    board.placeSetupStone(Color::White, Point{2, 0});
    board.placeSetupStone(Color::Black, Point{1, 0});
    board.placeSetupStone(Color::Black, Point{0, 1});
    board.placeSetupStone(Color::Black, Point{2, 1});
    board.placeSetupStone(Color::Black, Point{1, 2});

    std::string error;
    require(board.canPlay(Color::White, Point{1, 1}, &error), "capture should be legal even when the played point has no immediate liberties");
    const PlayResult result = board.playMove(Move::play(Color::White, Point{1, 1}));
    require(result.ok, "capture that creates liberties should play successfully");
    require(result.captured.size() == 1 && result.captured.front() == Point{1, 0}, "capture should remove the adjacent black group");
    require(board.at(Point{1, 1}) == Stone::White, "capturing stone should remain on the board");
    require(board.whiteCaptures() == 1, "white capture count should increase");
}

void testKo()
{
    BoardPosition board(BoardSize::square(3));
    board.placeSetupStone(Color::White, Point{1, 0});
    board.placeSetupStone(Color::Black, Point{2, 0});
    board.placeSetupStone(Color::White, Point{0, 1});
    board.placeSetupStone(Color::Black, Point{1, 1});

    const PlayResult result = board.playMove(Move::play(Color::Black, Point{0, 0}));
    require(result.ok, "ko capture should be legal");
    require(result.captured.size() == 1 && result.captured.front() == Point{1, 0}, "ko capture should remove expected stone");
    require(board.ko().has_value() && *board.ko() == Point{1, 0}, "ko point should be recorded");
    require(!board.canPlay(Color::White, Point{1, 0}), "immediate ko recapture should be illegal");

    board.playMove(Move::pass(Color::White));
    board.playMove(Move::pass(Color::Black));
    require(board.canPlay(Color::White, Point{1, 0}), "ko recapture should be legal after intervening passes");
}

void testPositionHash()
{
    BoardPosition first(BoardSize::square(9));
    BoardPosition second(BoardSize::square(9));
    require(first.zobrist() == second.zobrist(), "empty equivalent boards should have identical hashes");

    const std::uint64_t emptyHash = first.zobrist();
    first.playMove(Move::play(Color::Black, Point{3, 3}));
    require(first.zobrist() != emptyHash, "hash should change after a play move");

    second.playMove(Move::play(Color::Black, Point{3, 3}));
    require(first.zobrist() == second.zobrist(), "equivalent played positions should have identical hashes");

    second.playMove(Move::pass(Color::White));
    require(first.zobrist() != second.zobrist(), "hash should include side to move");

    first.clear();
    require(first.zobrist() == emptyHash, "clear should restore the empty-board hash");

    BoardPosition rectangular(BoardSize{9, 13});
    require(rectangular.zobrist() != emptyHash, "hash should include board size");
}

void testGameModelPosition()
{
    GameModel model(BoardSize::square(5));
    model.root().setupStones.black.push_back(Point{0, 0});
    const NodeId first = model.addMove(Move::play(Color::White, Point{1, 0}));
    const NodeId variation = model.addChild(model.rootId(), Move::play(Color::White, Point{0, 1}));

    const BoardPosition firstPosition = model.positionAt(first);
    require(firstPosition.at(Point{0, 0}) == Stone::Black, "root setup stone should apply");
    require(firstPosition.at(Point{1, 0}) == Stone::White, "main child move should apply");

    const BoardPosition variationPosition = model.positionAt(variation);
    require(variationPosition.at(Point{1, 0}) == Stone::Empty, "variation should not include sibling move");
    require(variationPosition.at(Point{0, 1}) == Stone::White, "variation move should apply");
}

void testGameModelPositionReportsInvalidSetup()
{
    GameModel blackSetup(BoardSize::square(5));
    blackSetup.root().setupStones.black.push_back(Point{5, 0});

    std::string blackError;
    static_cast<void>(blackSetup.positionAt(blackSetup.rootId(), &blackError));
    require(
        blackError.find("setup stone is outside the board") != std::string::npos,
        "out-of-board setup stones should report a position error");

    GameModel emptySetup(BoardSize::square(5));
    emptySetup.root().setupStones.empty.push_back(Point{-1, 0});

    std::string emptyError;
    static_cast<void>(emptySetup.positionAt(emptySetup.rootId(), &emptyError));
    require(
        emptyError.find("setup clear point is outside the board") != std::string::npos,
        "out-of-board setup clears should report a position error");
}

void testGameModelTryAddMoveValidatesLocalRules()
{
    GameModel model(BoardSize::square(3));

    std::string error;
    const std::optional<NodeId> first = model.tryAddMove(Move::play(Color::Black, Point{0, 0}), &error);
    require(first.has_value(), "checked add move should accept a legal move");
    require(error.empty(), "checked add move should clear stale errors on success");
    require(model.currentId() == *first, "checked add move should advance current node on success");
    require(model.node(*first).move->point == Point{0, 0}, "checked add move should store the played move");

    const NodeId currentBeforeOccupied = model.currentId();
    const std::optional<NodeId> occupied = model.tryAddMove(Move::play(Color::White, Point{0, 0}), &error);
    require(!occupied.has_value(), "checked add move should reject occupied points");
    require(error == "point is already occupied", "checked add move should report occupied-point errors");
    require(model.currentId() == currentBeforeOccupied, "rejected occupied moves should not change current node");
    require(model.node(currentBeforeOccupied).children.empty(), "rejected occupied moves should not create child nodes");

    const std::optional<NodeId> wrongPlayer = model.tryAddMove(Move::play(Color::Black, Point{1, 0}), &error);
    require(!wrongPlayer.has_value(), "checked add move should reject moves by the wrong side");
    require(error == "wrong player to move", "checked add move should report wrong-player errors");
    require(model.currentId() == currentBeforeOccupied, "rejected wrong-player moves should not change current node");

    const std::optional<NodeId> pass = model.tryAddMove(Move::pass(Color::White), &error);
    require(pass.has_value(), "checked add move should accept legal passes");
    require(model.currentId() == *pass, "legal pass should advance current node");
    require(model.currentPosition().sideToMove() == Color::Black, "legal pass should update side to move");

    GameModel existingBranch(BoardSize::square(5));
    const std::optional<NodeId> firstBranch =
        existingBranch.tryAddMove(Move::play(Color::Black, Point{1, 1}), &error);
    require(firstBranch.has_value(), "test should create an initial branch");
    require(existingBranch.setCurrent(existingBranch.rootId()), "test should return to root");
    const std::optional<NodeId> reusedBranch =
        existingBranch.tryAddMove(Move::play(Color::Black, Point{1, 1}), &error);
    require(reusedBranch == firstBranch, "checked add move should reuse an existing matching child");
    require(existingBranch.currentId() == *firstBranch, "reused existing child should become current");
    require(existingBranch.root().children.size() == 1, "reused existing child should not duplicate branches");
    require(existingBranch.setCurrent(existingBranch.rootId()), "test should return to root before adding variation");
    const std::optional<NodeId> variationBranch =
        existingBranch.tryAddMove(Move::play(Color::Black, Point{2, 2}), &error);
    require(variationBranch.has_value(), "different legal moves should still create variations");
    require(existingBranch.root().children.size() == 2, "new variation should be added as a sibling");

    GameModel suicide(BoardSize::square(3));
    suicide.root().setupStones.black = {Point{0, 1}, Point{1, 0}, Point{2, 1}, Point{1, 2}};
    suicide.root().playerToMove = Color::White;
    const std::optional<NodeId> suicideMove = suicide.tryAddMove(Move::play(Color::White, Point{1, 1}), &error);
    require(!suicideMove.has_value(), "checked add move should reject suicide");
    require(error == "suicide is not legal", "checked add move should report suicide errors");
    require(suicide.currentId() == suicide.rootId(), "rejected suicide should keep current at root");
    require(suicide.root().children.empty(), "rejected suicide should not create child nodes");

    const NodeId rawIllegal = suicide.addMove(Move::play(Color::White, Point{1, 1}));
    require(suicide.currentId() == rawIllegal, "raw addMove should remain available for imported bad records");
    std::string positionError;
    static_cast<void>(suicide.currentPosition(&positionError));
    require(positionError == "suicide is not legal", "raw illegal records should remain diagnosable");
}

void testBranchEditing()
{
    GameModel model(BoardSize::square(5));
    const NodeId first = model.addMove(Move::play(Color::Black, Point{1, 1}));
    const NodeId mainReply = model.addChild(first, Move::play(Color::White, Point{2, 2}));
    const NodeId variationReply = model.addChild(first, Move::play(Color::White, Point{3, 3}));
    const NodeId variationContinuation = model.addChild(variationReply, Move::play(Color::Black, Point{4, 4}));
    require(model.setCurrent(variationContinuation), "test should set current to branch child");

    require(model.promoteVariation(variationReply), "variation promotion should succeed");
    require(model.node(first).children.front() == variationReply, "promoted variation should become main child");
    require(model.node(first).children.back() == mainReply, "previous main child should remain as variation");
    require(!model.promoteVariation(model.rootId()), "root promotion should fail");

    require(model.deleteSubtree(variationReply), "variation subtree delete should succeed");
    require(model.currentId() == first, "deleting current subtree should move current to parent");
    require(model.findNode(variationReply) == nullptr, "deleted variation root should be removed");
    require(model.findNode(variationContinuation) == nullptr, "deleted variation descendants should be removed");
    require(model.node(first).children.size() == 1 && model.node(first).children.front() == mainReply,
            "remaining sibling should stay attached");
    require(!model.deleteSubtree(model.rootId()), "root delete should fail");
}

void testAnalysisInvalidation()
{
    GameModel model(BoardSize::square(5));
    const NodeId first = model.addMove(Move::play(Color::Black, Point{1, 1}));
    const NodeId reply = model.addChild(first, Move::play(Color::White, Point{2, 2}));
    const NodeId variation = model.addChild(model.rootId(), Move::play(Color::Black, Point{3, 3}));

    model.root().analysis = AnalysisSnapshot{};
    model.node(first).analysis = AnalysisSnapshot{};
    model.node(first).analysisError = "old analysis error";
    model.node(first).properties["LZ"] = {"old analysis"};
    model.node(first).properties["LZYERR"] = {"old error"};
    model.node(first).properties["LZYPERSP"] = {"W"};
    model.node(reply).analysis = AnalysisSnapshot{};
    model.node(reply).analysisError = "old descendant error";
    model.node(variation).analysis = AnalysisSnapshot{};

    require(model.clearAnalysisAt(first), "node analysis clear should accept an existing node");
    require(!model.node(first).analysis.has_value(), "node analysis clear should clear the target analysis");
    require(model.node(first).analysisError.empty(), "node analysis clear should clear the target error");
    require(!model.node(first).properties.contains("LZ"), "node analysis clear should remove stale LZ");
    require(!model.node(first).properties.contains("LZYERR"), "node analysis clear should remove stale LZYERR");
    require(!model.node(first).properties.contains("LZYPERSP"), "node analysis clear should remove stale perspective");
    require(model.node(reply).analysis.has_value(), "node analysis clear should preserve descendants");
    require(!model.clearAnalysisAt(999), "node analysis clear should reject missing nodes");

    model.node(first).analysis = AnalysisSnapshot{};
    model.node(first).analysisError = "old analysis error";
    require(model.clearAnalysisFrom(first), "analysis clear should accept an existing subtree");
    require(model.root().analysis.has_value(), "subtree clear should preserve ancestor analysis");
    require(!model.node(first).analysis.has_value(), "subtree root analysis should clear");
    require(model.node(first).analysisError.empty(), "subtree root analysis error should clear");
    require(!model.node(reply).analysis.has_value(), "subtree descendant analysis should clear");
    require(model.node(reply).analysisError.empty(), "subtree descendant analysis error should clear");
    require(model.node(variation).analysis.has_value(), "subtree clear should preserve sibling analysis");
    require(!model.clearAnalysisFrom(999), "analysis clear should reject missing nodes");

    model.clearAllAnalysis();
    require(!model.root().analysis.has_value(), "clear all should clear root analysis");
    require(!model.node(variation).analysis.has_value(), "clear all should clear remaining node analysis");
}

void testResignRoundTrip()
{
    GameModel model(BoardSize::square(9));
    const NodeId resign = model.addMove(Move::resign(Color::Black));
    require(model.node(resign).move.has_value(), "resign node should store a move");
    require(model.node(resign).move->type == MoveType::Resign, "resign node should store resign move type");

    const std::string written = writeSgf(model);
    require(written.find("B[resign]") != std::string::npos, "writer should encode black resign");

    SgfParser parser;
    GameModel restored = parser.parseGame(written);
    const GameNode& restoredNode = restored.node(restored.root().children.front());
    require(restoredNode.move.has_value(), "resign SGF should parse as a move");
    require(restoredNode.move->type == MoveType::Resign, "resign SGF should parse as resign");
    require(restoredNode.move->color == Color::Black, "resign SGF should preserve color");
}

void testSgfRoundTrip()
{
    const std::string sgf =
        "(;GM[1]FF[4]SZ[9]KM[6.5]PB[Black]PW[White]C[root]XX[keep]"
        ";B[dd]C[first]MN[12](;W[]LB[dd:A]CR[ee]SQ[ff]MA[gg])(;W[ee]AB[aa][bb]TR[cc]))";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    require(model.boardSize().width == 9 && model.boardSize().height == 9, "SGF board size should parse");
    require(model.gameInfo().komi.has_value() && *model.gameInfo().komi == 6.5, "SGF komi should parse");
    model.gameInfo().blackRank = "1d";
    model.gameInfo().whiteRank = "2d";
    model.gameInfo().result = "B+R";
    model.gameInfo().date = "2026-06-29";
    model.gameInfo().source = "unit-test";
    model.gameInfo().rules = "Chinese";
    model.gameInfo().handicap = 2;
    require(model.root().properties.contains("XX"), "unknown root property should be preserved");
    require(model.root().children.size() == 1, "root should have one main child");

    const GameNode& first = model.node(model.root().children.front());
    require(first.move.has_value() && first.move->type == MoveType::Play, "first move should parse");
    require(first.moveNumber.has_value() && *first.moveNumber == 12, "MN should parse as move number");
    require(first.children.size() == 2, "move should have two variations");
    const GameNode& labeled = model.node(first.children.front());
    require(labeled.labels.size() == 1 && labeled.labels.front().text == "A", "LB should parse as a label");
    require(labeled.marks.size() == 3, "CR/SQ/MA should parse as marks");
    const GameNode& marked = model.node(first.children.back());
    require(marked.marks.size() == 1 && marked.marks.front().shape == BoardMarkShape::Triangle, "TR should parse");

    const std::string written = writeSgf(model);
    require(written.find("XX[keep]") != std::string::npos, "writer should preserve unknown property");
    require(written.find("W[]") != std::string::npos, "writer should preserve pass move");
    require(written.find("AB[aa][bb]") != std::string::npos, "writer should preserve setup stones");
    require(written.find("BR[1d]") != std::string::npos, "writer should write black rank");
    require(written.find("WR[2d]") != std::string::npos, "writer should write white rank");
    require(written.find("RE[B+R]") != std::string::npos, "writer should write result");
    require(written.find("DT[2026-06-29]") != std::string::npos, "writer should write date");
    require(written.find("SO[unit-test]") != std::string::npos, "writer should write source");
    require(written.find("RU[Chinese]") != std::string::npos, "writer should write rules");
    require(written.find("HA[2]") != std::string::npos, "writer should write handicap");
    require(written.find("LB[dd:A]") != std::string::npos, "writer should preserve labels");
    require(written.find("CR[ee]") != std::string::npos, "writer should preserve circle marks");
    require(written.find("SQ[ff]") != std::string::npos, "writer should preserve square marks");
    require(written.find("MA[gg]") != std::string::npos, "writer should preserve cross marks");
    require(written.find("TR[cc]") != std::string::npos, "writer should preserve triangle marks");
    require(written.find("MN[12]") != std::string::npos, "writer should preserve move number");

    GameModel reparsed = parser.parseGame(written);
    require(reparsed.root().children.size() == 1, "written SGF should parse again");
    require(reparsed.node(reparsed.root().children.front()).children.size() == 2, "written SGF should preserve variations");

    model.gameInfo().blackPlayer.clear();
    model.gameInfo().whitePlayer.clear();
    model.gameInfo().blackRank.clear();
    model.gameInfo().whiteRank.clear();
    model.gameInfo().result.clear();
    model.gameInfo().date.clear();
    model.gameInfo().source.clear();
    model.gameInfo().rules.clear();
    model.gameInfo().komi.reset();
    model.gameInfo().handicap.reset();
    const std::string cleared = writeSgf(model);
    require(cleared.find("PB[") == std::string::npos, "writer should clear empty black player");
    require(cleared.find("PW[") == std::string::npos, "writer should clear empty white player");
    require(cleared.find("BR[") == std::string::npos, "writer should clear empty black rank");
    require(cleared.find("WR[") == std::string::npos, "writer should clear empty white rank");
    require(cleared.find("RE[") == std::string::npos, "writer should clear empty result");
    require(cleared.find("DT[") == std::string::npos, "writer should clear empty date");
    require(cleared.find("SO[") == std::string::npos, "writer should clear empty source");
    require(cleared.find("RU[") == std::string::npos, "writer should clear empty rules");
    require(cleared.find("KM[") == std::string::npos, "writer should clear missing komi");
    require(cleared.find("HA[") == std::string::npos, "writer should clear missing handicap");
}

void testSgfRejectsNonFiniteKomi()
{
    SgfParser parser;

    GameModel nanModel = parser.parseGame("(;GM[1]FF[4]SZ[9]KM[nan])");
    require(!nanModel.gameInfo().komi.has_value(), "SGF KM[nan] should not import as komi");

    GameModel infinityModel = parser.parseGame("(;GM[1]FF[4]SZ[9]KM[inf])");
    require(!infinityModel.gameInfo().komi.has_value(), "SGF KM[inf] should not import as komi");

    GameModel model(BoardSize::square(9));
    model.gameInfo().komi = std::numeric_limits<double>::infinity();
    const std::string written = writeSgf(model);
    require(written.find("KM[") == std::string::npos, "writer should omit non-finite komi");
}

void testSgfNumericPropertiesTrimWhitespace()
{
    SgfParser parser;
    GameModel model = parser.parseGame(
        "(;GM[1]FF[4]SZ[ 9 : 13 ]KM[ 6.5 ]HA[\t2\n]XX[  keep  ];B[dd]MN[ 12 ])");

    require(model.boardSize().width == 9 && model.boardSize().height == 13, "whitespace SZ should parse");
    require(model.gameInfo().komi.has_value() && *model.gameInfo().komi == 6.5, "whitespace KM should parse");
    require(model.gameInfo().handicap.has_value() && *model.gameInfo().handicap == 2, "whitespace HA should parse");
    require(model.root().properties.at("XX").front() == "  keep  ", "unknown text values should preserve whitespace");

    const GameNode& child = model.node(model.root().children.front());
    require(child.moveNumber.has_value() && *child.moveNumber == 12, "whitespace MN should parse");
    const std::string written = writeSgf(model);
    require(written.find("SZ[9:13]") != std::string::npos, "writer should canonicalize whitespace SZ");
    require(written.find("KM[6.5]") != std::string::npos, "writer should canonicalize whitespace KM");
    require(written.find("HA[2]") != std::string::npos, "writer should canonicalize whitespace HA");
    require(written.find("MN[12]") != std::string::npos, "writer should canonicalize whitespace MN");
    require(written.find("XX[  keep  ]") != std::string::npos, "writer should preserve unknown whitespace");
}

void testSgfRejectsInvalidIntegerProperties()
{
    SgfParser parser;
    GameModel imported = parser.parseGame("(;GM[1]FF[4]SZ[9]HA[-2];B[dd]MN[-5];W[ee]MN[0])");

    require(!imported.gameInfo().handicap.has_value(), "negative HA should not import as handicap");
    const GameNode& negativeMoveNumber = imported.node(imported.root().children.front());
    require(!negativeMoveNumber.moveNumber.has_value(), "negative MN should not import as move number");
    const GameNode& zeroMoveNumber = imported.node(negativeMoveNumber.children.front());
    require(zeroMoveNumber.moveNumber.has_value() && *zeroMoveNumber.moveNumber == 0, "MN[0] should import");

    const std::string rewritten = writeSgf(imported);
    require(rewritten.find("HA[-2]") == std::string::npos, "writer should remove imported negative HA");
    require(rewritten.find("MN[-5]") == std::string::npos, "writer should remove imported negative MN");
    require(rewritten.find("MN[0]") != std::string::npos, "writer should preserve zero MN");

    GameModel inMemory(BoardSize::square(9));
    inMemory.gameInfo().handicap = -2;
    const NodeId moveId = inMemory.addMove(Move::play(Color::Black, Point{3, 3}));
    inMemory.node(moveId).moveNumber = -5;

    const std::string written = writeSgf(inMemory);
    require(written.find("HA[") == std::string::npos, "writer should omit in-memory negative HA");
    require(written.find("MN[") == std::string::npos, "writer should omit in-memory negative MN");
}

void testSgfWriterCanonicalizesMixedCaseStandardProperties()
{
    const std::string sgf =
        "(;gm[0]GM[1]ff[3]FF[4]sz[5]SZ[9]pb[stale-black]PB[Old Black]"
        "pw[stale-white]PW[Old White]km[0]KM[6.5]ha[9]HA[2]"
        "AB[aa]ab[bb]AW[cc]aw[dd]AE[ee]ae[ff]LB[aa:A]lb[bb:B]"
        "CR[cc]cr[dd]MN[7]mn[8]PL[W]pl[B]C[root]c[stale-root]ZZ[keep]"
        ";b[aa]B[bb]w[cc]W[dd]pl[W]PL[B]C[child]c[stale-child]YY[keep])";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    model.gameInfo().blackPlayer = "New Black";
    model.gameInfo().whitePlayer.clear();
    model.gameInfo().komi.reset();
    model.gameInfo().handicap.reset();
    model.root().playerToMove.reset();
    model.root().labels.clear();
    model.root().marks.clear();
    model.root().moveNumber.reset();

    const NodeId childId = model.root().children.front();
    GameNode& child = model.node(childId);
    child.move.reset();
    child.playerToMove.reset();
    child.comment = "updated child";

    const std::string written = writeSgf(model);
    require(written.find("GM[1]") != std::string::npos, "writer should write canonical GM");
    require(written.find("FF[4]") != std::string::npos, "writer should write canonical FF");
    require(written.find("SZ[9]") != std::string::npos, "writer should write canonical SZ");
    require(written.find("PB[New Black]") != std::string::npos, "writer should write updated canonical PB");
    require(written.find("AB[aa]") != std::string::npos, "writer should preserve canonical setup stones");
    require(written.find("AE[ee]") != std::string::npos, "writer should preserve canonical setup clears");
    require(written.find("C[root]") != std::string::npos, "writer should preserve canonical root comments");
    require(written.find("C[updated child]") != std::string::npos, "writer should write updated child comments");
    require(written.find("ZZ[keep]") != std::string::npos, "writer should preserve unknown root properties");
    require(written.find("YY[keep]") != std::string::npos, "writer should preserve unknown child properties");

    require(written.find("gm[") == std::string::npos, "writer should remove mixed-case GM");
    require(written.find("ff[") == std::string::npos, "writer should remove mixed-case FF");
    require(written.find("sz[") == std::string::npos, "writer should remove mixed-case SZ");
    require(written.find("pb[") == std::string::npos, "writer should remove mixed-case PB");
    require(written.find("pw[") == std::string::npos, "writer should remove mixed-case PW");
    require(written.find("PW[") == std::string::npos, "writer should clear canonical PW and its variants");
    require(written.find("km[") == std::string::npos, "writer should remove mixed-case KM");
    require(written.find("KM[") == std::string::npos, "writer should clear canonical KM and its variants");
    require(written.find("ha[") == std::string::npos, "writer should remove mixed-case HA");
    require(written.find("HA[") == std::string::npos, "writer should clear canonical HA and its variants");
    require(written.find("ab[") == std::string::npos, "writer should remove mixed-case AB");
    require(written.find("aw[") == std::string::npos, "writer should remove mixed-case AW");
    require(written.find("ae[") == std::string::npos, "writer should remove mixed-case AE");
    require(written.find("lb[") == std::string::npos, "writer should remove mixed-case LB");
    require(written.find("LB[") == std::string::npos, "writer should clear canonical LB and its variants");
    require(written.find("cr[") == std::string::npos, "writer should remove mixed-case CR");
    require(written.find("CR[") == std::string::npos, "writer should clear canonical CR and its variants");
    require(written.find("mn[") == std::string::npos, "writer should remove mixed-case MN");
    require(written.find("MN[") == std::string::npos, "writer should clear canonical MN and its variants");
    require(written.find("pl[") == std::string::npos, "writer should remove mixed-case PL");
    require(written.find("PL[") == std::string::npos, "writer should clear canonical PL and its variants");
    require(written.find("b[aa]") == std::string::npos, "writer should remove mixed-case B moves");
    require(written.find("w[cc]") == std::string::npos, "writer should remove mixed-case W moves");
    require(written.find(";B[") == std::string::npos, "writer should clear canonical B moves");
    require(written.find(";W[") == std::string::npos, "writer should clear canonical W moves");
    require(written.find("c[stale") == std::string::npos, "writer should remove mixed-case C comments");
}

void testSgfParserAcceptsMixedCaseStandardProperties()
{
    const std::string sgf =
        "(;gm[1]ff[4]sz[9]pb[Black]pw[White]br[1d]wr[2d]re[B+R]dt[2026-06-30]"
        "so[test-source]ru[Japanese]km[6.5]ha[2]ab[aa]aw[bb]ae[cc]pl[w]"
        "lb[dd:A]cr[ee]sq[ff]tr[gg]ma[hh]mn[12]c[root note]zz[keep];w[]c[child note])";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    require(model.boardSize().width == 9 && model.boardSize().height == 9, "mixed-case SZ should parse");
    require(model.gameInfo().blackPlayer == "Black", "mixed-case PB should parse");
    require(model.gameInfo().whitePlayer == "White", "mixed-case PW should parse");
    require(model.gameInfo().blackRank == "1d", "mixed-case BR should parse");
    require(model.gameInfo().whiteRank == "2d", "mixed-case WR should parse");
    require(model.gameInfo().result == "B+R", "mixed-case RE should parse");
    require(model.gameInfo().date == "2026-06-30", "mixed-case DT should parse");
    require(model.gameInfo().source == "test-source", "mixed-case SO should parse");
    require(model.gameInfo().rules == "Japanese", "mixed-case RU should parse");
    require(model.gameInfo().komi == 6.5, "mixed-case KM should parse");
    require(model.gameInfo().handicap == 2, "mixed-case HA should parse");
    require(model.root().setupStones.black.size() == 1, "mixed-case AB should parse");
    require(model.root().setupStones.white.size() == 1, "mixed-case AW should parse");
    require(model.root().setupStones.empty.size() == 1, "mixed-case AE should parse");
    require(model.root().playerToMove == Color::White, "mixed-case PL should parse");
    require(model.root().labels.size() == 1, "mixed-case LB should parse");
    require(model.root().marks.size() == 4, "mixed-case mark properties should parse");
    require(model.root().moveNumber == 12, "mixed-case MN should parse");
    require(model.root().comment == "root note", "mixed-case C should parse");

    const NodeId childId = model.root().children.front();
    const GameNode& child = model.node(childId);
    require(child.move.has_value(), "mixed-case W should parse child moves");
    require(child.move->color == Color::White, "mixed-case W should preserve move color");
    require(child.move->type == MoveType::Pass, "mixed-case W pass should parse");
    require(child.comment == "child note", "mixed-case child C should parse");

    const std::string written = writeSgf(model);
    require(written.find("PB[Black]") != std::string::npos, "writer should canonicalize parsed mixed-case PB");
    require(written.find("W[]") != std::string::npos, "writer should canonicalize parsed mixed-case W");
    require(written.find("C[root note]") != std::string::npos, "writer should canonicalize parsed mixed-case C");
    require(written.find("zz[keep]") != std::string::npos, "writer should preserve unknown mixed-case properties");
    require(written.find("pb[") == std::string::npos, "writer should remove parsed mixed-case PB");
    require(written.find("w[]") == std::string::npos, "writer should remove parsed mixed-case W");
    require(written.find("c[root note]") == std::string::npos, "writer should remove parsed mixed-case C");
}

void testSgfCollectionParse()
{
    SgfParser parser;
    const std::vector<GameModel> games =
        parser.parseCollection("(;GM[1]FF[4]SZ[9]PB[First];B[aa])\n(;GM[1]FF[4]SZ[13]PB[Second];B[mm])");

    require(games.size() == 2, "parser should preserve every game in an SGF collection");
    require(
        games[0].boardSize().width == 9 && games[0].boardSize().height == 9,
        "first collection game should parse board size");
    require(games[0].gameInfo().blackPlayer == "First", "first collection game should parse game info");
    require(games[0].root().children.size() == 1, "first collection game should parse moves");
    require(
        games[1].boardSize().width == 13 && games[1].boardSize().height == 13,
        "second collection game should parse board size");
    require(games[1].gameInfo().blackPlayer == "Second", "second collection game should parse game info");
    const GameNode& secondMove = games[1].node(games[1].root().children.front());
    require(secondMove.move.has_value(), "second collection game should parse moves");
    require(secondMove.move->point == Point{12, 12}, "second collection game should preserve its own coordinates");

    const std::string written = writeSgfCollection(games);
    const std::vector<GameModel> restored = parser.parseCollection(written);
    require(restored.size() == 2, "collection writer should preserve every game");
    require(restored[0].gameInfo().blackPlayer == "First", "collection writer should preserve first game info");
    require(restored[1].gameInfo().blackPlayer == "Second", "collection writer should preserve second game info");
    require(
        restored[1].node(restored[1].root().children.front()).move->point == Point{12, 12},
        "collection writer should preserve second game moves");
}

void testSgfPlayerToMove()
{
    SgfParser parser;

    GameModel explicitRoot = parser.parseGame("(;GM[1]FF[4]SZ[9]PL[W])");
    require(explicitRoot.root().playerToMove == Color::White, "root PL should parse as player to move");
    require(explicitRoot.currentPosition().sideToMove() == Color::White, "root PL should set current side to move");
    require(writeSgf(explicitRoot).find("PL[W]") != std::string::npos, "writer should preserve root PL");

    GameModel explicitChild = parser.parseGame("(;GM[1]FF[4]SZ[9];B[dd]PL[B])");
    const NodeId childId = explicitChild.root().children.front();
    require(
        explicitChild.node(childId).playerToMove == Color::Black,
        "child PL should parse as player to move");
    require(
        explicitChild.positionAt(childId).sideToMove() == Color::Black,
        "child PL should override the move-derived player");
    require(writeSgf(explicitChild).find("PL[B]") != std::string::npos, "writer should preserve child PL");

    GameModel handicap = parser.parseGame("(;GM[1]FF[4]SZ[9]HA[2]AB[dd][fd])");
    require(handicap.currentPosition().sideToMove() == Color::White, "handicap root setup should leave white to move");
    require(
        writeSgf(handicap).find("PL[") == std::string::npos,
        "inferred handicap player should not add PL on write");
}

void testRectangularSgfRoundTrip()
{
    const std::string sgf = "(;GM[1]FF[4]SZ[9:13]KM[7.5]AB[ai][am];W[hm]C[wide board])";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    require(model.boardSize().width == 9 && model.boardSize().height == 13, "rectangular SGF size should parse");
    require(model.root().setupStones.black.size() == 2, "rectangular setup stones should parse");
    require(model.root().setupStones.black[0] == Point{0, 8}, "rectangular setup y coordinate should parse");
    require(model.root().setupStones.black[1] == Point{0, 12}, "rectangular lower edge should parse");

    const GameNode& moveNode = model.node(model.root().children.front());
    require(moveNode.move.has_value(), "rectangular child move should parse");
    require(moveNode.move->type == MoveType::Play, "rectangular child should be a play move");
    require(moveNode.move->point == Point{7, 12}, "rectangular move should preserve bottom row");

    const std::string written = writeSgf(model);
    require(written.find("SZ[9:13]") != std::string::npos, "writer should preserve rectangular size");
    require(written.find("AB[ai][am]") != std::string::npos, "writer should preserve rectangular setup points");
    require(written.find("W[hm]") != std::string::npos, "writer should preserve rectangular move");

    GameModel restored = parser.parseGame(written);
    require(restored.boardSize().width == 9 && restored.boardSize().height == 13, "rectangular write should parse again");
    const BoardPosition position = restored.positionAt(restored.root().children.front());
    require(position.at(Point{0, 8}) == Stone::Black, "rectangular restored setup stone should apply");
    require(position.at(Point{7, 12}) == Stone::White, "rectangular restored move should apply");
}

void testMaximumSgfCoordinatesRoundTrip()
{
    const BoardSize maximumSize{52, 52};
    require(maximumSize.isValid(), "52x52 should be the maximum valid SGF board size");
    require(pointFromSgf("ZZ", maximumSize) == Point{51, 51}, "uppercase SGF coordinates should reach 52x52 edge");
    require(pointToSgf(Point{51, 51}) == "ZZ", "maximum SGF point should encode as ZZ");

    const std::optional<Move> maximumMove = moveFromSgf(Color::Black, "ZZ", maximumSize);
    require(
        maximumMove.has_value() && maximumMove->type == MoveType::Play && maximumMove->point == Point{51, 51},
        "maximum SGF coordinate should parse as a play move on 52x52");

    const std::optional<Move> oldStylePass = moveFromSgf(Color::White, "ZZ", BoardSize::square(19));
    require(
        oldStylePass.has_value() && oldStylePass->type == MoveType::Pass,
        "maximum SGF coordinate outside a smaller board should remain compatible as pass");

    SgfParser parser;
    GameModel model = parser.parseGame("(;GM[1]FF[4]SZ[52:52]AB[ZZ];W[AZ])");
    require(model.boardSize().width == 52 && model.boardSize().height == 52, "52x52 SGF size should parse");
    require(model.root().setupStones.black.size() == 1, "52x52 setup stone should parse");
    require(model.root().setupStones.black.front() == Point{51, 51}, "52x52 setup stone should preserve ZZ");

    const NodeId moveId = model.root().children.front();
    require(model.node(moveId).move.has_value(), "52x52 child move should parse");
    require(model.node(moveId).move->point == Point{26, 51}, "52x52 child move should preserve uppercase x coordinate");

    const BoardPosition position = model.positionAt(moveId);
    require(position.at(Point{51, 51}) == Stone::Black, "52x52 setup stone should apply to board position");
    require(position.at(Point{26, 51}) == Stone::White, "52x52 move should apply to board position");

    const std::string written = writeSgf(model);
    require(written.find("SZ[52]") != std::string::npos, "square 52x52 writer should preserve board size");
    require(written.find("AB[ZZ]") != std::string::npos, "writer should preserve maximum setup coordinate");
    require(written.find("W[AZ]") != std::string::npos, "writer should preserve maximum-row move coordinate");
}

void testClearedSetupPropertiesDoNotPersist()
{
    SgfParser parser;
    GameModel model = parser.parseGame("(;GM[1]FF[4]SZ[9]AB[aa][bb]AW[cc]AE[dd]ZZ[keep])");
    require(model.root().setupStones.black.size() == 2, "test SGF should parse setup black stones");
    require(model.root().setupStones.white.size() == 1, "test SGF should parse setup white stones");
    require(model.root().setupStones.empty.size() == 1, "test SGF should parse setup clear points");

    model.root().setupStones.black.clear();
    model.root().setupStones.white.clear();
    model.root().setupStones.empty.clear();
    const std::string written = writeSgf(model);
    require(written.find("AB[") == std::string::npos, "writer should remove stale AB when setup black is empty");
    require(written.find("AW[") == std::string::npos, "writer should remove stale AW when setup white is empty");
    require(written.find("AE[") == std::string::npos, "writer should remove stale AE when setup clear is empty");
    require(written.find("ZZ[keep]") != std::string::npos, "writer should preserve unrelated unknown properties");
}

void testConflictingSetupPropertiesAreCanonicalized()
{
    SgfParser parser;
    GameModel model = parser.parseGame("(;GM[1]FF[4]SZ[5]AB[aa][bb][aa]AW[bb][cc]AE[cc][dd]ZZ[keep])");

    require(model.root().setupStones.black.size() == 1, "conflicting setup should leave one black stone");
    require(model.root().setupStones.black.front() == Point{0, 0}, "black setup should keep the non-conflicting point");
    require(model.root().setupStones.white.size() == 1, "white setup should override conflicting black setup");
    require(model.root().setupStones.white.front() == Point{1, 1}, "white setup should keep the overwritten point");
    require(model.root().setupStones.empty.size() == 2, "AE setup should override stones and deduplicate points");
    require(model.root().setupStones.empty[0] == Point{2, 2}, "AE setup should clear the former white point");
    require(model.root().setupStones.empty[1] == Point{3, 3}, "AE setup should preserve explicit empty points");

    const BoardPosition position = model.currentPosition();
    require(position.at(Point{0, 0}) == Stone::Black, "canonicalized black setup should apply to position");
    require(position.at(Point{1, 1}) == Stone::White, "canonicalized white setup should apply to position");
    require(position.at(Point{2, 2}) == Stone::Empty, "canonicalized AE should clear conflicting white setup");
    require(position.at(Point{3, 3}) == Stone::Empty, "canonicalized AE should keep explicit empty points clear");

    const std::string written = writeSgf(model);
    require(written.find("AB[aa]") != std::string::npos, "canonicalized SGF should write black setup once");
    require(written.find("AW[bb]") != std::string::npos, "canonicalized SGF should write white setup once");
    require(written.find("AE[cc][dd]") != std::string::npos, "canonicalized SGF should write AE setup once");
    require(written.find("ZZ[keep]") != std::string::npos, "canonicalized SGF should preserve unrelated properties");
    require(countOccurrences(written, "[aa]") == 1, "canonicalized SGF should not duplicate black setup");
    require(countOccurrences(written, "[bb]") == 1, "canonicalized SGF should not keep stale black setup");
    require(countOccurrences(written, "[cc]") == 1, "canonicalized SGF should not keep stale white setup");
}

void testConflictingMarkupPropertiesAreCanonicalizedOnWrite()
{
    SgfParser parser;
    GameModel model = parser.parseGame(
        "(;GM[1]FF[4]SZ[5]LB[aa:A][bb:B][aa:C]CR[cc][dd]SQ[cc]TR[ee]MA[dd]ZZ[keep])");

    require(model.root().labels.size() == 3, "test SGF should parse duplicate labels before write canonicalization");
    require(
        model.root().marks.size() == 5,
        "test SGF should parse duplicate/conflicting marks before write canonicalization");

    const std::string written = writeSgf(model);
    require(written.find("[aa:A]") == std::string::npos, "canonicalized markup should remove stale labels");
    require(written.find("[aa:C]") != std::string::npos, "canonicalized markup should keep the last label text");
    require(written.find("[bb:B]") != std::string::npos, "canonicalized markup should preserve unrelated labels");
    require(countOccurrences(written, "aa:") == 1, "canonicalized markup should write one label per point");
    require(written.find("CR[cc]") == std::string::npos, "canonicalized markup should remove stale circle marks");
    require(written.find("CR[dd]") == std::string::npos, "canonicalized markup should remove overwritten circle marks");
    require(written.find("SQ[cc]") != std::string::npos, "canonicalized markup should keep the last square mark");
    require(written.find("TR[ee]") != std::string::npos, "canonicalized markup should preserve unrelated triangle marks");
    require(written.find("MA[dd]") != std::string::npos, "canonicalized markup should keep the last cross mark");
    require(written.find("ZZ[keep]") != std::string::npos, "canonicalized markup should preserve unrelated properties");

    const GameModel restored = parser.parseGame(written);
    const auto hasLabel = [&restored](Point point, const std::string& text) {
        return std::find_if(
                   restored.root().labels.begin(),
                   restored.root().labels.end(),
                   [point, &text](const BoardLabel& label) {
                       return label.point == point && label.text == text;
                   }) != restored.root().labels.end();
    };
    const auto hasMark = [&restored](Point point, BoardMarkShape shape) {
        return std::find_if(
                   restored.root().marks.begin(),
                   restored.root().marks.end(),
                   [point, shape](const BoardMark& mark) {
                       return mark.point == point && mark.shape == shape;
                   }) != restored.root().marks.end();
    };
    require(restored.root().labels.size() == 2, "canonicalized markup should round-trip one label per point");
    require(hasLabel(Point{0, 0}, "C"), "canonicalized markup should round-trip the last duplicate label");
    require(hasLabel(Point{1, 1}, "B"), "canonicalized markup should round-trip unrelated labels");
    require(restored.root().marks.size() == 3, "canonicalized markup should round-trip one mark per point");
    require(
        hasMark(Point{2, 2}, BoardMarkShape::Square),
        "canonicalized markup should round-trip overwritten square marks");
    require(hasMark(Point{3, 3}, BoardMarkShape::Cross), "canonicalized markup should round-trip overwritten cross marks");
    require(hasMark(Point{4, 4}, BoardMarkShape::Triangle), "canonicalized markup should round-trip unrelated marks");
}

void testClearedMarkupPropertiesDoNotPersist()
{
    SgfParser parser;
    GameModel model = parser.parseGame("(;GM[1]FF[4]SZ[9];B[dd]MN[42]LB[aa:A]CR[bb]SQ[cc]TR[ee]MA[ff]ZZ[keep])");
    const NodeId childId = model.root().children.front();
    GameNode& child = model.node(childId);
    require(child.moveNumber.has_value(), "test SGF should parse move number");
    require(!child.labels.empty(), "test SGF should parse labels");
    require(child.marks.size() == 4, "test SGF should parse all mark shapes");

    child.moveNumber.reset();
    child.labels.clear();
    child.marks.clear();
    const std::string written = writeSgf(model);
    require(written.find("MN[") == std::string::npos, "writer should remove stale MN when move number is empty");
    require(written.find("LB[") == std::string::npos, "writer should remove stale LB when labels are empty");
    require(written.find("CR[") == std::string::npos, "writer should remove stale CR when circle marks are empty");
    require(written.find("SQ[") == std::string::npos, "writer should remove stale SQ when square marks are empty");
    require(written.find("TR[") == std::string::npos, "writer should remove stale TR when triangle marks are empty");
    require(written.find("MA[") == std::string::npos, "writer should remove stale MA when cross marks are empty");
    require(written.find("ZZ[keep]") != std::string::npos, "writer should preserve unrelated unknown child properties");
}

void testClearedMovePropertiesDoNotPersist()
{
    SgfParser parser;
    GameModel model = parser.parseGame("(;GM[1]FF[4]SZ[9];B[dd]W[ee]ZZ[keep])");
    const NodeId childId = model.root().children.front();
    GameNode& child = model.node(childId);
    require(child.move.has_value(), "test SGF should parse a move");

    child.move.reset();
    const std::string written = writeSgf(model);
    require(written.find("B[") == std::string::npos, "writer should remove stale B when move is empty");
    require(written.find("W[") == std::string::npos, "writer should remove stale W when move is empty");
    require(written.find("ZZ[keep]") != std::string::npos, "writer should preserve unrelated unknown move-node properties");
}

void testClearedAnalysisPropertiesDoNotPersist()
{
    SgfParser parser;
    GameModel model = parser.parseGame(
        "(;GM[1]FF[4]SZ[9]lzop[old-root]Lz[wrong-root]lzyerr[old-root-error]lzyPersp[W]ZZ[keep-root];"
        "B[dd]lz[old-child]LzOp[wrong-child]LzyErr[old-child-error]LZYpersp[B]YY[keep-child])");
    const NodeId childId = model.root().children.front();

    model.root().analysis = AnalysisSnapshot{};
    model.node(childId).analysis = AnalysisSnapshot{};
    require(model.root().properties.contains("lzop"), "test SGF should retain mixed-case root LZOP property");
    require(model.node(childId).properties.contains("lz"), "test SGF should retain mixed-case child LZ property");

    require(model.clearAnalysisFrom(model.rootId()), "analysis clear should accept the root subtree");
    const std::string cleared = writeSgf(model);
    require(cleared.find("LZOP[") == std::string::npos, "cleared analysis should remove stale LZOP properties");
    require(cleared.find("LZ[") == std::string::npos, "cleared analysis should remove stale LZ properties");
    require(cleared.find("LZYERR[") == std::string::npos, "cleared analysis should remove stale error properties");
    require(cleared.find("LZYPERSP[") == std::string::npos, "cleared analysis should remove stale perspective properties");
    require(
        cleared.find("lzop[") == std::string::npos && cleared.find("LzOp[") == std::string::npos,
        "cleared analysis should remove mixed-case LZOP properties");
    require(
        cleared.find("lz[") == std::string::npos && cleared.find("Lz[") == std::string::npos,
        "cleared analysis should remove mixed-case LZ properties");
    require(
        cleared.find("lzyerr[") == std::string::npos && cleared.find("LzyErr[") == std::string::npos,
        "cleared analysis should remove mixed-case error properties");
    require(
        cleared.find("lzyPersp[") == std::string::npos && cleared.find("LZYpersp[") == std::string::npos,
        "cleared analysis should remove mixed-case perspective properties");
    require(cleared.find("ZZ[keep-root]") != std::string::npos, "analysis clear should preserve unrelated root properties");
    require(cleared.find("YY[keep-child]") != std::string::npos, "analysis clear should preserve unrelated child properties");
}

void testWrittenAnalysisRemovesOppositeLegacyProperties()
{
    SgfParser parser;
    GameModel model = parser.parseGame("(;GM[1]FF[4]SZ[9]LZ[wrong-root];B[dd]LZOP[wrong-child])");
    const NodeId childId = model.root().children.front();

    AnalysisSnapshot rootAnalysis;
    rootAnalysis.rootWinrate = 0.51;
    rootAnalysis.rootScoreMean = 1.25;
    rootAnalysis.rootVisits = 7;
    model.root().analysis = rootAnalysis;

    AnalysisSnapshot childAnalysis;
    childAnalysis.rootWinrate = 0.49;
    childAnalysis.rootScoreMean = -0.75;
    childAnalysis.rootVisits = 9;
    childAnalysis.winratePerspective = Color::White;
    model.node(childId).analysis = childAnalysis;

    const std::string written = writeSgf(model);
    require(written.find("LZOP[LizzieYzyQt 49.0 7 1.25") != std::string::npos,
            "root analysis should write root LZOP");
    require(written.find("LZ[LizzieYzyQt 51.0 9 -0.75") != std::string::npos,
            "child analysis should write child LZ");
    require(written.find("LZYPERSP[B]") != std::string::npos, "root analysis should record black winrate perspective");
    require(written.find("LZYPERSP[W]") != std::string::npos, "child analysis should record white winrate perspective");
    require(written.find("LZ[wrong-root]") == std::string::npos, "root analysis should remove stale root LZ");
    require(written.find("LZOP[wrong-child]") == std::string::npos, "child analysis should remove stale child LZOP");
}

void testSuccessfulAnalysisSgfWriteClearsStaleErrorData()
{
    SgfParser parser;
    GameModel model = parser.parseGame(
        "(;GM[1]FF[4]SZ[9]C[root note\n\nLizzieYzy analysis error\nstale root failure]"
        "LZYERR[stale root failure]LZOP[stale-root-success]LZYPERSP[W];B[dd]"
        "C[user note\n\nLizzieYzy analysis error\nstale child failure]"
        "LZYERR[stale child failure]LZ[stale-child-success]LZYPERSP[B])");
    const NodeId childId = model.root().children.front();

    AnalysisSnapshot rootAnalysis;
    rootAnalysis.rootWinrate = 0.54;
    rootAnalysis.rootScoreMean = -0.25;
    rootAnalysis.rootVisits = 23;

    AnalysisSnapshot analysis;
    analysis.rootWinrate = 0.58;
    analysis.rootScoreMean = 1.5;
    analysis.rootVisits = 17;
    analysis.winratePerspective = Color::White;
    analysis.candidates.push_back(MoveCandidate{Move::play(Color::White, Point{4, 4}), 17, 0.58, 1.5});

    require(model.root().analysisError == "stale root failure", "test SGF should import the stale root error");
    model.root().analysisError.clear();
    model.root().analysis = rootAnalysis;

    GameNode& child = model.node(childId);
    require(child.analysisError == "stale child failure", "test SGF should import the stale child error");
    child.analysisError.clear();
    child.analysis = analysis;

    const std::string written = writeSgf(model);
    require(written.find("LZOP[LizzieYzyQt 46.0 23 -0.25") != std::string::npos,
            "successful root analysis should write canonical LZOP data");
    require(written.find("LZ[LizzieYzyQt 42.0 17 1.50") != std::string::npos,
            "successful child analysis should write canonical LZ data");
    require(written.find("LZYPERSP[B]") != std::string::npos, "successful root analysis should write fresh perspective");
    require(written.find("LZYPERSP[W]") != std::string::npos, "successful child analysis should write fresh perspective");
    require(written.find("LZYERR[") == std::string::npos, "successful analysis should remove stale structured errors");
    require(
        written.find("LizzieYzy analysis error") == std::string::npos,
        "successful analysis should remove stale generated error comments");
    require(written.find("stale root failure") == std::string::npos, "successful analysis should remove stale root error text");
    require(written.find("stale child failure") == std::string::npos, "successful analysis should remove stale child error text");
    require(
        written.find("LZOP[stale-root-success]") == std::string::npos,
        "successful analysis should replace stale root LZOP data");
    require(
        written.find("LZ[stale-child-success]") == std::string::npos,
        "successful analysis should replace stale child LZ data");
    require(
        written.find("C[root note\n\nLizzieYzy analysis\n") != std::string::npos,
        "successful root analysis should preserve user comments and append success diagnostics");
    require(
        written.find("C[user note\n\nLizzieYzy analysis\n") != std::string::npos,
        "successful child analysis should preserve user comments and append success diagnostics");

    GameModel restored = parser.parseGame(written);
    const NodeId restoredChildId = restored.root().children.front();
    require(restored.root().analysisError.empty(), "rewritten successful root analysis should not round-trip a stale error");
    require(
        restored.node(restoredChildId).analysisError.empty(),
        "rewritten successful child analysis should not round-trip a stale error");
}

void testAnalysisSgfWriteRejectsNonFiniteValues()
{
    GameModel model(BoardSize::square(9));
    const double infinity = std::numeric_limits<double>::infinity();
    const double quietNaN = std::numeric_limits<double>::quiet_NaN();

    AnalysisSnapshot analysis;
    analysis.rootVisits = 15;
    analysis.rootWinrate = infinity;
    analysis.rootScoreMean = -infinity;
    analysis.ownership.assign(static_cast<std::size_t>(model.boardSize().pointCount()), 0.1);
    analysis.ownership[3] = infinity;

    MoveCandidate candidate{Move::play(Color::Black, Point{3, 3}), 15, infinity, quietNaN};
    candidate.scoreStdev = infinity;
    candidate.policy = infinity;
    candidate.ownership.assign(static_cast<std::size_t>(model.boardSize().pointCount()), -0.1);
    candidate.ownership[5] = -infinity;
    analysis.candidates.push_back(candidate);
    model.root().analysis = analysis;

    const std::string written = writeSgf(model);
    require(written.find("inf") == std::string::npos, "SGF analysis export should not write infinity");
    require(written.find("nan") == std::string::npos, "SGF analysis export should not write NaN");
    require(
        written.find("scoreMean 0.00 scoreStdev 0.00") != std::string::npos,
        "SGF analysis export should sanitize non-finite candidate scores");
    require(written.find("movesOwnership") == std::string::npos, "SGF analysis export should drop bad ownership");
    require(written.find(" ownership") == std::string::npos, "SGF analysis export should drop bad root ownership");
    require(
        written.find("Ownership:") == std::string::npos,
        "readable analysis comment should drop bad root ownership");
}

void testAnalysisSgfWriteClampsNegativeVisits()
{
    GameModel model(BoardSize::square(9));

    AnalysisSnapshot analysis;
    analysis.rootVisits = -12;
    analysis.rootWinrate = 0.5;
    analysis.rootScoreMean = 0.0;

    MoveCandidate candidate{Move::play(Color::Black, Point{3, 3}), -7, 0.5, 0.0};
    candidate.pv = {Move::play(Color::Black, Point{3, 3})};
    candidate.pvVisits = {-3, 5};
    analysis.candidates.push_back(candidate);
    model.root().analysis = analysis;

    const std::string written = writeSgf(model);
    require(written.find("LizzieYzyQt 50.0 0 0.00") != std::string::npos, "SGF LZOP should clamp root visits");
    require(written.find("move D6 visits 0") != std::string::npos, "SGF LZOP should clamp candidate visits");
    require(written.find("pvVisits 0 5") != std::string::npos, "SGF LZOP should clamp PV visits");
    require(written.find("Visits: 0") != std::string::npos, "readable analysis comment should clamp root visits");
    require(
        written.find("D6 visits 0 winrate") != std::string::npos,
        "readable analysis comment should clamp candidate visits");
    require(written.find("visits -") == std::string::npos, "SGF analysis export should not write negative visits");
    require(written.find("Visits: -") == std::string::npos, "readable analysis export should not write negative visits");
}

void testReadableAnalysisCommentClampsRates()
{
    GameModel model(BoardSize::square(9));
    AnalysisSnapshot analysis;
    analysis.rootVisits = 12;
    analysis.rootWinrate = 1.25;
    analysis.rootScoreMean = 0.0;

    MoveCandidate candidate{Move::play(Color::Black, Point{3, 3}), 12, -0.4, 0.0};
    candidate.policy = 1.5;
    analysis.candidates.push_back(candidate);
    model.root().analysis = analysis;

    const std::string written = writeSgf(model);
    require(written.find("Winrate: 100.0%") != std::string::npos, "readable root winrate should clamp to 100%");
    require(
        written.find("D6 visits 12 winrate 0.0% score 0.00 stdev 0.00 policy 100.0%") != std::string::npos,
        "readable candidate winrate and policy should clamp to 0..100%");
    require(written.find("Winrate: 125.0%") == std::string::npos, "readable root winrate should not exceed 100%");
    require(written.find("winrate -40.0%") == std::string::npos, "readable candidate winrate should not be negative");
    require(written.find("policy 150.0%") == std::string::npos, "readable candidate policy should not exceed 100%");
}

void testReadableAnalysisCommentWritesCompleteOwnership()
{
    GameModel model(BoardSize::square(3));
    AnalysisSnapshot analysis;
    analysis.rootVisits = 8;
    analysis.rootWinrate = 0.52;
    analysis.rootScoreMean = 0.3;
    analysis.ownership = {0.25, -0.25, 0.15, -0.15, 0.05, -0.05, 0.4, -0.4, 0.0};

    MoveCandidate candidate{Move::play(Color::Black, Point{1, 1}), 8, 0.52, 0.3};
    candidate.ownership = {0.35, -0.35, 0.2, -0.2, 0.1, -0.1, 0.05, -0.05, 0.0};
    analysis.candidates.push_back(candidate);
    model.root().analysis = analysis;

    const std::string written = writeSgf(model);
    require(
        written.find("Ownership: 0.250000 -0.250000") != std::string::npos,
        "readable analysis comment should write complete root ownership");
    require(
        written.find("B2 visits 8 winrate 52.0% score 0.30 stdev 0.00 policy 0.0% movesOwnership "
                     "0.350000 -0.350000") != std::string::npos,
        "readable analysis comment should write complete candidate ownership");

    model.root().analysis->ownership = {0.25, -0.25};
    model.root().analysis->candidates.front().ownership = {0.35, -0.35};
    const std::string incomplete = writeSgf(model);
    require(
        incomplete.find("Ownership:") == std::string::npos,
        "readable analysis comment should filter incomplete root ownership");
    require(
        incomplete.find("policy 0.0% movesOwnership") == std::string::npos,
        "readable analysis comment should filter incomplete candidate ownership");
}

void testWideBoardAnalysisExportUsesSgfCoordinatesWhenGtpCannotRepresentMoves()
{
    GameModel model(BoardSize{26, 9});
    AnalysisSnapshot analysis;
    analysis.rootVisits = 21;
    analysis.rootWinrate = 0.54;
    analysis.rootScoreMean = 0.7;
    MoveCandidate candidate{Move::play(Color::Black, Point{25, 0}), 21, 0.54, 0.7};
    candidate.pv = {
        Move::play(Color::Black, Point{25, 0}),
        Move::play(Color::White, Point{0, 8}),
        Move::pass(Color::Black),
    };
    analysis.candidates.push_back(candidate);
    model.root().analysis = analysis;

    const std::string written = writeSgf(model);
    require(
        written.find("move za visits 21") != std::string::npos,
        "wide-board LZOP should write SGF coordinates when GTP cannot represent the move");
    require(
        written.find("pv za ai pass") != std::string::npos,
        "wide-board LZOP should write SGF coordinates in PVs when GTP cannot represent them");
    require(
        written.find("\nza visits 21") != std::string::npos,
        "wide-board generated analysis comment should write readable SGF coordinates");
    require(written.find("move  visits") == std::string::npos, "wide-board LZOP should not write empty move tokens");
    require(written.find("pv  ") == std::string::npos, "wide-board LZOP should not write empty PV tokens");
}

void testAnalysisErrorSgfRoundTrip()
{
    SgfParser parser;
    GameModel model = parser.parseGame(
        "(;GM[1]FF[4]SZ[9]C[root note];B[dd]C[user note]LZ[stale-success]LZOP[stale-wrong]LZYPERSP[W])");
    const NodeId childId = model.root().children.front();
    AnalysisSnapshot staleSuccess;
    staleSuccess.rootVisits = 12;
    staleSuccess.rootWinrate = 0.62;
    staleSuccess.rootScoreMean = 1.5;
    model.node(childId).analysis = staleSuccess;
    model.node(childId).analysisError = "analysis request failed: fake node\nraw: {\"id\":\"node:2\"}";

    const std::string written = writeSgf(model);
    require(
        written.find("LZYERR[analysis request failed: fake node\nraw: {\"id\":\"node:2\"}]") != std::string::npos,
        "writer should preserve analysis errors in a structured SGF property");
    require(written.find("C[user note\n\nLizzieYzy analysis error\n") != std::string::npos,
            "writer should append human-readable analysis errors to comments");
    require(written.find("LZ[stale-success]") == std::string::npos, "analysis error should remove stale LZ data");
    require(written.find("LZOP[stale-wrong]") == std::string::npos, "analysis error should remove stale LZOP data");
    require(written.find("LZYPERSP[") == std::string::npos, "analysis error should remove stale perspective data");
    require(
        written.find("LizzieYzy analysis\n") == std::string::npos,
        "analysis error should not also write a generated success analysis comment");
    require(
        written.find("LizzieYzyQt") == std::string::npos,
        "analysis error should not also write generated success analysis data");

    GameModel restored = parser.parseGame(written);
    const NodeId restoredChildId = restored.root().children.front();
    require(
        restored.node(restoredChildId).analysisError == model.node(childId).analysisError,
        "parser should restore structured analysis errors");

    GameModel mixedCaseRestored =
        parser.parseGame("(;GM[1]FF[4]SZ[9];B[dd]lzyErr[mixed-case analysis failure]ZZ[keep])");
    const NodeId mixedCaseChildId = mixedCaseRestored.root().children.front();
    require(
        mixedCaseRestored.node(mixedCaseChildId).analysisError == "mixed-case analysis failure",
        "parser should restore mixed-case structured analysis errors");
    const std::string mixedCaseWritten = writeSgf(mixedCaseRestored);
    require(
        mixedCaseWritten.find("LZYERR[mixed-case analysis failure]") != std::string::npos,
        "writer should canonicalize mixed-case structured analysis errors");
    require(
        mixedCaseWritten.find("lzyErr[") == std::string::npos,
        "writer should remove stale mixed-case structured analysis errors");
    require(
        mixedCaseWritten.find("ZZ[keep]") != std::string::npos,
        "writer should preserve unrelated properties when canonicalizing analysis errors");

    const std::string rewritten = writeSgf(restored);
    require(
        countOccurrences(rewritten, "LizzieYzy analysis error") == 1,
        "rewriting an analysis error should not duplicate the generated comment");
    require(
        countOccurrences(rewritten, "LZYERR[analysis request failed: fake node") == 1,
        "rewriting an analysis error should not duplicate the structured property");

    require(restored.clearAnalysisFrom(restoredChildId), "analysis clear should accept the error node");
    const std::string cleared = writeSgf(restored);
    require(cleared.find("LZYERR[") == std::string::npos, "cleared analysis should remove stale error properties");
    require(
        cleared.find("LizzieYzy analysis error") == std::string::npos,
        "cleared analysis should remove generated error comments");
    require(cleared.find("C[user note]") != std::string::npos, "cleared analysis should preserve user comments");
}

void testSgfPointRanges()
{
    SgfParser parser;
    GameModel model =
        parser.parseGame("(;GM[1]FF[4]SZ[5]AB[aa:bb]AW[ca:da]AE[ec:ed]CR[aa:bb]SQ[be:ce]TR[cc:dd]MA[da:db])");

    require(model.root().setupStones.black.size() == 4, "AB range should expand to four points");
    require(model.root().setupStones.white.size() == 2, "AW range should expand to two points");
    require(model.root().setupStones.empty.size() == 2, "AE range should expand to two points");
    const auto countMarks = [&](BoardMarkShape shape) {
        int count = 0;
        for (const BoardMark& mark : model.root().marks) {
            if (mark.shape == shape) {
                ++count;
            }
        }
        return count;
    };
    require(model.root().marks.size() == 12, "markup ranges should expand to marks");
    require(countMarks(BoardMarkShape::Circle) == 4, "CR range should expand to four circle marks");
    require(countMarks(BoardMarkShape::Square) == 2, "SQ range should expand to two square marks");
    require(countMarks(BoardMarkShape::Triangle) == 4, "TR range should expand to four triangle marks");
    require(countMarks(BoardMarkShape::Cross) == 2, "MA range should expand to two cross marks");

    const BoardPosition position = model.currentPosition();
    require(position.at(Point{0, 0}) == Stone::Black, "AB range top-left should apply");
    require(position.at(Point{1, 1}) == Stone::Black, "AB range bottom-right should apply");
    require(position.at(Point{2, 0}) == Stone::White, "AW range should apply");
    require(position.at(Point{4, 2}) == Stone::Empty, "AE range should leave point empty");

    const std::string written = writeSgf(model);
    require(written.find("AB[aa][ba][ab][bb]") != std::string::npos, "writer should preserve expanded AB range points");
    require(written.find("AW[ca][da]") != std::string::npos, "writer should preserve expanded AW range points");
    require(written.find("AE[ec][ed]") != std::string::npos, "writer should preserve expanded AE range points");
    require(written.find("CR[aa][ba][ab][bb]") != std::string::npos, "writer should preserve expanded circle range");
    require(written.find("SQ[be][ce]") != std::string::npos, "writer should preserve expanded square range");
    require(written.find("TR[cc][dc][cd][dd]") != std::string::npos, "writer should preserve expanded triangle range");
    require(written.find("MA[da][db]") != std::string::npos, "writer should preserve expanded cross range");
}

void testEscapedSgfValuesPreserve()
{
    const std::string sgf =
        R"((;GM[1]FF[4]SZ[9]C[root \] bracket \\ slash]LB[ee:A\]B\\C]XX[unknown \] value \\ keep][second \] value];B[dd]YY[child \] value \\ keep]))";

    SgfParser parser;
    GameModel model = parser.parseGame(sgf);
    require(model.root().comment == "root ] bracket \\ slash", "escaped root comment should parse");
    require(model.root().labels.size() == 1, "escaped label should parse");
    require(model.root().labels.front().point == Point{4, 4}, "escaped label point should parse");
    require(model.root().labels.front().text == "A]B\\C", "escaped label text should parse");
    require(model.root().properties.at("XX").size() == 2, "multi-value unknown root property should parse");
    require(model.root().properties.at("XX").front() == "unknown ] value \\ keep", "escaped unknown root value should parse");
    require(model.root().properties.at("XX").back() == "second ] value", "second escaped unknown root value should parse");

    const GameNode& child = model.node(model.root().children.front());
    require(child.properties.at("YY").front() == "child ] value \\ keep", "escaped unknown child value should parse");

    const std::string written = writeSgf(model);
    require(
        written.find(R"(C[root \] bracket \\ slash])") != std::string::npos,
        "writer should re-escape comment brackets and backslashes");
    require(
        written.find(R"(LB[ee:A\]B\\C])") != std::string::npos,
        "writer should re-escape label text");
    require(
        written.find(R"(XX[unknown \] value \\ keep])") != std::string::npos,
        "writer should re-escape unknown root properties");
    require(
        written.find(R"(XX[unknown \] value \\ keep][second \] value])") != std::string::npos,
        "writer should preserve multi-value unknown properties");
    require(
        written.find(R"(YY[child \] value \\ keep])") != std::string::npos,
        "writer should preserve and re-escape unknown child properties");

    GameModel restored = parser.parseGame(written);
    require(restored.root().comment == model.root().comment, "escaped comment should round-trip");
    require(restored.root().labels.size() == 1, "escaped label should round-trip");
    require(restored.root().labels.front().text == model.root().labels.front().text, "escaped label text should round-trip");
    require(
        restored.root().properties.at("XX").front() == model.root().properties.at("XX").front(),
        "escaped unknown root property should round-trip");
    require(
        restored.root().properties.at("XX").back() == model.root().properties.at("XX").back(),
        "second escaped unknown root property should round-trip");
    const GameNode& restoredChild = restored.node(restored.root().children.front());
    require(
        restoredChild.properties.at("YY").front() == child.properties.at("YY").front(),
        "escaped unknown child property should round-trip");
}

void testSgfWriterOmitsInvalidPropertyNames()
{
    GameModel model(BoardSize::square(9));
    model.root().properties["ZZ"] = {"keep"};
    model.root().properties["a1"] = {"compat"};
    model.root().properties[""] = {"empty-name"};
    model.root().properties["1A"] = {"leading-digit"};
    model.root().properties["BAD-NAME"] = {"dash-name"};
    model.root().properties["BAD NAME"] = {"space-name"};
    model.root().properties["BAD]NAME"] = {"bracket-name"};

    const NodeId childId = model.addMove(Move::play(Color::Black, Point{3, 3}));
    model.node(childId).properties["yy"] = {"child-keep"};
    model.node(childId).properties["bad name"] = {"child-space-name"};

    const std::string written = writeSgf(model);
    require(written.find("ZZ[keep]") != std::string::npos, "writer should preserve valid unknown property names");
    require(written.find("a1[compat]") != std::string::npos, "writer should preserve parser-compatible property names");
    require(written.find("yy[child-keep]") != std::string::npos, "writer should preserve valid child properties");
    require(written.find("empty-name") == std::string::npos, "writer should omit empty property names");
    require(written.find("1A[leading-digit]") == std::string::npos, "writer should omit leading-digit property names");
    require(written.find("BAD-NAME") == std::string::npos, "writer should omit dashed property names");
    require(written.find("BAD NAME") == std::string::npos, "writer should omit spaced property names");
    require(written.find("BAD]NAME") == std::string::npos, "writer should omit bracketed property names");
    require(written.find("bad name") == std::string::npos, "writer should omit invalid child property names");

    SgfParser parser;
    GameModel restored = parser.parseGame(written);
    require(restored.root().properties.at("ZZ").front() == "keep", "valid unknown property should round-trip");
    require(restored.root().properties.at("a1").front() == "compat", "compatible unknown property should round-trip");
    const GameNode& restoredChild = restored.node(restored.root().children.front());
    require(restoredChild.properties.at("yy").front() == "child-keep", "valid child property should round-trip");
}

}  // namespace

int main()
{
    try {
        testCoordinates();
        testCaptures();
        testSuicide();
        testCaptureIsNotSuicide();
        testKo();
        testPositionHash();
        testGameModelPosition();
        testGameModelPositionReportsInvalidSetup();
        testGameModelTryAddMoveValidatesLocalRules();
        testBranchEditing();
        testAnalysisInvalidation();
        testResignRoundTrip();
        testSgfRoundTrip();
        testSgfRejectsNonFiniteKomi();
        testSgfNumericPropertiesTrimWhitespace();
        testSgfRejectsInvalidIntegerProperties();
        testSgfWriterCanonicalizesMixedCaseStandardProperties();
        testSgfParserAcceptsMixedCaseStandardProperties();
        testSgfCollectionParse();
        testSgfPlayerToMove();
        testRectangularSgfRoundTrip();
        testMaximumSgfCoordinatesRoundTrip();
        testClearedSetupPropertiesDoNotPersist();
        testConflictingSetupPropertiesAreCanonicalized();
        testConflictingMarkupPropertiesAreCanonicalizedOnWrite();
        testClearedMarkupPropertiesDoNotPersist();
        testClearedMovePropertiesDoNotPersist();
        testClearedAnalysisPropertiesDoNotPersist();
        testWrittenAnalysisRemovesOppositeLegacyProperties();
        testSuccessfulAnalysisSgfWriteClearsStaleErrorData();
        testAnalysisSgfWriteRejectsNonFiniteValues();
        testAnalysisSgfWriteClampsNegativeVisits();
        testReadableAnalysisCommentClampsRates();
        testReadableAnalysisCommentWritesCompleteOwnership();
        testWideBoardAnalysisExportUsesSgfCoordinatesWhenGtpCannotRepresentMoves();
        testAnalysisErrorSgfRoundTrip();
        testSgfPointRanges();
        testEscapedSgfValuesPreserve();
        testSgfWriterOmitsInvalidPropertyNames();
    } catch (const std::exception& error) {
        std::cerr << "core_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "core_tests passed\n";
    return EXIT_SUCCESS;
}
