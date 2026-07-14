#include "GtpCommandQueue.h"
#include "GtpProtocol.h"
#include "KataGoConfig.h"
#include "ProcessDiagnostics.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
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

void testCommandFormatting()
{
    const GtpCommand command{7, "play", {"B", "D4"}};
    require(command.toLine() == "7 play B D4\n", "GTP command should include id, name, arguments, and newline");

    require(isValidGtpCommandName("kata-analyze"), "simple GTP command names should be valid");
    require(!isValidGtpCommandName(""), "empty GTP command names should be invalid");
    require(!isValidGtpCommandName("kata-analyze clear_board"), "GTP command names with spaces should be invalid");
    require(!isValidGtpCommandName("kata-stop\nclear_board"), "GTP command names with newlines should be invalid");

    const GtpCommand invalidIdCommand{-7, "name", {}};
    require(invalidIdCommand.toLine() == "name\n", "GTP command should not serialize non-positive ids");

    const GtpCommand multilineCommand{8, "kata-stop\nclear_board", {"B\rW", "D4\nQ16"}};
    const std::string line = multilineCommand.toLine();
    require(
        line == "8 kata-stop clear_board B W D4 Q16\n",
        "GTP command serialization should replace embedded CR/LF with spaces");
    require(
        line.find('\n') == line.size() - 1,
        "GTP command serialization should keep embedded text on a single protocol line");
}

void testResponseParser()
{
    GtpStreamParser parser;
    require(!parser.feedLine("=7 KataGo").has_value(), "response header should wait for blank line");
    require(!parser.feedLine("extra").has_value(), "response body should wait for blank line");
    const std::optional<GtpEvent> event = parser.feedLine("");
    require(event.has_value(), "blank line should finish response");
    require(event->type == GtpEvent::Type::Response, "event should be a response");
    require(event->response.has_value(), "response event should contain response");
    require(event->response->success, "response should be successful");
    require(event->response->id == 7, "response id should parse");
    require(event->response->payload == "KataGo\nextra", "multi-line payload should be preserved");
}

void testResponseParserKeepsNumericPayloadsUnnumbered()
{
    GtpStreamParser parser;
    require(!parser.feedLine("=1.15.0").has_value(), "numeric success payload should wait for blank line");
    const std::optional<GtpEvent> event = parser.feedLine("");
    require(event.has_value(), "blank line should finish numeric success payload");
    require(event->type == GtpEvent::Type::Response, "numeric success payload should still be a response");
    require(event->response.has_value(), "numeric success payload should contain response data");
    require(!event->response->id.has_value(), "numeric success payload without a separator should not parse as an id");
    require(event->response->payload == "1.15.0", "numeric success payload should be preserved");

    GtpStreamParser errorParser;
    require(!errorParser.feedLine("?404-not-found").has_value(), "numeric error payload should wait for blank line");
    const std::optional<GtpEvent> errorEvent = errorParser.feedLine("");
    require(errorEvent.has_value(), "blank line should finish numeric error payload");
    require(errorEvent->response.has_value(), "numeric error payload should contain response data");
    require(!errorEvent->response->id.has_value(), "numeric error payload without a separator should not parse as an id");
    require(errorEvent->response->payload == "404-not-found", "numeric error payload should be preserved");
}

void testErrorResponseParser()
{
    GtpStreamParser parser;
    require(!parser.feedLine("?13 illegal move\r").has_value(), "error response header should accept CRLF input");
    require(!parser.feedLine("details: point occupied").has_value(), "error response body should wait for blank line");
    const std::optional<GtpEvent> event = parser.feedLine("");
    require(event.has_value(), "blank line should finish error response");
    require(event->type == GtpEvent::Type::Response, "error event should be a response");
    require(event->response.has_value(), "error response event should contain response data");
    require(!event->response->success, "error response should not be successful");
    require(event->response->id == 13, "error response id should parse");
    require(
        event->response->payload == "illegal move\ndetails: point occupied",
        "multi-line error payload should be preserved");
    require(event->response->lines.size() == 2, "error response lines should be preserved separately");
}

