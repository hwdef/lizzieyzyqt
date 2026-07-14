#include "AnalysisJson.h"

#include <QJsonArray>
#include <QJsonDocument>
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

void testRequestJson()
{
    AnalysisRequest request;
    request.id = "node-42";
    request.boardSize = BoardSize::square(9);
    request.rules = " NZ ";
    request.komi = 6.5;
    request.includeOwnership = false;
    request.initialPlayer = Color::White;
    request.maxVisits = 100;
    request.maxPlayouts = 250;
    request.maxTime = 3.25;
    request.playoutDoublingAdvantage = -0.5;
    request.wideRootNoise = 0.15;
    request.moves = {
        Move::play(Color::Black, Point{3, 5}),
        Move::pass(Color::White),
    };
    request.initialStones = {
        InitialStone{Color::Black, Point{0, 8}},
        InitialStone{Color::White, Point{8, 0}},
    };

    const QJsonObject json = request.toJson();
    require(json.value("id").toString() == "node-42", "request id should serialize");
    require(json.value("boardXSize").toInt() == 9, "board width should serialize");
    require(json.value("boardYSize").toInt() == 9, "board height should serialize");
    require(
        json.value("rules").toString() == "new-zealand",
        "rules should serialize trimmed SGF aliases in KataGo canonical form");
    require(json.value("komi").toDouble() == 6.5, "komi should serialize");
    require(!json.value("includeOwnership").toBool(), "disabled ownership should serialize");
    require(json.value("initialPlayer").toString() == "W", "initial player should serialize");
    require(json.value("maxVisits").toInt() == 100, "maxVisits should serialize");
    require(json.value("maxPlayouts").toInt() == 250, "maxPlayouts should serialize");
    require(json.value("maxTime").toDouble() == 3.25, "maxTime should serialize");
    require(json.value("includePVVisits").toBool(), "PV visits should be requested");
    const QJsonObject overrideSettings = json.value("overrideSettings").toObject();
    require(
        overrideSettings.value("playoutDoublingAdvantage").toDouble() == -0.5,
        "PDA should serialize through overrideSettings");
    require(
        overrideSettings.value("wideRootNoise").toDouble() == 0.15,
        "wideRootNoise should serialize through overrideSettings");
    require(
        overrideSettings.value("reportAnalysisWinratesAs").toString() == "BLACK",
        "analysis request should force a stable black winrate perspective");

    const QJsonArray moves = json.value("moves").toArray();
    require(moves.size() == 2, "moves should serialize");
    require(moves.at(0).toArray().at(0).toString() == "B", "move color should serialize");
    require(moves.at(0).toArray().at(1).toString() == "D4", "move point should serialize in GTP coordinates");
    require(moves.at(1).toArray().at(1).toString() == "pass", "pass should serialize");

    const std::string line = request.toJsonLine();
    require(!line.empty() && line.back() == '\n', "analysis request should be newline delimited");
    require(QJsonDocument::fromJson(QByteArray::fromStdString(line)).isObject(), "request line should be JSON");
}

void testRequestJsonSkipsNonFiniteSearchOptions()
{
    AnalysisRequest request;
    request.id = "node-non-finite-options";
    request.komi = std::numeric_limits<double>::infinity();
    request.maxTime = std::numeric_limits<double>::infinity();
    request.playoutDoublingAdvantage = std::numeric_limits<double>::quiet_NaN();
    request.wideRootNoise = -std::numeric_limits<double>::infinity();

    const QJsonObject json = request.toJson();
    require(json.value("komi").toDouble() == 7.5, "non-finite request komi should serialize as the default");
    require(!json.contains("maxTime"), "non-finite maxTime should not serialize");
    const QJsonObject overrideSettings = json.value("overrideSettings").toObject();
    require(
        overrideSettings.value("reportAnalysisWinratesAs").toString() == "BLACK",
        "analysis request should keep stable winrate perspective with ignored search options");
    require(
        !overrideSettings.contains("playoutDoublingAdvantage"),
        "non-finite PDA should not serialize through overrideSettings");
    require(
        !overrideSettings.contains("wideRootNoise"),
        "non-finite WRN should not serialize through overrideSettings");
}

void testRequestJsonSkipsNonPositiveSearchOptions()
{
    AnalysisRequest request;
    request.id = "node-disabled-options";
    request.maxVisits = 0;
    request.maxPlayouts = -5;
    request.maxTime = 0.0;
    request.playoutDoublingAdvantage = 0.0;
    request.wideRootNoise = 0.0;

    const QJsonObject json = request.toJson();
    require(!json.contains("maxVisits"), "non-positive maxVisits should not serialize");
    require(!json.contains("maxPlayouts"), "non-positive maxPlayouts should not serialize");
    require(!json.contains("maxTime"), "non-positive maxTime should not serialize");
    const QJsonObject overrideSettings = json.value("overrideSettings").toObject();
    require(
        overrideSettings.value("reportAnalysisWinratesAs").toString() == "BLACK",
        "analysis request should keep stable winrate perspective with disabled search options");
    require(
        !overrideSettings.contains("playoutDoublingAdvantage"),
        "zero PDA should not serialize through overrideSettings");
    require(
        !overrideSettings.contains("wideRootNoise"),
        "zero WRN should not serialize through overrideSettings");
}