void testResponseParserRejectsOverflowIds()
{
    GtpStreamParser parser;
    const std::optional<GtpEvent> event = parser.feedLine("=999999999999999999999999 ok");
    require(event.has_value(), "overflow response id should be emitted as a raw protocol line");
    require(event->type == GtpEvent::Type::RawLine, "overflow response id should not produce a response event");
    require(!event->response.has_value(), "overflow response id should not resolve through the command queue");
    require(event->rawLine == "=999999999999999999999999 ok", "overflow response line should be preserved");
    require(!parser.feedLine("").has_value(), "blank line after rejected response header should not finish a response");
}

void testCommandQueue()
{
    GtpCommandQueue queue;
    GtpCommand first = queue.enqueue(GtpCommand{std::nullopt, "name", {}});
    GtpCommand second = queue.enqueue(GtpCommand{std::nullopt, "version", {}});

    require(first.id == 1 && second.id == 2, "queue should assign increasing ids");
    require(queue.nextCommandId() == 3, "queue should expose the next assigned id");
    require(queue.pendingCount() == 2, "queue should track pending commands");

    GtpResponse secondResponse;
    secondResponse.success = true;
    secondResponse.id = 2;
    secondResponse.payload = "1.15.0";
    QueuedGtpResponse resolvedSecond = queue.resolve(secondResponse);
    require(resolvedSecond.command.has_value(), "id response should resolve a pending command");
    require(resolvedSecond.command->id == 2, "id response should resolve the matching command");
    require(resolvedSecond.command->name == "version", "resolved command should keep the original name");
    require(queue.pendingCount() == 1, "resolving an id response should remove only that command");

    GtpResponse unknownResponse;
    unknownResponse.success = true;
    unknownResponse.id = 99;
    QueuedGtpResponse unresolved = queue.resolve(unknownResponse);
    require(!unresolved.command.has_value(), "unknown id response should not be assigned to another command");
    require(queue.pendingCount() == 1, "unknown id response should leave pending commands untouched");

    GtpResponse unnumberedResponse;
    unnumberedResponse.success = true;
    unnumberedResponse.payload = "KataGo";
    QueuedGtpResponse resolvedFirst = queue.resolve(unnumberedResponse);
    require(resolvedFirst.command.has_value(), "unnumbered response should resolve the oldest pending command");
    require(resolvedFirst.command->id == 1, "unnumbered response should resolve in FIFO order");
    require(queue.pendingCount() == 0, "all commands should be resolved");

    GtpCommand explicitId = queue.enqueue(GtpCommand{12, "list_commands", {}});
    require(explicitId.id == 12, "queue should preserve an explicit command id");
    require(queue.nextCommandId() == 13, "explicit high id should advance the next generated id");

    GtpCommand negativeId = queue.enqueue(GtpCommand{-4, "invalid-negative", {}});
    require(negativeId.id == 13, "queue should replace negative explicit ids with the next generated id");
    require(queue.nextCommandId() == 14, "negative explicit ids should still advance the generated id");

    GtpCommand duplicateId = queue.enqueue(GtpCommand{12, "duplicate-id", {}});
    require(duplicateId.id == 14, "queue should replace duplicate explicit ids with a generated id");
    require(queue.nextCommandId() == 15, "duplicate explicit ids should advance the generated id");

    GtpCommandQueue overflowQueue;
    GtpCommand overflowId = overflowQueue.enqueue(
        GtpCommand{std::numeric_limits<int>::max(), "overflow-id", {}});
    require(overflowId.id == 1, "queue should replace max int explicit ids to avoid next-id overflow");
    require(overflowQueue.nextCommandId() == 2, "queue should keep next id usable after max int explicit ids");

    queue.reset();
    require(queue.nextCommandId() == 1 && queue.pendingCount() == 0, "reset should clear pending commands and ids");
}