void testRequestJsonSkipsUnrepresentablePoints()
{
    AnalysisRequest request;
    request.id = "node-invalid-points";
    request.boardSize = BoardSize::square(9);
    request.moves = {
        Move::play(Color::Black, Point{3, 5}),
        Move::play(Color::White, Point{9, 0}),
        Move::pass(Color::Black),
        Move::resign(Color::White),
        Move::play(Color::Black, Point{-1, 0}),
    };
    request.initialStones = {
        InitialStone{Color::Black, Point{0, 8}},
        InitialStone{Color::White, Point{9, 0}},
        InitialStone{Color::Black, Point{0, -1}},
    };

    const QJsonObject json = request.toJson();
    const QJsonArray moves = json.value("moves").toArray();
    require(moves.size() == 3, "request should skip play moves outside the board");
    require(moves.at(0).toArray().at(1).toString() == "D4", "valid play move should serialize");
    require(moves.at(1).toArray().at(1).toString() == "pass", "pass move should remain after invalid play moves");
    require(moves.at(2).toArray().at(1).toString() == "resign", "resign move should remain after invalid play moves");

    const QJsonArray initialStones = json.value("initialStones").toArray();
    require(initialStones.size() == 1, "request should skip initial stones outside the board");
    require(initialStones.at(0).toArray().at(1).toString() == "A1", "valid initial stone should serialize");

    const std::string line = request.toJsonLine();
    require(line.find("\"\"") == std::string::npos, "request JSON should not contain empty point strings");
}

void testRequestJsonFallsBackForInvalidBoardSize()
{
    AnalysisRequest request;
    request.id = "node-invalid-board";
    request.boardSize = BoardSize{0, 53};
    request.moves = {
        Move::play(Color::Black, Point{3, 3}),
        Move::pass(Color::White),
        Move::resign(Color::Black),
    };
    request.initialStones = {
        InitialStone{Color::Black, Point{0, 0}},
    };

    const QJsonObject json = request.toJson();
    require(json.value("boardXSize").toInt() == 19, "invalid request board width should fall back to 19");
    require(json.value("boardYSize").toInt() == 19, "invalid request board height should fall back to 19");

    const QJsonArray moves = json.value("moves").toArray();
    require(moves.size() == 2, "invalid-board request should not reinterpret play coordinates");
    require(moves.at(0).toArray().at(1).toString() == "pass", "invalid-board request should keep pass moves");
    require(moves.at(1).toArray().at(1).toString() == "resign", "invalid-board request should keep resign moves");
    require(json.value("initialStones").toArray().isEmpty(), "invalid-board request should drop initial stones");

    const std::string line = request.toJsonLine();
    require(line.find("\"\"") == std::string::npos, "invalid-board request JSON should not contain empty point strings");
    require(line.find("D16") == std::string::npos, "invalid-board request should not remap play moves onto 19x19");
}

void testResponseJson()
{
    const std::string json =
        R"({"id":"node-42","rootInfo":{"winrate":0.52,"scoreMean":1.3,"visits":200},)"
        R"("ownership":[0.1,-0.1],)"
        R"("moveInfos":[{"move":"D4","visits":120,"winrate":0.55,"scoreMean":2.1,)"
        R"("scoreStdev":8.2,"policy":0.031,"pv":["D4","Q16","pass"],)"
        R"("pvVisits":[120,80,40],"ownership":[0.2,-0.2]}]})";

    const auto response = parseAnalysisResponse(json, BoardSize::square(19), Color::Black);
    require(response.has_value(), "analysis response should parse");
    require(response->id == "node-42", "response id should parse");
    require(response->rootVisits == 200, "root visits should parse");
    require(response->rootWinrate == 0.52, "root winrate should parse");
    require(response->winratePerspective == Color::Black, "analysis response should use black winrate perspective");
    require(response->ownership.empty(), "incomplete root ownership should be discarded");
    require(response->moveInfos.size() == 1, "moveInfos should parse");

    const MoveCandidate& candidate = response->moveInfos.front();
    require(candidate.move.type == MoveType::Play && candidate.move.point == Point{3, 15}, "candidate move should parse");
    require(candidate.visits == 120, "candidate visits should parse");
    require(candidate.pv.size() == 3, "PV should parse");
    require(candidate.pv[2].type == MoveType::Pass, "PV pass should parse");
    require(candidate.pvVisits.size() == 3 && candidate.pvVisits[2] == 40, "PV visits should parse");
    require(candidate.ownership.empty(), "incomplete candidate ownership should be discarded");
}