void testRawProtocolLine()
{
    GtpStreamParser parser;
    const std::optional<GtpEvent> event = parser.feedLine("unstructured engine log");
    require(event.has_value(), "raw protocol line should create an event");
    require(event->type == GtpEvent::Type::RawLine, "event should be raw line");
    require(event->rawLine == "unstructured engine log", "raw line should be preserved");
}

void testCapabilities()
{
    const EngineCapabilities capabilities =
        EngineCapabilities::fromListCommandsPayload(
            "name\nversion\nkata-analyze\nkata-set-rules\nkata-raw-nn\nkata-stop\nkata-set-param\ngenmove\n");
    require(capabilities.kataAnalyze, "kata-analyze should be detected");
    require(capabilities.kataSetRules, "kata-set-rules should be detected");
    require(capabilities.kataRawNn, "kata-raw-nn should be detected");
    require(capabilities.kataStop, "kata-stop should be detected");
    require(capabilities.kataSetParam, "kata-set-param should be detected");
    require(capabilities.genmove, "genmove should be detected");
}

void testAnalyzeInfo()
{
    const auto info = parseKataAnalyzeInfo(
        "info move D4 visits 123 winrate 5234 scoreLead 1.5 scoreStdev 8.25 prior 0.031 pv D4 Q16 C3 pvVisits 123 80 40 ownership 0.1 -0.2");
    require(info.has_value(), "kata-analyze info line should parse");
    require(info->move == "D4", "move should parse");
    require(info->visits == 123, "visits should parse");
    require(info->winrate == 5234.0, "winrate should parse");
    require(info->scoreMean == 1.5, "scoreLead should parse as score mean");
    require(info->scoreStdev == 8.25, "scoreStdev should parse");
    require(info->policy == 0.031, "prior should parse as policy");
    require(info->pv.size() == 3 && info->pv[1] == "Q16", "PV should parse");
    require(info->pvVisits.size() == 3 && info->pvVisits[2] == 40, "PV visits should parse");
    require(info->ownership.size() == 2 && info->ownership[1] == -0.2, "ownership should parse");

    const auto movesOwnershipAfterAlias = parseKataAnalyzeInfo(
        "info move C3 visits 40 winrate 0.51 scoreLead 0.2 prior 0.04 pv C3 "
        "ownership 0.3 -0.3 movesOwnership 0.1 -0.1");
    require(movesOwnershipAfterAlias.has_value(), "candidate with ownership aliases should parse");
    require(
        movesOwnershipAfterAlias->ownership.size() == 2 && movesOwnershipAfterAlias->ownership[0] == 0.1,
        "movesOwnership should replace compatible candidate ownership aliases");

    const auto aliasAfterMovesOwnership = parseKataAnalyzeInfo(
        "info move C3 visits 40 winrate 0.51 scoreLead 0.2 prior 0.04 pv C3 "
        "movesOwnership 0.1 -0.1 ownership 0.3 -0.3");
    require(aliasAfterMovesOwnership.has_value(), "candidate with ownership aliases in reverse order should parse");
    require(
        aliasAfterMovesOwnership->ownership.size() == 2 && aliasAfterMovesOwnership->ownership[0] == 0.1,
        "compatible ownership aliases should not overwrite movesOwnership");
}