void testResponseJsonRootInfoOwnershipFallback()
{
    const std::string json =
        R"({"id":"node-root-ownership",)"
        R"("rootInfo":{"winrate":0.52,"scoreMean":1.3,"visits":200,"ownership":[0.3,-0.3,0.2,-0.2]},)"
        R"("moveInfos":[{"move":"A2","visits":120,"winrate":0.55,"scoreMean":2.1,)"
        R"("scoreStdev":8.2,"policy":0.031,"pv":["A2"],"ownership":[0.6,-0.6,0.4,-0.4]}]})";

    const auto response = parseAnalysisResponse(json, BoardSize::square(2), Color::Black);
    require(response.has_value(), "analysis response with rootInfo ownership should parse");
    require(response->ownership.size() == 4, "rootInfo ownership should populate root ownership when top-level ownership is absent");
    require(
        response->ownership[0] == 0.3 && response->ownership[1] == -0.3,
        "rootInfo ownership values should be preserved");
    require(
        response->moveInfos.front().ownership.size() == 4 &&
            response->moveInfos.front().ownership[0] == 0.6 &&
            response->moveInfos.front().ownership[1] == -0.6,
        "complete candidate ownership should be preserved");

    const std::string topLevelJson =
        R"({"id":"node-top-ownership","ownership":[0.1,-0.1,0.05,-0.05],)"
        R"("rootInfo":{"winrate":0.52,"scoreMean":1.3,"visits":200,"ownership":[0.3,-0.3,0.2,-0.2]},)"
        R"("moveInfos":[{"move":"A2","visits":120,"winrate":0.55,"scoreMean":2.1,)"
        R"("scoreStdev":8.2,"policy":0.031,"pv":["A2"]}]})";

    const auto topLevelResponse = parseAnalysisResponse(topLevelJson, BoardSize::square(2), Color::Black);
    require(topLevelResponse.has_value(), "analysis response with top-level ownership should parse");
    require(
        topLevelResponse->ownership.size() == 4 && topLevelResponse->ownership[0] == 0.1 &&
            topLevelResponse->ownership[1] == -0.1,
        "top-level ownership should take precedence over rootInfo ownership");
}

void testResponseJsonFiltersIncompleteOwnership()
{
    const std::string json =
        R"({"id":"node-incomplete-ownership","ownership":[0.1,-0.1],)"
        R"("rootInfo":{"ownership":[0.3,-0.3,0.2,-0.2]},)"
        R"("moveInfos":[{"move":"A2","visits":20,"winrate":0.52,"scoreMean":0.4,)"
        R"("ownership":[0.6,-0.6]}]})";

    const auto response = parseAnalysisResponse(json, BoardSize::square(2), Color::Black);
    require(response.has_value(), "analysis response with incomplete top-level ownership should parse");
    require(
        response->ownership.size() == 4 && response->ownership[0] == 0.3 &&
            response->ownership[1] == -0.3,
        "complete rootInfo ownership should be used when top-level ownership is incomplete");
    require(response->moveInfos.size() == 1, "valid candidate should remain with incomplete ownership");
    require(response->moveInfos.front().ownership.empty(), "incomplete candidate ownership should be discarded");

    const auto ownershipOnly = parseAnalysisResponse(
        R"({"id":"node-only-incomplete-ownership","ownership":[0.1,-0.1]})",
        BoardSize::square(2),
        Color::Black);
    require(!ownershipOnly.has_value(), "ownership-only response with incomplete ownership should reject");

    const std::string malformedJson =
        R"({"id":"node-malformed-ownership","ownership":[0.1,-0.1,0.05,-0.05,"bad"],)"
        R"("rootInfo":{"winrate":0.52,"scoreMean":1.3,"visits":200,)"
        R"("ownership":[0.3,-0.3,0.2,-0.2,"bad"]},)"
        R"("moveInfos":[{"move":"A2","visits":20,"winrate":0.52,"scoreMean":0.4,)"
        R"("ownership":[0.6,-0.6,0.4,-0.4,"bad"]}]})";
    const auto malformedResponse = parseAnalysisResponse(malformedJson, BoardSize::square(2), Color::Black);
    require(malformedResponse.has_value(), "analysis response with malformed ownership should keep scalar fields");
    require(malformedResponse->ownership.empty(), "malformed top-level/rootInfo ownership should be discarded");
    require(malformedResponse->moveInfos.size() == 1, "valid candidate should remain with malformed ownership");
    require(malformedResponse->moveInfos.front().ownership.empty(), "malformed candidate ownership should be discarded");

    const auto malformedOwnershipOnly = parseAnalysisResponse(
        R"({"id":"node-only-malformed-ownership","ownership":[0.1,-0.1,0.05,-0.05,"bad"]})",
        BoardSize::square(2),
        Color::Black);
    require(!malformedOwnershipOnly.has_value(), "ownership-only response with malformed ownership should reject");
}

void testResponseJsonKataGoPriorAliases()
{
    const std::string json =
        R"({"id":"node-prior","rootInfo":{"winrate":0.52,"scoreLead":1.3,"visits":200},)"
        R"("moveInfos":[{"move":"D4","visits":120,"winrate":0.55,"scoreLead":2.1,)"
        R"("scoreStdev":8.2,"prior":0.031,"pv":["D4"],"pvVisits":[120]}]})";

    const auto response = parseAnalysisResponse(json, BoardSize::square(19), Color::Black);
    require(response.has_value(), "analysis response with KataGo aliases should parse");
    require(response->rootScoreMean == 1.3, "root scoreLead should parse as score mean");
    require(response->moveInfos.size() == 1, "aliased moveInfos should parse");

    const MoveCandidate& candidate = response->moveInfos.front();
    require(candidate.scoreMean == 2.1, "candidate scoreLead should parse as score mean");
    require(candidate.policy == 0.031, "candidate prior should parse as policy");
    require(candidate.pvVisits.size() == 1 && candidate.pvVisits.front() == 120, "candidate PV visits should parse");
}