void testAnalyzeInfoRejectsInvalidVisitCounts()
{
    const auto invalidCandidate = parseKataAnalyzeInfo(
        "info move D4 visits -1 winrate 0.52 scoreLead 1.5 prior 0.031 pv D4");
    require(!invalidCandidate.has_value(), "kata-analyze candidates with negative visits should be rejected");

    const auto fractionalCandidate = parseKataAnalyzeInfo(
        "info move D4 visits 12.5 winrate 0.52 scoreLead 1.5 prior 0.031 pv D4");
    require(!fractionalCandidate.has_value(), "kata-analyze candidates with fractional visits should be rejected");

    const auto update = parseKataAnalyzeLine(
        "info move D4 visits 12.5 winrate 0.52 scoreLead 1.5 prior 0.031 pv D4 "
        "info move C3 visits 80 winrate 0.51 scoreMean 0.3 policy 0.12 pv C3 pvVisits 80 40.5 -1 20 "
        "rootInfo visits -9 winrate 0.57 scoreLead 1.2");
    require(update.infos.size() == 1, "kata-analyze lines should skip candidates with invalid visits");
    require(update.infos.front().move == "C3", "valid candidates should remain after skipped invalid visits");
    require(update.infos.front().pvVisits.empty(), "invalid PV visits should discard the whole pvVisits field");
    require(update.rootInfo.has_value(), "rootInfo with other valid fields should still parse");
    require(!update.rootInfo->hasVisits, "rootInfo with negative visits should not report a usable visit count");
    require(update.rootInfo->hasWinrate, "rootInfo should report a usable winrate when present");
    require(update.rootInfo->hasScoreMean, "rootInfo should report a usable score when present");
    require(update.rootInfo->winrate == 0.57, "rootInfo winrate should remain usable when visits are invalid");

    const auto invalidRootOnly = parseKataAnalyzeLine("rootInfo visits -1");
    require(
        !invalidRootOnly.rootInfo.has_value(),
        "rootInfo with only invalid visits should not become a usable rootInfo block");
}

void testAnalyzeInfoRejectsNonFiniteNumericFields()
{
    const auto info = parseKataAnalyzeInfo(
        "info move D4 visits 12 winrate nan scoreLead inf scoreStdev -inf prior inf "
        "pv D4 ownership 0.1 nan 0.2");
    require(info.has_value(), "kata-analyze candidate with valid visits should parse despite invalid scalar fields");
    require(info->winrate == 0.0, "non-finite candidate winrate should be ignored");
    require(info->scoreMean == 0.0, "non-finite candidate score should be ignored");
    require(info->scoreStdev == 0.0, "non-finite candidate score stdev should be ignored");
    require(info->policy == 0.0, "non-finite candidate policy should be ignored");
    require(
        info->ownership.size() == 1 && info->ownership.front() == 0.1,
        "candidate ownership should stop before non-finite values");

    const auto update = parseKataAnalyzeLine(
        "ownership 0.2 inf -0.2 "
        "rootInfo visits 30 winrate nan scoreLead inf ownership 0.25 nan -0.25");
    require(update.ownership.size() == 1 && update.ownership.front() == 0.2, "top-level ownership should stay finite");
    require(update.rootInfo.has_value(), "rootInfo should parse from valid visits and finite ownership");
    require(update.rootInfo->hasVisits, "rootInfo visits should remain usable with invalid scalar fields");
    require(update.rootInfo->visits == 30, "rootInfo visits should parse while invalid scalar fields are ignored");
    require(!update.rootInfo->hasWinrate, "non-finite root winrate should not be marked usable");
    require(!update.rootInfo->hasScoreMean, "non-finite root score should not be marked usable");
    require(update.rootInfo->winrate == 0.0, "non-finite root winrate should be ignored");
    require(update.rootInfo->scoreMean == 0.0, "non-finite root score should be ignored");
    require(
        update.rootInfo->ownership.size() == 1 && update.rootInfo->ownership.front() == 0.25,
        "rootInfo ownership should stop before non-finite values");

    const auto invalidRootOnly = parseKataAnalyzeLine("rootInfo winrate nan scoreLead inf ownership nan");
    require(
        !invalidRootOnly.rootInfo.has_value(),
        "rootInfo with only non-finite numeric fields should not become usable");
}

void testAnalyzeInfoBatchLine()
{
    const auto infos = parseKataAnalyzeInfos(
        "info move D4 visits 123 winrate 0.52 scoreLead 1.5 prior 0.031 pv D4 Q16 pvVisits 123 80 "
        "info move Q16 visits 80 winrate 0.49 scoreMean -0.4 policy 0.12 pv Q16 D4 pvVisits 80 40 "
        "rootInfo visits 512 winrate 0.57 scoreLead 2.3");
    require(infos.size() == 2, "multi-candidate kata-analyze line should parse each info block");
    require(infos[0].move == "D4" && infos[0].pvVisits.size() == 2, "first info block should parse");
    require(infos[1].move == "Q16", "second info block move should parse");
    require(infos[1].scoreMean == -0.4, "second info block score should parse");
    require(infos[1].policy == 0.12, "second info block policy should parse");
    require(infos[1].pvVisits.size() == 2 && infos[1].pvVisits[1] == 40, "second info block PV visits should parse");

    const auto update = parseKataAnalyzeLine(
        "info move D4 visits 123 winrate 0.52 scoreLead 1.5 prior 0.031 pv D4 Q16 "
        "rootInfo visits 512 winrate 0.57 scoreLead 2.3 ownership 0.25 -0.25 0.1 -0.1");
    require(update.infos.size() == 1, "analysis line should retain candidate infos");
    require(update.infos.front().visits == 123, "rootInfo visits should not overwrite candidate visits");
    require(update.rootInfo.has_value(), "analysis line should parse rootInfo block");
    require(update.rootInfo->visits == 512, "rootInfo visits should parse");
    require(update.rootInfo->winrate == 0.57, "rootInfo winrate should parse");
    require(update.rootInfo->scoreMean == 2.3, "rootInfo scoreLead should parse as score mean");
    require(
        update.rootInfo->ownership.size() == 4 && update.rootInfo->ownership[1] == -0.25,
        "rootInfo ownership should parse");
    require(
        update.ownership.size() == 4 && update.ownership[2] == 0.1,
        "rootInfo-adjacent ownership should also populate top-level ownership");

    const auto topLevelOwnershipUpdate = parseKataAnalyzeLine(
        "ownership 0.4 -0.4 0.2 -0.2 "
        "info move C3 visits 24 winrate 0.51 scoreLead 0.3 prior 0.04 pv C3 "
        "rootInfo visits 64 winrate 0.55 scoreLead 1.1");
    require(
        topLevelOwnershipUpdate.ownership.size() == 4 && topLevelOwnershipUpdate.ownership[1] == -0.4,
        "top-level ownership block should parse even when it precedes rootInfo");
    require(topLevelOwnershipUpdate.infos.size() == 1, "top-level ownership should not hide candidate infos");
    require(topLevelOwnershipUpdate.rootInfo.has_value(), "top-level ownership should not hide rootInfo");

    const auto topLevelOwnershipPrecedence = parseKataAnalyzeLine(
        "ownership 0.4 -0.4 0.2 -0.2 "
        "rootInfo visits 64 winrate 0.55 scoreLead 1.1 ownership 0.25 -0.25 0.1 -0.1");
    require(
        topLevelOwnershipPrecedence.ownership.size() == 4 &&
            topLevelOwnershipPrecedence.ownership[0] == 0.4 &&
            topLevelOwnershipPrecedence.ownership[1] == -0.4,
        "standard top-level ownership should take precedence over later rootInfo ownership");
    require(
        topLevelOwnershipPrecedence.rootInfo.has_value() &&
            topLevelOwnershipPrecedence.rootInfo->ownership.size() == 4 &&
            topLevelOwnershipPrecedence.rootInfo->ownership[0] == 0.25,
        "rootInfo ownership should remain available as rootInfo data");
}