void testResponseJsonFallsBackToBestMoveInfoWhenRootInfoIsMissing()
{
    const std::string json =
        R"({"id":"node-no-root",)"
        R"("moveInfos":[)"
        R"({"move":"Q16","visits":50,"winrate":0.48,"scoreMean":-0.6,"policy":0.12,"pv":["Q16"]},)"
        R"({"move":"D4","visits":120,"winrate":0.55,"scoreLead":2.1,"prior":0.031,"pv":["D4","Q16"]})"
        R"(]})";

    const auto response = parseAnalysisResponse(json, BoardSize::square(19), Color::Black);
    require(response.has_value(), "analysis response without rootInfo should parse");
    require(response->moveInfos.size() == 2, "analysis response without rootInfo should retain moveInfos");
    require(
        response->moveInfos.front().move.point == Point{3, 15},
        "analysis response without rootInfo should sort the most-visited candidate first");
    require(response->rootVisits == 120, "missing rootInfo should fall back to the most-visited candidate visits");
    require(response->rootWinrate == 0.55, "missing rootInfo should fall back to the most-visited candidate winrate");
    require(response->rootScoreMean == 2.1, "missing rootInfo should fall back to the most-visited candidate score");
}

void testResponseJsonPartialRootInfoFallsBackPerField()
{
    const std::string partialJson =
        R"({"id":"node-partial-root",)"
        R"("rootInfo":{"visits":200,"winrate":"bad","scoreMean":"bad"},)"
        R"("moveInfos":[{"move":"D4","visits":120,"winrate":0.55,"scoreLead":2.1,)"
        R"("scoreStdev":"bad","policy":"bad","pv":["D4"]}]})";

    const auto partialResponse = parseAnalysisResponse(partialJson, BoardSize::square(19), Color::Black);
    require(partialResponse.has_value(), "analysis response with partial rootInfo should parse");
    require(partialResponse->rootVisits == 200, "valid rootInfo visits should remain authoritative");
    require(
        partialResponse->rootWinrate == 0.55,
        "invalid rootInfo winrate should fall back to the best candidate");
    require(
        partialResponse->rootScoreMean == 2.1,
        "invalid rootInfo score should fall back to the best candidate");
    require(partialResponse->moveInfos.size() == 1, "partial-root response should retain the valid candidate");
    require(partialResponse->moveInfos.front().scoreStdev == 0.0, "invalid candidate stdev should be ignored");
    require(partialResponse->moveInfos.front().policy == 0.0, "invalid candidate policy should be ignored");

    const std::string invalidVisitsJson =
        R"({"id":"node-invalid-root-visits","rootInfo":{"visits":12.5},)"
        R"("moveInfos":[{"move":"C3","visits":80,"winrate":0.50,"scoreMean":1.2,"pv":["C3"]}]})";

    const auto invalidVisitsResponse =
        parseAnalysisResponse(invalidVisitsJson, BoardSize::square(19), Color::Black);
    require(invalidVisitsResponse.has_value(), "analysis response with invalid root visits should parse candidates");
    require(invalidVisitsResponse->rootVisits == 80, "invalid rootInfo visits should fall back to the best candidate");
    require(
        invalidVisitsResponse->rootWinrate == 0.50,
        "missing rootInfo winrate should fall back to the best candidate");
    require(
        invalidVisitsResponse->rootScoreMean == 1.2,
        "missing rootInfo score should fall back to the best candidate");

    const std::string zeroVisitsJson =
        R"({"id":"node-zero-root-visits","rootInfo":{"visits":0},)"
        R"("moveInfos":[{"move":"Q16","visits":45,"winrate":0.47,"scoreMean":-0.8,"pv":["Q16"]}]})";

    const auto zeroVisitsResponse = parseAnalysisResponse(zeroVisitsJson, BoardSize::square(19), Color::Black);
    require(zeroVisitsResponse.has_value(), "analysis response with zero root visits should parse candidates");
    require(zeroVisitsResponse->rootVisits == 45, "zero rootInfo visits should fall back to the best candidate");
    require(zeroVisitsResponse->rootWinrate == 0.47, "zero-visits rootInfo should fall back to candidate winrate");
    require(zeroVisitsResponse->rootScoreMean == -0.8, "zero-visits rootInfo should fall back to candidate score");
}

void testResponseJsonSortsCandidatesByVisitsAndMoveKey()
{
    const std::string json =
        R"({"id":"node-sorted","rootInfo":{"winrate":0.51,"scoreLead":0.4,"visits":300},)"
        R"("moveInfos":[)"
        R"({"move":"pass","visits":40,"winrate":0.47,"scoreMean":-0.3,"policy":0.05,"pv":["pass"]},)"
        R"({"move":"D4","visits":120,"winrate":0.55,"scoreLead":2.1,"prior":0.031,"pv":["D4"]},)"
        R"({"move":"Q16","visits":50,"winrate":0.48,"scoreMean":-0.6,"policy":0.12,"pv":["Q16"]},)"
        R"({"move":"C3","visits":120,"winrate":0.53,"scoreMean":1.8,"policy":0.04,"pv":["C3"]})"
        R"(]})";

    const auto response = parseAnalysisResponse(json, BoardSize::square(19), Color::Black);
    require(response.has_value(), "analysis response with unsorted candidates should parse");
    require(response->moveInfos.size() == 4, "analysis response should retain all valid moveInfos");
    require(
        response->moveInfos.at(0).move.type == MoveType::Play && response->moveInfos.at(0).move.point == Point{2, 16},
        "candidate ties should sort by move key after visits");
    require(
        response->moveInfos.at(1).move.type == MoveType::Play && response->moveInfos.at(1).move.point == Point{3, 15},
        "second tied candidate should follow move-key order");
    require(
        response->moveInfos.at(2).move.type == MoveType::Play && response->moveInfos.at(2).move.point == Point{15, 3},
        "lower-visit play candidate should follow tied leaders");
    require(response->moveInfos.at(3).move.type == MoveType::Pass, "lowest-visit pass candidate should sort last");
    require(response->rootVisits == 300, "explicit rootInfo should remain authoritative after candidate sorting");
}

void testResponseJsonNormalizesRateScales()
{
    const std::string json =
        R"({"id":"node-scales","rootInfo":{"winrate":6345,"scoreMean":1.3,"visits":200},)"
        R"("moveInfos":[{"move":"D4","visits":120,"winrate":61.23,"scoreMean":2.1,)"
        R"("scoreStdev":8.2,"prior":2222,"pv":["D4"]}]})";

    const auto response = parseAnalysisResponse(json, BoardSize::square(19), Color::Black);
    require(response.has_value(), "analysis response with legacy rate scales should parse");
    require(response->rootWinrate > 0.6344 && response->rootWinrate < 0.6346, "legacy root winrate should normalize");
    require(response->moveInfos.size() == 1, "legacy-scaled moveInfos should parse");

    const MoveCandidate& candidate = response->moveInfos.front();
    require(candidate.winrate > 0.6122 && candidate.winrate < 0.6124, "percent candidate winrate should normalize");
    require(candidate.policy > 0.2221 && candidate.policy < 0.2223, "legacy candidate prior should normalize");
}

void testResponseJsonImportsNumericStringFields()
{
    const std::string json =
        R"({"id":"node-string-numbers",)"
        R"("rootInfo":{"winrate":"63.45","scoreLead":"2.75","visits":"88",)"
        R"("ownership":["0.1","-0.1","0.2","-0.2"]},)"
        R"("moveInfos":[{"move":"A2","visits":"44","winrate":"61.23","scoreLead":"1.25",)"
        R"("scoreStdev":"4.5","prior":"12.5","pv":["A2","pass"],"pvVisits":["44","22"],)"
        R"("ownership":["0.3","-0.3","0.1","-0.1"]}]})";

    const auto response = parseAnalysisResponse(json, BoardSize::square(2), Color::Black);
    require(response.has_value(), "numeric-string Analysis JSON should parse");
    require(response->rootVisits == 88, "numeric-string root visits should parse");
    require(
        response->rootWinrate > 0.6344 && response->rootWinrate < 0.6346,
        "numeric-string root winrate should normalize");
    require(
        response->rootScoreMean > 2.74 && response->rootScoreMean < 2.76,
        "numeric-string root score should parse");
    require(
        response->ownership.size() == 4 && response->ownership[0] == 0.1 && response->ownership[1] == -0.1,
        "numeric-string root ownership should parse");
    require(response->moveInfos.size() == 1, "numeric-string moveInfos should parse");

    const MoveCandidate& candidate = response->moveInfos.front();
    require(candidate.visits == 44, "numeric-string candidate visits should parse");
    require(
        candidate.winrate > 0.6122 && candidate.winrate < 0.6124,
        "numeric-string candidate winrate should normalize");
    require(candidate.scoreMean > 1.24 && candidate.scoreMean < 1.26, "numeric-string candidate score should parse");
    require(candidate.scoreStdev > 4.49 && candidate.scoreStdev < 4.51, "numeric-string candidate stdev should parse");
    require(candidate.policy > 0.124 && candidate.policy < 0.126, "numeric-string candidate prior should normalize");
    require(candidate.pvVisits.size() == 2 && candidate.pvVisits[1] == 22, "numeric-string PV visits should parse");
    require(
        candidate.ownership.size() == 4 && candidate.ownership[0] == 0.3 && candidate.ownership[1] == -0.3,
        "numeric-string candidate ownership should parse");
}

void testResponseJsonRejectsInvalidVisitCounts()
{
    const std::string json =
        R"({"id":"node-visit-counts",)"
        R"("moveInfos":[)"
        R"({"move":"D4","visits":120.5,"winrate":0.55,"scoreMean":2.1,"pv":["D4"],"pvVisits":[120]},)"
        R"({"move":"C3","visits":80,"winrate":0.50,"scoreMean":1.2,"pv":["C3"],"pvVisits":[80,40.5,20]})"
        R"(]})";

    const auto response = parseAnalysisResponse(json, BoardSize::square(19), Color::Black);
    require(response.has_value(), "analysis response should keep candidates with valid integer visits");
    require(response->moveInfos.size() == 1, "analysis response should skip candidates with fractional visits");
    require(response->moveInfos.front().move.point == Point{2, 16}, "valid candidate should remain after skipped visits");
    require(response->rootVisits == 80, "candidate fallback should use the valid candidate visits");
    require(
        response->moveInfos.front().pvVisits.empty(),
        "Analysis JSON pvVisits with invalid entries should be discarded");

    const auto invalidRootOnly =
        parseAnalysisResponse(R"({"id":"node-invalid-root","rootInfo":{"visits":12.5}})", BoardSize::square(19), Color::Black);
    require(!invalidRootOnly.has_value(), "rootInfo with only fractional visits should not become a success response");

    const auto zeroRootOnly =
        parseAnalysisResponse(R"({"id":"node-zero-root","rootInfo":{"visits":0}})", BoardSize::square(19), Color::Black);
    require(!zeroRootOnly.has_value(), "rootInfo with only zero visits should not become a success response");
}