void testAnalyzeInfoExtensionFieldBoundaries()
{
    const auto update = parseKataAnalyzeLine(
        "info move D4 visits 123 winrate 0.52 scoreLead 1.5 prior 0.031 "
        "pv D4 Q16 pvEdgeVisits 123 80 edgeVisits 77 utility -0.1 utilityLcb -0.2 "
        "scoreSelfplay 0.4 noResultValue 0.01 isSymmetryOf C3 "
        "movesOwnershipStdev 0.01 0.02 ownershipStdev 0.03 0.04 "
        "movesOwnership 0.1 -0.2 pvVisits 123 80 "
        "rootInfo visits 512 winrate 0.57 scoreLead 2.3 "
        "rawStWrError 0.01 rawStScoreError 0.2 rawVarTimeLeft 42 ownership 0.25 -0.25");
    require(update.infos.size() == 1, "extension fields should not hide candidate infos");

    const KataAnalyzeInfo& info = update.infos.front();
    require(info.move == "D4", "extension-field candidate move should parse");
    require(info.policy == 0.031, "extension-field candidate policy should parse");
    require(
        info.pv.size() == 2 && info.pv[0] == "D4" && info.pv[1] == "Q16",
        "known extension fields should terminate PV tokens");
    require(
        info.pvVisits.size() == 2 && info.pvVisits[0] == 123 && info.pvVisits[1] == 80,
        "PV visits should parse after skipped extension fields");
    require(
        info.ownership.size() == 2 && info.ownership[1] == -0.2,
        "movesOwnership should parse after ownership stdev extension fields");

    require(update.rootInfo.has_value(), "extension fields should not hide rootInfo");
    require(update.rootInfo->visits == 512, "rootInfo visits should parse after extension fields");
    require(
        update.ownership.size() == 2 && update.ownership[0] == 0.25,
        "rootInfo ownership should parse after raw statistical extension fields");
}

void testAnalyzeInfoCaseInsensitiveKeys()
{
    const auto update = parseKataAnalyzeLine(
        "INFO MOVE D4 VISITS 123 WINRATE 0.52 SCORELEAD 1.5 SCORESTDEV 8.25 PRIOR 0.031 "
        "PV D4 Q16 PVVISITS 123 80 MOVESOWNERSHIP 0.1 -0.2 "
        "Info Move Q16 Visits 80 Winrate 0.49 ScoreMean -0.4 Policy 0.12 Pv Q16 D4 PvVisits 80 40 "
        "RootInfo Visits 512 Winrate 0.57 ScoreLead 2.3 Ownership 0.25 -0.25");
    require(update.infos.size() == 2, "mixed-case kata-analyze line should parse candidate blocks");
    require(update.infos[0].move == "D4", "mixed-case move key should parse");
    require(update.infos[0].visits == 123, "mixed-case visits key should parse");
    require(update.infos[0].scoreMean == 1.5, "mixed-case scoreLead key should parse");
    require(update.infos[0].scoreStdev == 8.25, "mixed-case scoreStdev key should parse");
    require(update.infos[0].policy == 0.031, "mixed-case prior key should parse");
    require(
        update.infos[0].pv.size() == 2 && update.infos[0].pvVisits.size() == 2,
        "mixed-case pvVisits key should terminate PV and parse visit counts");
    require(
        update.infos[0].ownership.size() == 2 && update.infos[0].ownership[1] == -0.2,
        "mixed-case movesOwnership key should parse ownership values");
    require(update.infos[1].move == "Q16", "mixed-case second info block should parse");
    require(update.rootInfo.has_value(), "mixed-case rootInfo block should parse");
    require(update.rootInfo->visits == 512, "mixed-case rootInfo visits should parse");
    require(update.rootInfo->scoreMean == 2.3, "mixed-case rootInfo scoreLead should parse");
    require(
        update.rootInfo->ownership.size() == 2 && update.rootInfo->ownership[1] == -0.25,
        "mixed-case rootInfo ownership should parse");
    require(
        update.ownership.size() == 2 && update.ownership[0] == 0.25,
        "mixed-case ownership should also populate top-level ownership");
}