void testResponseRejectsMalformedPv()
{
    const std::string json =
        R"({"id":"node-bad-pv","rootInfo":{"winrate":0.52,"scoreMean":1.3,"visits":200},)"
        R"("moveInfos":[{"move":"D4","visits":120,"winrate":0.55,"scoreMean":2.1,)"
        R"("scoreStdev":8.2,"policy":0.031,"pv":["D4","not-a-point","Q16","pass"]}]})";

    const auto response = parseAnalysisResponse(json, BoardSize::square(19), Color::Black);
    require(response.has_value(), "analysis response with malformed PV token should parse");
    require(response->moveInfos.size() == 1, "valid moveInfo should be retained");

    require(response->moveInfos.front().pv.empty(), "malformed Analysis JSON PV should be discarded");
}

void testResponseDiscardsMismatchedPvVisits()
{
    const std::string json =
        R"({"id":"node-mismatched-pv-visits","rootInfo":{"visits":120},)"
        R"("moveInfos":[{"move":"D4","visits":120,"winrate":0.55,"scoreMean":2.1,)"
        R"("pv":["D4","Q16"],"pvVisits":[120]}]})";

    const auto response = parseAnalysisResponse(json, BoardSize::square(19), Color::Black);
    require(response.has_value(), "analysis response with mismatched PV visits should parse");
    require(response->moveInfos.size() == 1, "mismatched PV visits should keep candidate");
    require(response->moveInfos.front().pv.size() == 2, "mismatched PV visits should keep PV");
    require(response->moveInfos.front().pvVisits.empty(), "mismatched Analysis JSON PV visits should be discarded");
}

void testErrorResponseJson()
{
    const std::string json = R"({"id":"node-error","error":"illegal position"})";

    const auto response = parseAnalysisResponse(json, BoardSize::square(19), Color::Black);
    require(response.has_value(), "analysis error response should parse");
    require(response->id == "node-error", "analysis error response id should parse");
    require(response->errorMessage == "illegal position", "analysis error message should parse");
    require(response->moveInfos.empty(), "analysis error response should not fabricate moveInfos");
    require(response->rootVisits == 0, "analysis error response should not fabricate root visits");
    require(response->ownership.empty(), "analysis error response should not fabricate ownership");

    const std::string noisyErrorJson =
        R"({"id":"node-noisy-error","error":"analysis failed",)"
        R"("rootInfo":{"winrate":0.52,"scoreMean":1.3,"visits":200},)"
        R"("ownership":[0.1,-0.1],)"
        R"("moveInfos":[{"move":"D4","visits":120,"winrate":0.55,"scoreMean":2.1}]})";
    const auto noisyErrorResponse = parseAnalysisResponse(noisyErrorJson, BoardSize::square(19), Color::Black);
    require(noisyErrorResponse.has_value(), "analysis error response with extra fields should parse");
    require(noisyErrorResponse->id == "node-noisy-error", "noisy analysis error response id should parse");
    require(noisyErrorResponse->errorMessage == "analysis failed", "noisy analysis error message should parse");
    require(noisyErrorResponse->moveInfos.empty(), "analysis error response should ignore moveInfos");
    require(noisyErrorResponse->ownership.empty(), "analysis error response should ignore ownership");
    require(noisyErrorResponse->rootVisits == 0, "analysis error response should ignore rootInfo visits");
    require(noisyErrorResponse->rootWinrate == 0.0, "analysis error response should ignore rootInfo winrate");
    require(noisyErrorResponse->rootScoreMean == 0.0, "analysis error response should ignore rootInfo score");
}

void testEmptySuccessResponseJsonIsRejected()
{
    const auto emptyResponse = parseAnalysisResponse(R"({"id":"node-empty"})", BoardSize::square(19), Color::Black);
    require(!emptyResponse.has_value(), "analysis response without rootInfo, moveInfos, ownership, or error should reject");

    const std::string invalidOnlyJson =
        R"({"id":"node-invalid-only","moveInfos":[{"move":"not-a-point","visits":120}]})";
    const auto invalidOnlyResponse = parseAnalysisResponse(invalidOnlyJson, BoardSize::square(19), Color::Black);
    require(!invalidOnlyResponse.has_value(), "analysis response with no valid moveInfos should reject");
}