void testGtpPoints()
{
    const BoardSize size = BoardSize::square(19);
    require(isGtpBoardSizeSupported(size), "19x19 should be supported by GTP coordinate formatting");
    require(formatPointForGtp(Point{3, 15}, size) == "D4", "point should format to GTP coordinates");
    const auto point = parseGtpPoint("D4", size);
    require(point.has_value() && *point == Point{3, 15}, "GTP coordinate should parse");
    const auto afterI = parseGtpPoint("J10", size);
    require(afterI.has_value() && *afterI == Point{8, 9}, "GTP parser should skip I column");
    require(!parseGtpPoint("I10", size).has_value(), "GTP parser should reject omitted I column");
    require(!parseGtpPoint("[10", size).has_value(), "GTP parser should reject non-coordinate columns");

    const BoardSize rectangular{9, 13};
    require(formatPointForGtp(Point{7, 12}, rectangular) == "H1", "rectangular point should use board height");
    require(formatPointForGtp(Point{8, 0}, rectangular) == "J13", "rectangular point should skip I column");
    const auto rectangularPoint = parseGtpPoint("H1", rectangular);
    require(
        rectangularPoint.has_value() && *rectangularPoint == Point{7, 12},
        "rectangular GTP coordinate should parse with board height");
    require(!parseGtpPoint("J14", rectangular).has_value(), "rectangular GTP parser should reject outside rows");

    const BoardSize maxWidth{25, 9};
    require(isGtpBoardSizeSupported(maxWidth), "25-column boards should fit GTP coordinates");
    require(formatPointForGtp(Point{24, 8}, maxWidth) == "Z1", "last supported GTP column should be Z");

    const BoardSize tooWide{26, 9};
    require(!isGtpBoardSizeSupported(tooWide), "26-column boards should be rejected for GTP sync");
    require(formatPointForGtp(Point{25, 8}, tooWide).empty(), "unsupported GTP columns should not format");
    const auto tooWideStillRepresentable = parseGtpPoint("Z1", tooWide);
    require(
        tooWideStillRepresentable.has_value() && *tooWideStillRepresentable == Point{24, 8},
        "wide Analysis JSON boards should still parse representable GTP coordinates");

    const BoardSize invalid{0, 19};
    require(!isGtpBoardSizeSupported(invalid), "invalid boards should be rejected for GTP sync");
    require(!parseGtpPoint("A1", invalid).has_value(), "invalid GTP board sizes should reject coordinates");
}

void testKataGoConfig()
{
    KataGoConfig config;
    config.executable = "/opt/Kata Go/katago";
    config.model = "/models/main model.bin.gz";
    config.gtpConfig = "/configs/main gtp.cfg";
    config.analysisConfig = "/configs/analysis gpu.cfg";
    config.extraArgs = {
        "-override-config",
        "logDir=/tmp/kata logs",
        "--override-config",
        "numSearchThreads=8",
    };

    require(config.hasGtpMinimum(), "GTP config should be complete");
    require(config.hasAnalysisMinimum(), "analysis config should be complete");
    const std::vector<std::string> gtpArgs = config.gtpArguments();
    const std::vector<std::string> expectedGtpArgs = {
        "gtp",
        "-model",
        "/models/main model.bin.gz",
        "-config",
        "/configs/main gtp.cfg",
        "-override-config",
        "logDir=/tmp/kata logs",
        "--override-config",
        "numSearchThreads=8",
    };
    require(gtpArgs == expectedGtpArgs, "GTP args should preserve paths with spaces and extra args");
    const std::vector<std::string> analysisArgs = config.analysisArguments();
    const std::vector<std::string> expectedAnalysisArgs = {
        "analysis",
        "-model",
        "/models/main model.bin.gz",
        "-config",
        "/configs/analysis gpu.cfg",
        "-quit-without-waiting",
        "-override-config",
        "logDir=/tmp/kata logs",
        "--override-config",
        "numSearchThreads=8",
    };
    require(
        analysisArgs == expectedAnalysisArgs,
        "analysis args should preserve paths with spaces, quit flag, and extra args");

    KataGoConfig gtpOnly = config;
    gtpOnly.analysisConfig.clear();
    require(gtpOnly.hasGtpMinimum(), "GTP startup should not require an analysis config");
    require(!gtpOnly.hasAnalysisMinimum(), "analysis startup should require an analysis config");

    KataGoConfig analysisOnly = config;
    analysisOnly.gtpConfig.clear();
    require(!analysisOnly.hasGtpMinimum(), "GTP startup should require a GTP config");
    require(analysisOnly.hasAnalysisMinimum(), "analysis startup should not require a GTP config");

    KataGoConfig missingModel = config;
    missingModel.model.clear();
    require(!missingModel.hasGtpMinimum(), "GTP startup should require a model");
    require(!missingModel.hasAnalysisMinimum(), "analysis startup should require a model");

    KataGoConfig whitespaceExecutable = config;
    whitespaceExecutable.executable = " \t\n ";
    require(!whitespaceExecutable.hasGtpMinimum(), "GTP startup should reject whitespace-only executable paths");
    require(
        !whitespaceExecutable.hasAnalysisMinimum(),
        "analysis startup should reject whitespace-only executable paths");

    KataGoConfig whitespaceModel = config;
    whitespaceModel.model = " \t ";
    require(!whitespaceModel.hasGtpMinimum(), "GTP startup should reject whitespace-only model paths");
    require(!whitespaceModel.hasAnalysisMinimum(), "analysis startup should reject whitespace-only model paths");

    KataGoConfig whitespaceGtpConfig = config;
    whitespaceGtpConfig.gtpConfig = "\n\t";
    require(!whitespaceGtpConfig.hasGtpMinimum(), "GTP startup should reject whitespace-only GTP config paths");
    require(
        whitespaceGtpConfig.hasAnalysisMinimum(),
        "analysis startup should not require a GTP config even when the GTP config field is whitespace");

    KataGoConfig whitespaceAnalysisConfig = config;
    whitespaceAnalysisConfig.analysisConfig = "\n\t";
    require(
        whitespaceAnalysisConfig.hasGtpMinimum(),
        "GTP startup should not require an analysis config even when the analysis config field is whitespace");
    require(
        !whitespaceAnalysisConfig.hasAnalysisMinimum(),
        "analysis startup should reject whitespace-only analysis config paths");
}

void testProcessDiagnosticCommandLine()
{
    const QString commandLine = commandLineForDiagnostics(
        "/opt/Kata Go/katago",
        QStringList{
            "gtp",
            "-model",
            "model \"fast\".bin.gz",
            R"(C:\katago\cfg file.cfg)",
            "",
        });
    require(
        commandLine.toStdString() ==
            R"("/opt/Kata Go/katago" gtp -model "model \"fast\".bin.gz" "C:\\katago\\cfg file.cfg" "")",
        "process diagnostic command line should quote spaces, quotes, backslashes, and empty arguments");
}

}  // namespace

int main()
{
    try {
        testCommandFormatting();
        testResponseParser();
        testResponseParserKeepsNumericPayloadsUnnumbered();
        testErrorResponseParser();
        testResponseParserRejectsOverflowIds();
        testCommandQueue();
        testRawProtocolLine();
        testCapabilities();
        testAnalyzeInfo();
        testAnalyzeInfoRejectsInvalidVisitCounts();
        testAnalyzeInfoRejectsNonFiniteNumericFields();
        testAnalyzeInfoBatchLine();
        testAnalyzeInfoExtensionFieldBoundaries();
        testAnalyzeInfoCaseInsensitiveKeys();
        testGtpPoints();
        testKataGoConfig();
        testProcessDiagnosticCommandLine();
    } catch (const std::exception& error) {
        std::cerr << "engine_protocol_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "engine_protocol_tests passed\n";
    return EXIT_SUCCESS;
}