void testResponseJsonRejectsMissingIds()
{
    const auto missingSuccessId = parseAnalysisResponse(
        R"({"rootInfo":{"winrate":0.52,"scoreMean":1.3,"visits":200}})",
        BoardSize::square(19),
        Color::Black);
    require(!missingSuccessId.has_value(), "analysis success response without id should reject");

    const auto emptySuccessId = parseAnalysisResponse(
        R"({"id":"","rootInfo":{"winrate":0.52,"scoreMean":1.3,"visits":200}})",
        BoardSize::square(19),
        Color::Black);
    require(!emptySuccessId.has_value(), "analysis success response with empty id should reject");

    const auto missingErrorId =
        parseAnalysisResponse(R"({"error":"illegal position"})", BoardSize::square(19), Color::Black);
    require(!missingErrorId.has_value(), "analysis error response without id should reject");
}

void testResponseJsonRejectsInvalidBoardSizeForSuccess()
{
    const std::string successJson =
        R"({"id":"node-invalid-board-response","rootInfo":{"winrate":0.52,"scoreMean":1.3,"visits":200},)"
        R"("moveInfos":[{"move":"pass","visits":120,"winrate":0.55,"scoreMean":2.1}]})";
    const auto successResponse = parseAnalysisResponse(successJson, BoardSize{0, 53}, Color::Black);
    require(!successResponse.has_value(), "successful analysis response with invalid board size should reject");

    const std::string errorJson = R"({"id":"node-invalid-board-error","error":"illegal board size"})";
    const auto errorResponse = parseAnalysisResponse(errorJson, BoardSize{0, 53}, Color::Black);
    require(errorResponse.has_value(), "analysis error response should parse even when board size is invalid");
    require(errorResponse->id == "node-invalid-board-error", "invalid-board error response id should parse");
    require(errorResponse->errorMessage == "illegal board size", "invalid-board error payload should remain visible");
}

void testRectangularAnalysisJson()
{
    AnalysisRequest request;
    request.id = "rect-node";
    request.boardSize = BoardSize{9, 13};
    request.moves = {
        Move::play(Color::Black, Point{7, 12}),
        Move::play(Color::White, Point{8, 0}),
    };
    request.initialStones = {
        InitialStone{Color::Black, Point{0, 12}},
        InitialStone{Color::White, Point{8, 0}},
    };

    const QJsonObject json = request.toJson();
    require(json.value("boardXSize").toInt() == 9, "rectangular request width should serialize");
    require(json.value("boardYSize").toInt() == 13, "rectangular request height should serialize");
    const QJsonArray moves = json.value("moves").toArray();
    require(moves.at(0).toArray().at(1).toString() == "H1", "rectangular move should use board height");
    require(moves.at(1).toArray().at(1).toString() == "J13", "rectangular move should skip I column");
    const QJsonArray initialStones = json.value("initialStones").toArray();
    require(initialStones.at(0).toArray().at(1).toString() == "A1", "rectangular initial stone should serialize");
    require(initialStones.at(1).toArray().at(1).toString() == "J13", "rectangular top edge should serialize");

    const std::string responseJson =
        R"({"id":"rect-node","rootInfo":{"winrate":0.49,"scoreMean":-0.4,"visits":64},)"
        R"("ownership":[0.1,0.2,0.3],)"
        R"("moveInfos":[{"move":"H1","visits":64,"winrate":0.49,"scoreMean":-0.4,)"
        R"("scoreStdev":5.0,"policy":0.15,"pv":["H1","J13","pass"],"pvVisits":[64,32,16]}]})";

    const auto response = parseAnalysisResponse(responseJson, BoardSize{9, 13}, Color::Black);
    require(response.has_value(), "rectangular response should parse");
    require(response->moveInfos.size() == 1, "rectangular moveInfos should parse");
    const MoveCandidate& candidate = response->moveInfos.front();
    require(candidate.move.point == Point{7, 12}, "rectangular candidate should parse with board height");
    require(candidate.pv.size() == 3, "rectangular PV should parse");
    require(candidate.pv[0].point == Point{7, 12}, "rectangular PV first move should parse");
    require(candidate.pv[1].point == Point{8, 0}, "rectangular PV top edge should parse");
    require(candidate.pv[2].type == MoveType::Pass, "rectangular PV pass should parse");
}

void testWideBoardAnalysisJson()
{
    AnalysisRequest request;
    request.id = "wide-node";
    request.boardSize = BoardSize{26, 9};
    request.moves = {
        Move::play(Color::Black, Point{25, 8}),
        Move::play(Color::White, Point{0, 0}),
    };
    request.initialStones = {
        InitialStone{Color::Black, Point{25, 0}},
        InitialStone{Color::White, Point{0, 8}},
    };

    const QJsonObject json = request.toJson();
    require(json.value("boardXSize").toInt() == 26, "wide request width should serialize");
    require(json.value("boardYSize").toInt() == 9, "wide request height should serialize");
    const QJsonArray moves = json.value("moves").toArray();
    require(moves.at(0).toArray().at(1).toString() == "(25,8)", "wide lower edge should use KataGo x,y");
    require(moves.at(1).toArray().at(1).toString() == "(0,0)", "wide top edge should use KataGo x,y");
    const QJsonArray initialStones = json.value("initialStones").toArray();
    require(
        initialStones.at(0).toArray().at(1).toString() == "(25,0)",
        "wide initial stone should use KataGo x,y");
    require(
        initialStones.at(1).toArray().at(1).toString() == "(0,8)",
        "wide lower-left initial stone should use KataGo x,y");

    const std::string responseJson =
        "{\"id\":\"wide-node\",\"rootInfo\":{\"winrate\":0.48,\"scoreMean\":-0.2,\"visits\":32},"
        "\"moveInfos\":[{\"move\":\"AA1\",\"visits\":32,\"winrate\":0.48,\"scoreMean\":-0.2,"
        "\"scoreStdev\":4.0,\"policy\":0.25,\"pv\":[\"AA1\",\"(0,0)\",\"pass\"],"
        "\"pvVisits\":[32,16,8]}]}";

    const auto response = parseAnalysisResponse(responseJson, BoardSize{26, 9}, Color::Black);
    require(response.has_value(), "wide response should parse");
    require(response->moveInfos.size() == 1, "wide moveInfos should parse");
    const MoveCandidate& candidate = response->moveInfos.front();
    require(candidate.move.point == Point{25, 8}, "KataGo extended column should parse");
    require(candidate.pv.size() == 3, "wide PV should parse");
    require(candidate.pv[0].point == Point{25, 8}, "wide PV extended column should parse");
    require(candidate.pv[1].point == Point{0, 0}, "wide PV x,y coordinate should parse");
    require(candidate.pv[2].type == MoveType::Pass, "wide PV pass should parse");

    AnalysisRequest maxWideRequest;
    maxWideRequest.id = "max-wide-node";
    maxWideRequest.boardSize = BoardSize{52, 52};
    maxWideRequest.moves = {
        Move::play(Color::Black, Point{51, 51}),
    };
    maxWideRequest.initialStones = {
        InitialStone{Color::White, Point{51, 0}},
    };

    const QJsonObject maxWideJson = maxWideRequest.toJson();
    require(maxWideJson.value("boardXSize").toInt() == 52, "max-wide request width should serialize");
    require(maxWideJson.value("boardYSize").toInt() == 52, "max-wide request height should serialize");
    require(
        maxWideJson.value("moves").toArray().at(0).toArray().at(1).toString() == "(51,51)",
        "max-wide lower-right move should serialize with KataGo x,y");
    require(
        maxWideJson.value("initialStones").toArray().at(0).toArray().at(1).toString() == "(51,0)",
        "max-wide top-right initial stone should serialize with KataGo x,y");

    const std::string maxWideResponseJson =
        "{\"id\":\"max-wide-node\",\"rootInfo\":{\"winrate\":0.51,\"scoreMean\":0.2,\"visits\":16},"
        "\"moveInfos\":[{\"move\":\"BB1\",\"visits\":16,\"winrate\":0.51,\"scoreMean\":0.2,"
        "\"scoreStdev\":3.0,\"policy\":0.17,\"pv\":[\"BB1\",\"(0,0)\",\"pass\"],"
        "\"pvVisits\":[16,8,4]}]}";

    const auto maxWideResponse = parseAnalysisResponse(maxWideResponseJson, BoardSize{52, 52}, Color::Black);
    require(maxWideResponse.has_value(), "max-wide response should parse");
    require(maxWideResponse->moveInfos.size() == 1, "max-wide moveInfos should parse");
    const MoveCandidate& maxWideCandidate = maxWideResponse->moveInfos.front();
    require(maxWideCandidate.move.point == Point{51, 51}, "max-wide extended BB column should parse");
    require(maxWideCandidate.pv.size() == 3, "max-wide PV should parse");
    require(maxWideCandidate.pv[0].point == Point{51, 51}, "max-wide PV BB point should parse");
    require(maxWideCandidate.pv[1].point == Point{0, 0}, "max-wide PV machine coordinate should parse");
    require(maxWideCandidate.pv[2].type == MoveType::Pass, "max-wide PV pass should parse");
}

}  // namespace

int main()
{
    try {
        testRequestJson();
        testRequestJsonSkipsNonFiniteSearchOptions();
        testRequestJsonSkipsNonPositiveSearchOptions();
        testRequestJsonSkipsUnrepresentablePoints();
        testRequestJsonFallsBackForInvalidBoardSize();
        testResponseJson();
        testResponseJsonRootInfoOwnershipFallback();
        testResponseJsonFiltersIncompleteOwnership();
        testResponseJsonKataGoPriorAliases();
        testResponseJsonFallsBackToBestMoveInfoWhenRootInfoIsMissing();
        testResponseJsonPartialRootInfoFallsBackPerField();
        testResponseJsonSortsCandidatesByVisitsAndMoveKey();
        testResponseJsonNormalizesRateScales();
        testResponseJsonImportsNumericStringFields();
        testResponseJsonRejectsInvalidVisitCounts();
        testResponseRejectsMalformedPv();
        testResponseDiscardsMismatchedPvVisits();
        testErrorResponseJson();
        testEmptySuccessResponseJsonIsRejected();
        testResponseJsonRejectsMissingIds();
        testResponseJsonRejectsInvalidBoardSizeForSuccess();
        testRectangularAnalysisJson();
        testWideBoardAnalysisJson();
    } catch (const std::exception& error) {
        std::cerr << "analysis_json_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "analysis_json_tests passed\n";
    return EXIT_SUCCESS;
}
