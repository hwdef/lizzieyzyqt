#include "AnalysisStore.h"
#include "AnalysisStreamGate.h"
#include "BatchAnalysisRun.h"
#include "BatchAnalysisTracker.h"
#include "BatchAnalysis.h"
#include "EngineCommandDispatcher.h"
#include "EngineCommandPlanner.h"
#include "EngineAutomation.h"
#include "EngineManager.h"
#include "GameEditor.h"
#include "GameSetup.h"
#include "PositionRequestGuard.h"
#include "Sgf.h"

#include <QString>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool containsPoint(const std::vector<lizzie::core::Point>& points, lizzie::core::Point point)
{
    return std::ranges::find(points, point) != points.end();
}

bool containsInitialStone(
    const std::vector<lizzie::engine::InitialStone>& stones,
    lizzie::core::Color color,
    lizzie::core::Point point)
{
    return std::ranges::find_if(stones, [color, point](const lizzie::engine::InitialStone& stone) {
        return stone.color == color && stone.point == point;
    }) != stones.end();
}

bool containsCommandName(const std::vector<lizzie::engine::GtpCommand>& commands, std::string_view name)
{
    return std::ranges::any_of(commands, [name](const lizzie::engine::GtpCommand& command) {
        return command.name == name;
    });
}

bool containsWarning(const std::vector<std::string>& warnings, std::string_view fragment)
{
    return std::ranges::any_of(warnings, [fragment](const std::string& warning) {
        return warning.find(fragment) != std::string::npos;
    });
}

void testSelfPlayContinuesAfterNormalMove()
{
    const lizzie::app::EngineAutomationPlan plan = lizzie::app::planAfterEngineMove(
        lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{3, 3}),
        lizzie::app::EngineAutomationState{.selfPlayActive = true});
    require(plan.requestSelfPlayMove, "self-play should request another move after a normal engine move");
    require(!plan.stopSelfPlay, "self-play should not stop after a normal engine move");
    require(!plan.requestEngineGameMove, "self-play should not request engine-game continuation");
}

void testEngineGameContinuesAfterNormalMove()
{
    const lizzie::app::EngineAutomationPlan plan = lizzie::app::planAfterEngineMove(
        lizzie::core::Move::pass(lizzie::core::Color::White),
        lizzie::app::EngineAutomationState{.engineGameActive = true});
    require(plan.requestEngineGameMove, "engine game should continue after a non-terminal engine move");
    require(!plan.stopEngineGame, "engine game should not stop after a non-terminal engine move");
    require(!plan.requestSelfPlayMove, "engine game should not request self-play continuation");
}

void testResignStopsSelfPlay()
{
    const lizzie::app::EngineAutomationPlan plan = lizzie::app::planAfterEngineMove(
        lizzie::core::Move::resign(lizzie::core::Color::Black),
        lizzie::app::EngineAutomationState{.selfPlayActive = true});
    require(plan.stopSelfPlay, "engine resign should stop self-play");
    require(!plan.requestSelfPlayMove, "engine resign should not request another self-play move");
    require(!plan.requestEngineGameMove, "engine resign should not request engine-game continuation");
}

void testResignStopsHumanVsAi()
{
    const lizzie::app::EngineAutomationPlan plan = lizzie::app::planAfterEngineMove(
        lizzie::core::Move::resign(lizzie::core::Color::Black),
        lizzie::app::EngineAutomationState{.humanVsAiActive = true});
    require(plan.stopHumanVsAi, "engine resign should stop human-vs-AI mode");
    require(!plan.requestSelfPlayMove, "engine resign should not request self-play continuation");
    require(!plan.requestEngineGameMove, "engine resign should not request engine-game continuation");
}

void testResignStopsEngineGame()
{
    const lizzie::app::EngineAutomationPlan plan = lizzie::app::planAfterEngineMove(
        lizzie::core::Move::resign(lizzie::core::Color::White),
        lizzie::app::EngineAutomationState{.engineGameActive = true});
    require(plan.stopEngineGame, "engine resign should stop engine game");
    require(!plan.requestEngineGameMove, "engine resign should not request another engine-game move");
    require(!plan.requestSelfPlayMove, "engine resign should not request self-play continuation");
}

void testDoublePassStopsSelfPlay()
{
    const lizzie::app::EngineAutomationPlan plan = lizzie::app::planAfterEngineMove(
        lizzie::core::Move::pass(lizzie::core::Color::White),
        lizzie::app::EngineAutomationState{.selfPlayActive = true, .previousMoveWasPass = true});
    require(plan.stopSelfPlay, "two consecutive passes should stop self-play");
    require(!plan.requestSelfPlayMove, "two consecutive passes should not request another self-play move");
}

void testDoublePassStopsHumanVsAi()
{
    const lizzie::app::EngineAutomationPlan plan = lizzie::app::planAfterEngineMove(
        lizzie::core::Move::pass(lizzie::core::Color::White),
        lizzie::app::EngineAutomationState{.humanVsAiActive = true, .previousMoveWasPass = true});
    require(plan.stopHumanVsAi, "two consecutive passes should stop human-vs-AI mode");
    require(!plan.requestSelfPlayMove, "two consecutive passes should not request self-play continuation");
    require(!plan.requestEngineGameMove, "two consecutive passes should not request engine-game continuation");
}

void testHumanMovePlansHumanVsAiReply()
{
    const lizzie::app::HumanMoveAutomationPlan normalMove = lizzie::app::planAfterHumanMove(
        lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{3, 3}),
        lizzie::app::EngineAutomationState{.humanVsAiActive = true});
    require(normalMove.requestHumanVsAiReply, "human-vs-AI should request an AI reply after a normal human move");
    require(!normalMove.stopHumanVsAi, "human-vs-AI should remain active after a normal human move");

    const lizzie::app::HumanMoveAutomationPlan firstPass = lizzie::app::planAfterHumanMove(
        lizzie::core::Move::pass(lizzie::core::Color::Black),
        lizzie::app::EngineAutomationState{.humanVsAiActive = true});
    require(firstPass.requestHumanVsAiReply, "human-vs-AI should request an AI reply after the first pass");
    require(!firstPass.stopHumanVsAi, "human-vs-AI should not stop after a single pass");

    const lizzie::app::HumanMoveAutomationPlan secondPass = lizzie::app::planAfterHumanMove(
        lizzie::core::Move::pass(lizzie::core::Color::White),
        lizzie::app::EngineAutomationState{.humanVsAiActive = true, .previousMoveWasPass = true});
    require(secondPass.stopHumanVsAi, "human-vs-AI should stop after a human second consecutive pass");
    require(!secondPass.requestHumanVsAiReply, "human-vs-AI should not request an AI reply after a terminal pass");

    const lizzie::app::HumanMoveAutomationPlan resign = lizzie::app::planAfterHumanMove(
        lizzie::core::Move::resign(lizzie::core::Color::White),
        lizzie::app::EngineAutomationState{.humanVsAiActive = true});
    require(resign.stopHumanVsAi, "human-vs-AI should stop after a human resign");
    require(!resign.requestHumanVsAiReply, "human-vs-AI should not request an AI reply after a human resign");
}

void testDoublePassStopsEngineGame()
{
    const lizzie::app::EngineAutomationPlan plan = lizzie::app::planAfterEngineMove(
        lizzie::core::Move::pass(lizzie::core::Color::Black),
        lizzie::app::EngineAutomationState{.engineGameActive = true, .previousMoveWasPass = true});
    require(plan.stopEngineGame, "two consecutive passes should stop engine game");
    require(!plan.requestEngineGameMove, "two consecutive passes should not request another engine-game move");
}

void testHumanVsAiToggleStopsConflictingAutomatedModes()
{
    const lizzie::app::AutomatedPlayTogglePlan plan = lizzie::app::planToggleHumanVsAi(
        lizzie::app::EngineAutomationState{.selfPlayActive = true, .engineGameActive = true});

    require(plan.startHumanVsAi, "human-vs-AI toggle should start human-vs-AI when inactive");
    require(plan.stopSelfPlay, "human-vs-AI toggle should stop self-play");
    require(plan.stopEngineGame, "human-vs-AI toggle should stop engine game");
    require(!plan.requestSelfPlayMove, "human-vs-AI toggle should not request a self-play move");
    require(!plan.requestEngineGameMove, "human-vs-AI toggle should not request an engine-game move");
}

void testSelfPlayToggleStartsMoveAndStopsConflicts()
{
    const lizzie::app::AutomatedPlayTogglePlan plan = lizzie::app::planToggleSelfPlay(
        lizzie::app::EngineAutomationState{.engineGameActive = true, .humanVsAiActive = true});

    require(plan.startSelfPlay, "self-play toggle should start self-play when inactive");
    require(plan.requestSelfPlayMove, "self-play toggle should request the first AI move");
    require(plan.stopHumanVsAi, "self-play toggle should stop human-vs-AI");
    require(plan.stopEngineGame, "self-play toggle should stop engine game");
    require(!plan.requestEngineGameMove, "self-play toggle should not request engine-game continuation");
}

void testEngineGameToggleStopsAnalysisAndConflictingModes()
{
    const lizzie::app::AutomatedPlayTogglePlan plan = lizzie::app::planToggleEngineGame(
        lizzie::app::EngineAutomationState{.selfPlayActive = true, .humanVsAiActive = true});

    require(plan.startEngineGame, "engine-game toggle should start engine game when inactive");
    require(plan.requestEngineGameMove, "engine-game toggle should request the first engine-game move");
    require(plan.stopSelfPlay, "engine-game toggle should stop self-play");
    require(plan.stopHumanVsAi, "engine-game toggle should stop human-vs-AI");
    require(plan.stopRealtimeAnalysis, "engine-game toggle should stop realtime analysis");
    require(plan.stopCompareAnalysis, "engine-game toggle should stop compare analysis");
    require(plan.clearCompareMoveRequest, "engine-game toggle should clear stale compare move requests");
}

void testAutomatedModeTogglesStopWhenAlreadyActive()
{
    require(
        lizzie::app::planToggleHumanVsAi(lizzie::app::EngineAutomationState{.humanVsAiActive = true}).stopHumanVsAi,
        "active human-vs-AI toggle should stop human-vs-AI");
    require(
        lizzie::app::planToggleSelfPlay(lizzie::app::EngineAutomationState{.selfPlayActive = true}).stopSelfPlay,
        "active self-play toggle should stop self-play");
    require(
        lizzie::app::planToggleEngineGame(lizzie::app::EngineAutomationState{.engineGameActive = true}).stopEngineGame,
        "active engine-game toggle should stop engine game");
}

void testAnalysisStreamGateWaitsForSyncResponses()
{
    lizzie::app::AnalysisStreamGate gate;
    gate.beginSync();
    gate.recordSyncCommand(10);
    gate.recordSyncCommand(12);

    require(gate.isPending(), "analysis stream gate should be pending during sync");
    require(!gate.acceptsAnalysis(), "analysis stream gate should reject stale analysis while syncing");
    require(
        gate.handleSyncResponse(99, true) == lizzie::app::AnalysisStreamGateEvent::Ignored,
        "untracked sync response should be ignored");
    require(gate.isPending(), "untracked sync response should not activate stream");
    require(
        gate.handleSyncResponse(10, true) == lizzie::app::AnalysisStreamGateEvent::Waiting,
        "partial sync response should not activate stream");
    require(!gate.acceptsAnalysis(), "partial sync should still reject analysis");
    require(
        gate.handleSyncResponse(12, true) == lizzie::app::AnalysisStreamGateEvent::Activated,
        "last sync response should activate stream");
    require(gate.isActive(), "analysis stream gate should become active");
    require(gate.acceptsAnalysis(), "active analysis stream gate should accept analysis");
}

void testAnalysisStreamGateStopsOnSyncFailure()
{
    lizzie::app::AnalysisStreamGate gate;
    gate.beginSync();
    gate.recordSyncCommand(10);
    gate.recordSyncCommand(11);

    require(
        gate.handleSyncResponse(10, false) == lizzie::app::AnalysisStreamGateEvent::Failed,
        "failed sync response should fail the analysis stream gate");
    require(!gate.isPending(), "failed sync response should clear pending state");
    require(!gate.isActive(), "failed sync response should not leave stream active");
    require(!gate.acceptsAnalysis(), "failed sync response should reject analysis");
    require(
        gate.handleSyncResponse(11, true) == lizzie::app::AnalysisStreamGateEvent::Ignored,
        "responses after sync failure should be ignored");
}

void testEngineCommandDispatcherRecordsSyncCommandIds()
{
    lizzie::app::AnalysisStreamGate gate;
    gate.beginSync();

    std::vector<std::string> sentCommands;
    int nextCommandId = 40;
    const std::vector<lizzie::engine::GtpCommand> commands{
        lizzie::engine::GtpCommand{std::nullopt, "clear_board", {}},
        lizzie::engine::GtpCommand{std::nullopt, "komi", {"6.5"}},
    };

    const lizzie::app::EngineCommandDispatchResult result = lizzie::app::dispatchGtpCommands(
        commands,
        [&](const lizzie::engine::GtpCommand& command) {
            sentCommands.push_back(command.toLine());
            return nextCommandId++;
        },
        &gate);

    require(result.ok(), "dispatcher should accept successful command sends");
    require(result.commandIds.size() == 2, "dispatcher should report sent command ids");
    require(result.commandIds[0] == 40 && result.commandIds[1] == 41, "dispatcher should preserve command ids");
    require(sentCommands.size() == 2, "dispatcher should send each command");
    require(sentCommands[0] == "clear_board\n", "dispatcher should send the first command");
    require(sentCommands[1] == "komi 6.5\n", "dispatcher should send the second command");
    require(
        gate.handleSyncResponse(40, true) == lizzie::app::AnalysisStreamGateEvent::Waiting,
        "dispatcher-recorded first sync response should keep the gate waiting");
    require(
        gate.handleSyncResponse(41, true) == lizzie::app::AnalysisStreamGateEvent::Activated,
        "dispatcher-recorded last sync response should activate the gate");
}

void testEngineCommandDispatcherSkipsRejectedSyncIds()
{
    lizzie::app::AnalysisStreamGate gate;
    gate.beginSync();

    const std::vector<lizzie::engine::GtpCommand> commands{
        lizzie::engine::GtpCommand{std::nullopt, "clear_board", {}},
        lizzie::engine::GtpCommand{std::nullopt, "komi", {"7.5"}},
    };
    int sendCount = 0;
    const lizzie::app::EngineCommandDispatchResult result = lizzie::app::dispatchGtpCommands(
        commands,
        [&](const lizzie::engine::GtpCommand&) {
            ++sendCount;
            return sendCount == 1 ? 50 : -1;
        },
        &gate);

    require(!result.ok(), "dispatcher should flag rejected command sends");
    require(result.commandIds.size() == 1 && result.commandIds.front() == 50, "dispatcher should only record valid ids");
    require(!gate.isPending(), "rejected sync sends should stop the analysis stream gate");
    require(!gate.isActive(), "rejected sync sends should not leave the gate active");
    require(
        gate.handleSyncResponse(50, true) == lizzie::app::AnalysisStreamGateEvent::Ignored,
        "responses after a rejected sync send should not activate the gate");
}

void testEngineCommandDispatcherRejectsEmptyCommands()
{
    lizzie::app::AnalysisStreamGate gate;
    gate.beginSync();

    int sendCount = 0;
    const lizzie::app::EngineCommandDispatchResult result = lizzie::app::dispatchGtpCommands(
        {
            lizzie::engine::GtpCommand{},
            lizzie::engine::GtpCommand{std::nullopt, "kata-analyze clear_board", {}},
            lizzie::engine::GtpCommand{std::nullopt, "kata-stop\nclear_board", {}},
        },
        [&](const lizzie::engine::GtpCommand&) {
            ++sendCount;
            return 60;
        },
        &gate);

    require(!result.ok(), "dispatcher should reject invalid GTP command names");
    require(sendCount == 0, "dispatcher should not send invalid GTP command names");
    require(result.commandIds.empty(), "dispatcher should not record ids for invalid GTP command names");
    require(!gate.isPending(), "invalid sync command names should stop the analysis stream gate");
    require(!gate.isActive(), "invalid sync command names should not leave the gate active");
    require(
        gate.handleSyncResponse(60, true) == lizzie::app::AnalysisStreamGateEvent::Ignored,
        "dispatcher should not record invalid command names in the sync gate");
}

void testEngineCommandDispatcherStopsGateWhenSenderUnavailable()
{
    lizzie::app::AnalysisStreamGate gate;
    gate.beginSync();
    gate.recordSyncCommand(70);

    const lizzie::app::EngineCommandDispatchResult result = lizzie::app::dispatchGtpCommands(
        {lizzie::engine::GtpCommand{std::nullopt, "clear_board", {}}},
        lizzie::app::GtpCommandSender{},
        &gate);

    require(!result.ok(), "dispatcher should reject missing command senders");
    require(!result.engineRunning, "missing command sender should report the engine unavailable");
    require(result.commandIds.empty(), "missing command sender should not report command ids");
    require(!gate.isPending(), "missing command sender should stop the analysis stream gate");
    require(!gate.isActive(), "missing command sender should not leave the gate active");
    require(
        gate.handleSyncResponse(70, true) == lizzie::app::AnalysisStreamGateEvent::Ignored,
        "responses after a missing sender should not activate the gate");
}

void testEngineCommandDispatcherParsesConsoleCommands()
{
    std::vector<std::string> sentCommands;
    int nextCommandId = 80;
    const bool accepted = lizzie::app::dispatchConsoleCommand(
        QStringLiteral("  kata-set-param analysisWideRootNoise \"0.25\"  "),
        [&](const lizzie::engine::GtpCommand& command) {
            sentCommands.push_back(command.toLine());
            return nextCommandId++;
        });

    require(accepted, "console dispatcher should accept a valid raw GTP command");
    require(sentCommands.size() == 1, "console dispatcher should send one GTP command");
    require(
        sentCommands.front() == "kata-set-param analysisWideRootNoise 0.25\n",
        "console dispatcher should parse command names and quoted arguments");

    std::vector<lizzie::engine::GtpCommand> complexCommands;
    const bool complexAccepted = lizzie::app::dispatchConsoleCommand(
        QStringLiteral(R"(custom-command "" "path with spaces" quoted"""value)"),
        [&](const lizzie::engine::GtpCommand& command) {
            complexCommands.push_back(command);
            return nextCommandId++;
        });
    require(complexAccepted, "console dispatcher should accept quoted empty raw GTP arguments");
    require(complexCommands.size() == 1, "console dispatcher should send one complex raw GTP command");
    require(complexCommands.front().name == "custom-command", "console dispatcher should preserve the command name");
    require(
        complexCommands.front().arguments == std::vector<std::string>({"", "path with spaces", "quoted\"value"}),
        "console dispatcher should preserve empty, spaced, and quoted raw GTP arguments");

    bool senderCalled = false;
    const bool emptyAccepted = lizzie::app::dispatchConsoleCommand(
        QStringLiteral("   "),
        [&](const lizzie::engine::GtpCommand&) {
            senderCalled = true;
            return 81;
        });
    require(!emptyAccepted, "console dispatcher should reject blank commands");
    require(!senderCalled, "console dispatcher should not call the sender for blank commands");
    require(
        !lizzie::app::dispatchConsoleCommand(QStringLiteral("name"), lizzie::app::GtpCommandSender{}),
        "console dispatcher should reject a missing sender");

    bool invalidNameSenderCalled = false;
    const bool invalidQuotedNameAccepted = lizzie::app::dispatchConsoleCommand(
        QStringLiteral(R"("kata-analyze clear_board" 50)"),
        [&](const lizzie::engine::GtpCommand&) {
            invalidNameSenderCalled = true;
            return 82;
        });
    require(!invalidQuotedNameAccepted, "console dispatcher should reject command names containing spaces");
    require(!invalidNameSenderCalled, "console dispatcher should not send invalid quoted command names");

    bool multilineNameSenderCalled = false;
    const bool multilineNameAccepted = lizzie::app::dispatchConsoleCommand(
        QStringLiteral("\"kata-analyze\nclear_board\" 50"),
        [&](const lizzie::engine::GtpCommand&) {
            multilineNameSenderCalled = true;
            return 83;
        });
    require(!multilineNameAccepted, "console dispatcher should reject command names containing newlines");
    require(!multilineNameSenderCalled, "console dispatcher should not send multiline command names");

    bool unbalancedQuoteSenderCalled = false;
    const bool unbalancedQuoteAccepted = lizzie::app::dispatchConsoleCommand(
        QStringLiteral(R"(kata-set-param "maxVisits 100)"),
        [&](const lizzie::engine::GtpCommand&) {
            unbalancedQuoteSenderCalled = true;
            return 84;
        });
    require(!unbalancedQuoteAccepted, "console dispatcher should reject commands with unbalanced quotes");
    require(!unbalancedQuoteSenderCalled, "console dispatcher should not send commands with unbalanced quotes");
}

void testPositionRequestGuardConsumesOnlyMatchingPosition()
{
    lizzie::app::PositionRequestGuard guard;
    require(!guard.consumeIfMatches("v1|old"), "empty position request guard should reject responses");

    guard.start("v1|node:3");
    require(guard.hasPendingRequest(), "position request guard should track pending request");
    require(!guard.consumeIfMatches("v1|node:4"), "position request guard should reject stale responses");
    require(!guard.hasPendingRequest(), "stale response should consume the pending request");

    guard.start("v1|node:5");
    require(guard.consumeIfMatches("v1|node:5"), "position request guard should accept matching responses");
    require(!guard.hasPendingRequest(), "matching response should clear the pending request");

    guard.start("v1|node:6");
    guard.clear();
    require(!guard.hasPendingRequest(), "position request guard should clear cancelled requests");
}

void testAnalysisStoreWritesSnapshotAndClearsError()
{
    lizzie::core::GameModel model(lizzie::core::BoardSize::square(9));
    const lizzie::core::NodeId nodeId =
        model.addMove(lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{2, 2}));
    model.node(nodeId).analysisError = "old failure";

    lizzie::core::AnalysisSnapshot snapshot;
    snapshot.rootVisits = 42;
    snapshot.rootWinrate = 0.63;

    lizzie::app::AnalysisStore store(&model);
    require(store.setSnapshot(nodeId, snapshot), "analysis store should write a snapshot to an existing node");
    require(model.node(nodeId).analysis.has_value(), "analysis store should populate node analysis");
    require(model.node(nodeId).analysis->rootVisits == 42, "analysis store should preserve snapshot visits");
    require(model.node(nodeId).analysisError.empty(), "analysis store should clear stale node errors on success");
    require(!store.setSnapshot(999, snapshot), "analysis store should reject missing nodes");
}

void testAnalysisStoreAppliesBatchResponseWithPositionKey()
{
    lizzie::core::GameModel model(lizzie::core::BoardSize::square(9));
    model.gameInfo().rules = "Japanese";
    const lizzie::core::NodeId nodeId =
        model.addMove(lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{3, 3}));
    const std::string positionKey = lizzie::engine::analysisPositionKey(model, nodeId);
    model.node(nodeId).analysisError = "old batch failure";

    lizzie::engine::AnalysisResponse response;
    response.id = lizzie::engine::analysisRequestId(nodeId);
    response.rootVisits = 77;
    response.rootWinrate = 0.58;
    response.rootScoreMean = 1.25;

    lizzie::app::AnalysisStore store(&model);
    const lizzie::app::AnalysisStoreUpdateResult stale =
        store.applyResponse(response, std::string_view("stale-key"));
    require(
        stale.status == lizzie::app::AnalysisStoreUpdateStatus::StalePosition,
        "analysis store should reject stale batch responses");
    require(!model.node(nodeId).analysis.has_value(), "stale batch responses should not populate analysis");
    require(
        model.node(nodeId).analysisError == "old batch failure",
        "stale batch responses should preserve existing analysis errors");

    const lizzie::app::AnalysisStoreUpdateResult applied =
        store.applyResponse(response, std::string_view(positionKey));
    require(applied.applied(), "analysis store should apply a matching batch response");
    require(applied.nodeId == nodeId, "analysis store should report the updated node");
    require(model.node(nodeId).analysis.has_value(), "matching batch response should populate analysis");
    require(model.node(nodeId).analysis->rootVisits == 77, "matching batch response should preserve visits");
    require(model.node(nodeId).analysisError.empty(), "matching batch response should clear stale errors");

    response.ownership = {0.1, -0.1};
    response.moveInfos = {
        lizzie::core::MoveCandidate{
            lizzie::core::Move::play(lizzie::core::Color::White, lizzie::core::Point{4, 4}),
            12,
            0.49,
            -0.4}};
    response.moveInfos.front().ownership = {0.2, -0.2};
    response.rootVisits = 78;
    const lizzie::app::AnalysisStoreUpdateResult incompleteOwnershipApplied =
        store.applyResponse(response, std::string_view(positionKey));
    require(
        incompleteOwnershipApplied.applied(),
        "analysis store should apply responses with incomplete ownership after clearing ownership");
    require(
        model.node(nodeId).analysis->ownership.empty(),
        "analysis store should discard incomplete root ownership");
    require(
        model.node(nodeId).analysis->candidates.front().ownership.empty(),
        "analysis store should discard incomplete candidate ownership");

    response.ownership.clear();
    response.moveInfos.clear();
    std::string legacyRawRuleKey = positionKey;
    const std::size_t rulePosition = legacyRawRuleKey.find("|RU:japanese|");
    require(rulePosition != std::string::npos, "test should build a canonical Japanese rules key");
    legacyRawRuleKey.replace(rulePosition, std::string("|RU:japanese|").size(), "|RU:jp|");
    response.rootVisits = 79;
    const lizzie::app::AnalysisStoreUpdateResult rawRuleApplied =
        store.applyResponse(response, std::string_view(legacyRawRuleKey));
    require(rawRuleApplied.applied(), "analysis store should accept legacy raw-rule position keys");
    require(model.node(nodeId).analysis->rootVisits == 79, "legacy raw-rule key should update analysis");

    std::string legacyNoPlayerKey = positionKey;
    const std::size_t playerPosition = legacyNoPlayerKey.find("|P:W");
    require(playerPosition != std::string::npos, "test should build a player-to-move key");
    legacyNoPlayerKey.erase(playerPosition, std::string("|P:W").size());
    response.rootVisits = 80;
    const lizzie::app::AnalysisStoreUpdateResult noPlayerApplied =
        store.applyResponse(response, std::string_view(legacyNoPlayerKey));
    require(noPlayerApplied.applied(), "analysis store should accept unambiguous legacy keys without player-to-move");
    require(model.node(nodeId).analysis->rootVisits == 80, "legacy no-player key should update analysis");
}

void testAnalysisStoreWritesFailuresWithPositionKey()
{
    lizzie::core::GameModel model(lizzie::core::BoardSize::square(9));
    model.gameInfo().rules = "Japanese";
    const lizzie::core::NodeId nodeId =
        model.addMove(lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{4, 4}));
    const std::string requestId = lizzie::engine::analysisRequestId(nodeId);
    const std::string positionKey = lizzie::engine::analysisPositionKey(model, nodeId);

    lizzie::core::AnalysisSnapshot snapshot;
    snapshot.rootVisits = 12;
    model.node(nodeId).analysis = snapshot;

    lizzie::app::AnalysisStore store(&model);
    const lizzie::app::AnalysisStoreUpdateResult stale =
        store.setErrorForRequest(requestId, "analysis failed", std::string_view("old-key"));
    require(
        stale.status == lizzie::app::AnalysisStoreUpdateStatus::StalePosition,
        "analysis store should reject stale request failures");
    require(model.node(nodeId).analysis.has_value(), "stale request failures should preserve existing analysis");
    require(model.node(nodeId).analysisError.empty(), "stale request failures should not write an error");

    const lizzie::app::AnalysisStoreUpdateResult applied =
        store.setErrorForRequest(requestId, "analysis failed", std::string_view(positionKey));
    require(applied.applied(), "analysis store should write a matching request failure");
    require(!model.node(nodeId).analysis.has_value(), "matching request failure should clear analysis");
    require(model.node(nodeId).analysisError == "analysis failed", "matching request failure should store diagnostics");

    model.node(nodeId).analysis = snapshot;
    model.node(nodeId).analysisError.clear();
    std::string legacyRawRuleKey = positionKey;
    const std::size_t rulePosition = legacyRawRuleKey.find("|RU:japanese|");
    require(rulePosition != std::string::npos, "test should build a canonical Japanese failure key");
    legacyRawRuleKey.replace(rulePosition, std::string("|RU:japanese|").size(), "|RU:jp|");
    const lizzie::app::AnalysisStoreUpdateResult legacyApplied =
        store.setErrorForRequest(requestId, "legacy-key failure", std::string_view(legacyRawRuleKey));
    require(legacyApplied.applied(), "analysis store should accept legacy raw-rule keys for failures");
    require(model.node(nodeId).analysisError == "legacy-key failure", "legacy raw-rule failure should store diagnostics");

    const lizzie::app::AnalysisStoreUpdateResult invalid =
        store.setErrorForRequest("bad-id", "analysis failed");
    require(
        invalid.status == lizzie::app::AnalysisStoreUpdateStatus::InvalidRequestId,
        "analysis store should reject invalid request ids");

    const lizzie::app::AnalysisStoreUpdateResult missing =
        store.setErrorForRequest(lizzie::engine::analysisRequestId(999), "analysis failed");
    require(
        missing.status == lizzie::app::AnalysisStoreUpdateStatus::MissingNode,
        "analysis store should reject missing request nodes");
}

void testEngineManagerOwnsStableEngineSessions()
{
    lizzie::app::EngineManager manager;

    lizzie::engine::ThreadedKataGoProcess* mainEngine = &manager.mainEngine();
    lizzie::engine::ThreadedKataGoProcess* compareEngine = &manager.compareEngine();
    lizzie::engine::ThreadedAnalysisProcess* batchAnalysis = &manager.batchAnalysis();
    require(mainEngine != compareEngine, "engine manager should own separate main and compare engines");
    require(&manager.mainEngine() == mainEngine, "engine manager should return a stable main engine reference");
    require(&manager.compareEngine() == compareEngine, "engine manager should return a stable compare engine reference");
    require(&manager.batchAnalysis() == batchAnalysis, "engine manager should return a stable batch process reference");

    manager.mainCapabilities().genmove = true;
    manager.compareCapabilities().kataAnalyze = true;
    manager.resetMainCapabilities();
    manager.resetCompareCapabilities();
    require(!manager.mainCapabilities().genmove, "engine manager should reset main capabilities");
    require(!manager.compareCapabilities().kataAnalyze, "engine manager should reset compare capabilities");

    manager.stopAll();
    require(!manager.mainEngine().isRunning(), "engine manager stopAll should leave main engine stopped");
    require(!manager.compareEngine().isRunning(), "engine manager stopAll should leave compare engine stopped");
    require(!manager.batchAnalysis().isRunning(), "engine manager stopAll should leave batch analysis stopped");
}

void testRealtimeAnalysisCommandPlannerBuildsFullSyncPlan()
{
    lizzie::core::GameModel model(lizzie::core::BoardSize{9, 13});
    model.gameInfo().komi = 6.5;
    model.gameInfo().rules = "Japanese";
    const lizzie::core::NodeId nodeId =
        model.addMove(lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{2, 2}));

    lizzie::engine::RealtimeAnalysisOptions options;
    options.includeOwnership = false;
    options.maxVisits = 123;
    options.maxPlayouts = 456;
    options.maxTimeSeconds = 4.5;
    options.playoutDoublingAdvantage = -0.25;
    options.analysisWideRootNoise = 0.35;

    lizzie::engine::EngineCapabilities capabilities;
    capabilities.kataSetRules = true;
    capabilities.kataStop = true;
    capabilities.kataSetParam = true;
    const lizzie::app::EngineCommandPlan plan =
        lizzie::app::buildRealtimeAnalysisCommandPlan(model, nodeId, capabilities, options);

    require(!plan.fatal, "realtime command plan should accept supported rectangular boards");
    require(plan.warnings.empty(), "realtime command plan should not warn for supported sync");
    require(plan.sideToMove == lizzie::core::Color::White, "realtime command plan should use the current side to move");
    require(!plan.positionKey.empty(), "realtime command plan should include a stale-response position key");
    require(
        plan.preparationCommands.size() == 11,
        "realtime command plan should include stop, sync commands, and search parameters");
    require(plan.preparationCommands[0].toLine() == "kata-stop\n", "realtime command plan should stop old analysis first");
    require(
        plan.preparationCommands[2].toLine() == "rectangular_boardsize 9 13\n",
        "realtime command plan should sync rectangular board size");
    require(
        plan.preparationCommands[4].toLine() == "kata-set-rules japanese\n",
        "realtime command plan should send canonical KataGo rules when supported");
    require(
        plan.preparationCommands[6].toLine() == "kata-set-param maxVisits 123\n",
        "realtime command plan should include maxVisits search parameter");
    require(
        plan.preparationCommands[7].toLine() == "kata-set-param maxPlayouts 456\n",
        "realtime command plan should include maxPlayouts search parameter");
    require(
        plan.preparationCommands[8].toLine() == "kata-set-param maxTime 4.5\n",
        "realtime command plan should include maxTime search parameter");
    require(
        plan.preparationCommands[9].toLine() == "kata-set-param playoutDoublingAdvantage -0.25\n",
        "realtime command plan should include PDA search parameter");
    require(
        plan.preparationCommands[10].toLine() == "kata-set-param analysisWideRootNoise 0.35\n",
        "realtime command plan should include WRN search parameter");
    require(
        plan.finalCommand.toLine() ==
            "kata-analyze W 50 ownership false movesOwnership false pvVisits true rootInfo true\n",
        "realtime command plan should build the final kata-analyze command");
}

void testRealtimeAnalysisCommandPlannerSkipsUnsupportedExtensionCommands()
{
    lizzie::core::GameModel model(lizzie::core::BoardSize::square(9));

    lizzie::engine::RealtimeAnalysisOptions options;
    options.maxVisits = 123;

    const lizzie::app::EngineCommandPlan plan = lizzie::app::buildRealtimeAnalysisCommandPlan(
        model,
        model.currentId(),
        lizzie::engine::EngineCapabilities{},
        options);

    require(!plan.fatal, "realtime command plan should still sync when optional extensions are unsupported");
    require(
        !containsCommandName(plan.preparationCommands, "kata-stop"),
        "realtime command plan should skip unsupported kata-stop");
    require(
        !containsCommandName(plan.preparationCommands, "kata-set-param"),
        "realtime command plan should skip unsupported kata-set-param");
    require(
        containsWarning(plan.warnings, "kata-set-param"),
        "realtime command plan should warn when search parameters cannot be sent");
    require(
        plan.finalCommand.name == "kata-analyze",
        "realtime command plan should still build kata-analyze when sync succeeds");
}

void testRealtimeAnalysisCommandPlannerOmitsFinalCommandForFatalPosition()
{
    lizzie::core::GameModel model(lizzie::core::BoardSize::square(5));
    const lizzie::core::NodeId first =
        model.addMove(lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{1, 1}));
    const lizzie::core::NodeId illegal =
        model.addChild(first, lizzie::core::Move::play(lizzie::core::Color::White, lizzie::core::Point{1, 1}));

    lizzie::engine::EngineCapabilities capabilities;
    capabilities.kataStop = true;
    const lizzie::app::EngineCommandPlan plan =
        lizzie::app::buildRealtimeAnalysisCommandPlan(model, illegal, capabilities, {});

    require(plan.fatal, "realtime command plan should be fatal for an invalid local position");
    require(!plan.positionError.empty(), "fatal realtime command plan should keep the local position error");
    require(plan.preparationCommands.size() == 1, "fatal realtime command plan should only stop stale analysis");
    require(
        plan.preparationCommands.front().toLine() == "kata-stop\n",
        "fatal realtime command plan should stop old analysis first");
    require(plan.finalCommand.name.empty(), "fatal realtime command plan should not expose kata-analyze");
}

void testGenMoveCommandPlannerBuildsFinalMoveCommand()
{
    lizzie::core::GameModel model(lizzie::core::BoardSize::square(9));
    model.gameInfo().komi = 7.5;

    lizzie::engine::RealtimeAnalysisOptions options;
    options.maxVisits = 77;
    options.maxPlayouts = 88;
    options.maxTimeSeconds = 1.25;
    options.playoutDoublingAdvantage = -0.5;
    options.analysisWideRootNoise = 0.6;

    lizzie::engine::EngineCapabilities capabilities;
    capabilities.kataSetRules = true;
    capabilities.kataStop = true;
    capabilities.kataSetParam = true;
    const lizzie::app::EngineCommandPlan plan =
        lizzie::app::buildGenMoveCommandPlan(model, model.currentId(), capabilities, options);

    require(!plan.fatal, "genmove command plan should accept an empty root position");
    require(plan.preparationCommands.size() == 9, "genmove command plan should include stop, sync, and search params");
    require(plan.preparationCommands.front().toLine() == "kata-stop\n", "genmove command plan should stop analysis first");
    require(
        plan.preparationCommands[4].toLine() == "kata-set-param maxVisits 77\n",
        "genmove command plan should include maxVisits before genmove");
    require(
        plan.preparationCommands[5].toLine() == "kata-set-param maxPlayouts 88\n",
        "genmove command plan should include maxPlayouts before genmove");
    require(
        plan.preparationCommands[6].toLine() == "kata-set-param maxTime 1.25\n",
        "genmove command plan should include maxTime before genmove");
    require(
        plan.preparationCommands[7].toLine() == "kata-set-param playoutDoublingAdvantage -0.5\n",
        "genmove command plan should include PDA before genmove");
    require(
        plan.preparationCommands[8].toLine() == "kata-set-param analysisWideRootNoise 0.6\n",
        "genmove command plan should include WRN before genmove");
    require(plan.finalCommand.toLine() == "genmove B\n", "genmove command plan should request the side to move");
}

void testGenMoveCommandPlannerKeepsFatalSyncDiagnosable()
{
    lizzie::core::GameModel model(lizzie::core::BoardSize{26, 19});

    lizzie::engine::EngineCapabilities capabilities;
    capabilities.kataStop = true;
    const lizzie::app::EngineCommandPlan plan =
        lizzie::app::buildGenMoveCommandPlan(model, model.currentId(), capabilities, {});

    require(plan.fatal, "wide-board genmove command plan should be fatal for GTP");
    require(!plan.warnings.empty(), "wide-board genmove command plan should keep diagnostics");
    require(plan.preparationCommands.size() == 1, "wide-board genmove command plan should only stop stale analysis");
    require(
        plan.preparationCommands.front().toLine() == "kata-stop\n",
        "wide-board genmove command plan should stop analysis before reporting fatal sync");
    require(plan.finalCommand.name.empty(), "wide-board genmove command plan should not expose genmove");
}

void testBatchAnalysisRunPlannerBuildsEstimateRequest()
{
    lizzie::core::GameModel model(lizzie::core::BoardSize::square(9));
    const lizzie::core::NodeId first =
        model.addMove(lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{2, 2}));
    const lizzie::core::NodeId second =
        model.addMove(lizzie::core::Move::play(lizzie::core::Color::White, lizzie::core::Point{3, 3}));

    lizzie::engine::RealtimeAnalysisOptions options;
    options.includeOwnership = false;
    options.maxVisits = 77;
    options.maxPlayouts = 88;
    options.maxTimeSeconds = 1.5;
    options.playoutDoublingAdvantage = -0.25;
    options.analysisWideRootNoise = 0.4;

    const lizzie::app::BatchAnalysisRunPlan plan = lizzie::app::buildEstimateRunPlan(model, options);

    require(plan.hasRequests(), "estimate run planner should produce a request for the current node");
    require(plan.requests.size() == 1, "estimate run planner should only analyze the current node");
    require(plan.nodesToClear.size() == 1 && plan.nodesToClear.front() == second, "estimate should clear current node error");
    require(plan.requests.front().id == lizzie::engine::analysisRequestId(second), "estimate should request current node id");
    require(plan.requests.front().includeOwnership, "estimate should force ownership even if realtime setting hides it");
    require(plan.requests.front().maxVisits == 77, "estimate should copy max visits");
    require(plan.requests.front().maxPlayouts == 88, "estimate should copy max playouts");
    require(plan.requests.front().maxTime == 1.5, "estimate should copy max time");
    require(plan.requests.front().playoutDoublingAdvantage == -0.25, "estimate should copy PDA");
    require(plan.requests.front().wideRootNoise == 0.4, "estimate should copy WRN");
    require(plan.positionKeys.size() == 1, "estimate should track one position key");
    require(
        plan.positionKeys.at(plan.requests.front().id) == lizzie::engine::analysisPositionKey(model, second),
        "estimate should track the current node position key");
    require(model.currentId() == second && first != second, "test model should keep the intended current node");
}

void testBatchAnalysisRunPlannerBuildsFullBatchRequests()
{
    lizzie::core::GameModel model(lizzie::core::BoardSize::square(9));
    const lizzie::core::NodeId first =
        model.addMove(lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{2, 2}));
    const lizzie::core::NodeId second =
        model.addMove(lizzie::core::Move::play(lizzie::core::Color::White, lizzie::core::Point{3, 3}));

    lizzie::engine::RealtimeAnalysisOptions options;
    options.includeOwnership = false;
    options.maxVisits = 99;
    options.maxPlayouts = 88;
    options.maxTimeSeconds = 2.5;
    options.playoutDoublingAdvantage = 0.35;
    options.analysisWideRootNoise = 0.2;

    const lizzie::app::BatchAnalysisRunPlan plan = lizzie::app::buildBatchRunPlan(model, options);

    require(plan.requests.size() == 3, "batch run planner should analyze root and descendants");
    require(plan.nodesToClear.size() == 3, "batch run planner should clear each requested node error");
    require(plan.nodesToClear[0] == model.rootId(), "batch run planner should start at the root");
    require(plan.nodesToClear[1] == first, "batch run planner should include first move");
    require(plan.nodesToClear[2] == second, "batch run planner should include second move");
    require(!plan.requests.front().includeOwnership, "batch run planner should preserve ownership setting");
    require(plan.requests.front().maxVisits == 99, "batch run planner should copy search limits");
    require(plan.requests.front().maxPlayouts == 88, "batch run planner should copy max playouts");
    require(plan.requests.front().maxTime == 2.5, "batch run planner should copy max time");
    require(plan.requests.front().playoutDoublingAdvantage == 0.35, "batch run planner should copy PDA");
    require(plan.requests.front().wideRootNoise == 0.2, "batch run planner should copy WRN");
    require(plan.positionKeys.size() == plan.requests.size(), "batch run planner should track each position key");
    for (const lizzie::engine::AnalysisRequest& request : plan.requests) {
        require(plan.positionKeys.contains(request.id), "batch run planner should key every request id");
    }
}

void testBatchAnalysisRunPlannerKeepsMidGameSetupSnapshotRequest()
{
    lizzie::core::GameModel model(lizzie::core::BoardSize::square(9));
    const lizzie::core::NodeId first =
        model.addMove(lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{2, 2}));
    const lizzie::core::NodeId second =
        model.addMove(lizzie::core::Move::play(lizzie::core::Color::White, lizzie::core::Point{3, 3}));
    model.node(second).setupStones.empty.push_back(lizzie::core::Point{2, 2});

    lizzie::engine::RealtimeAnalysisOptions options;
    options.includeOwnership = false;
    options.maxVisits = 55;

    const lizzie::app::BatchAnalysisRunPlan plan = lizzie::app::buildBatchRunPlan(model, options);

    require(plan.requests.size() == 3, "batch run planner should keep mid-game setup nodes");
    require(plan.nodesToClear.size() == 3, "batch run planner should clear mid-game setup node errors");
    require(plan.warnings.empty(), "batch run planner should not warn for snapshot-compatible setup nodes");

    const std::string secondId = lizzie::engine::analysisRequestId(second);
    const auto found = std::ranges::find_if(plan.requests, [&secondId](const lizzie::engine::AnalysisRequest& request) {
        return request.id == secondId;
    });
    require(found != plan.requests.end(), "batch run planner should include the setup-node request");
    require(found->moves.empty(), "setup-node request should use a final-position snapshot");
    require(found->initialPlayer == lizzie::core::Color::Black, "setup-node snapshot should preserve side to move");
    require(found->initialStones.size() == 1, "setup-node snapshot should encode the final board");
    require(
        containsInitialStone(found->initialStones, lizzie::core::Color::White, lizzie::core::Point{3, 3}),
        "setup-node snapshot should keep the played white stone");
    require(
        !containsInitialStone(found->initialStones, lizzie::core::Color::Black, lizzie::core::Point{2, 2}),
        "setup-node snapshot should apply AE clears");
    require(plan.positionKeys.contains(secondId), "batch run planner should track the setup-node position key");
    require(
        plan.positionKeys.at(secondId) == lizzie::engine::analysisPositionKey(model, second),
        "batch run planner should store the setup-node position key");
}

void testBatchAnalysisTrackerTracksPendingRequests()
{
    lizzie::app::BatchAnalysisTracker tracker;
    require(tracker.empty(), "batch tracker should start empty");

    tracker.reset({{"node:1", "key-1"}, {"node:2", "key-2"}});
    require(!tracker.empty(), "batch tracker should not be empty after reset");
    require(tracker.size() == 2, "batch tracker should expose pending request count");

    const std::string* key = tracker.positionKeyFor("node:1");
    require(key != nullptr && *key == "key-1", "batch tracker should find tracked position keys");
    require(tracker.positionKeyFor("missing") == nullptr, "batch tracker should report missing request ids");

    const std::vector<lizzie::app::TrackedAnalysisRequest> pending = tracker.pendingRequests();
    require(pending.size() == 2, "batch tracker should list pending requests");

    tracker.remove("node:1");
    require(tracker.positionKeyFor("node:1") == nullptr, "batch tracker should remove completed requests");
    require(tracker.size() == 1, "batch tracker should keep remaining requests after remove");

    tracker.clear();
    require(tracker.empty(), "batch tracker should clear all requests");
}

void testBatchAnalysisCompletionPlannerWritesBatchOutput()
{
    lizzie::core::GameModel game(lizzie::core::BoardSize::square(9));
    lizzie::app::GameDocumentSession session;
    session.currentFile = QStringLiteral("/tmp/game.sgf");
    session.collection = {game};
    session.collectionGameCount = 1;

    const lizzie::app::BatchAnalysisCompletionPlan plan = lizzie::app::planBatchAnalysisCompletion(
        lizzie::app::BatchAnalysisRunMode::Batch,
        false,
        true,
        session,
        false);

    require(plan.output.has_value(), "batch completion planner should write saved batch output");
    require(!plan.output->writeSidecar, "batch completion planner should honor SGF writeback setting");
    require(plan.output->path == QStringLiteral("/tmp/game.sgf"), "batch completion planner should keep SGF output path");
    require(
        plan.output->normalizeSingleGameCollectionAfterSgfWrite,
        "batch completion planner should normalize single-game SGF writeback sessions");
    require(
        plan.output->message == lizzie::app::BatchAnalysisCompletionMessage::BatchFailedWithOutput,
        "batch completion planner should keep failed writeback message");
    require(
        plan.fallbackMessage == lizzie::app::BatchAnalysisCompletionMessage::BatchFailed,
        "batch completion planner should expose failed fallback message");
}

void testBatchAnalysisCompletionPlannerSkipsOutputWhenCancelledOrUnsaved()
{
    lizzie::app::GameDocumentSession session;

    const lizzie::app::BatchAnalysisCompletionPlan cancelledEstimate = lizzie::app::planBatchAnalysisCompletion(
        lizzie::app::BatchAnalysisRunMode::Estimate,
        true,
        false,
        session,
        true);
    require(!cancelledEstimate.output.has_value(), "estimate completion should not write output");
    require(
        cancelledEstimate.fallbackMessage == lizzie::app::BatchAnalysisCompletionMessage::EstimateCancelled,
        "estimate completion should report cancelled fallback");

    const lizzie::app::BatchAnalysisCompletionPlan unsavedBatch = lizzie::app::planBatchAnalysisCompletion(
        lizzie::app::BatchAnalysisRunMode::Batch,
        false,
        false,
        session,
        true);
    require(!unsavedBatch.output.has_value(), "unsaved batch completion should not write output");
    require(
        unsavedBatch.fallbackMessage == lizzie::app::BatchAnalysisCompletionMessage::BatchComplete,
        "unsaved batch completion should report complete fallback");
}

void testGameEditorUpdatesCommentsAndMarkup()
{
    lizzie::core::GameModel model(lizzie::core::BoardSize::square(9));
    lizzie::app::GameEditor editor(&model);
    const lizzie::core::NodeId root = model.rootId();

    require(editor.setComment(root, "root note"), "game editor should update comments");
    require(model.root().comment == "root note", "game editor should write node comment text");
    require(model.root().properties.contains("C"), "game editor should write SGF comment property");
    require(model.root().properties.at("C").front() == "root note", "game editor should mirror comment property");
    require(!editor.setComment(root, "root note"), "reapplying the same comment should report no change");

    model.root().properties["C"] = {"root note", "duplicate note"};
    model.root().properties["c"] = {"stale lowercase note"};
    require(
        editor.setComment(root, "root note"),
        "game editor should canonicalize duplicate comment properties even when text is unchanged");
    require(model.root().comment == "root note", "comment property canonicalization should preserve node text");
    require(model.root().properties.contains("C"), "comment property canonicalization should keep canonical C");
    require(model.root().properties.at("C").size() == 1, "comment property canonicalization should keep one C value");
    require(
        model.root().properties.at("C").front() == "root note",
        "comment property canonicalization should keep the current comment text");
    require(!model.root().properties.contains("c"), "comment property canonicalization should remove lowercase c");
    require(
        !editor.setComment(root, "root note"),
        "reapplying a canonical unchanged comment should report no change");

    model.root().comment = "legacy note";
    model.root().properties.clear();
    model.root().properties["c"] = {"legacy note"};
    require(editor.setComment(root, "updated note"), "game editor should update mixed-case SGF comments");
    require(model.root().comment == "updated note", "mixed-case comment update should change node text");
    require(model.root().properties.contains("C"), "mixed-case comment update should write canonical C");
    require(!model.root().properties.contains("c"), "mixed-case comment update should remove stale lowercase c");

    require(editor.setComment(root, ""), "game editor should clear comments");
    require(model.root().comment.empty(), "game editor should clear node comment text");
    require(!model.root().properties.contains("C"), "game editor should remove empty SGF comment property");
    require(!editor.setComment(root, ""), "clearing an already empty comment should report no change");
    model.root().properties["c"] = {"stale empty comment"};
    require(
        editor.setComment(root, ""),
        "game editor should clear stale comment properties even when empty comment text is unchanged");
    require(!model.root().properties.contains("c"), "empty comment canonicalization should remove lowercase c");
    require(
        !editor.setComment(root, ""),
        "reclearing a canonical empty comment should report no change");

    model.root().comment = "legacy clear";
    model.root().properties["c"] = {"legacy clear"};
    require(editor.setComment(root, ""), "game editor should clear mixed-case SGF comments");
    require(!model.root().properties.contains("c"), "mixed-case comment clear should remove stale lowercase c");

    const lizzie::core::Point point{2, 3};
    const lizzie::core::Point otherPoint{1, 1};
    const auto labelCountAt = [&model](lizzie::core::Point labelPoint) {
        return std::ranges::count_if(model.root().labels, [labelPoint](const lizzie::core::BoardLabel& label) {
            return label.point == labelPoint;
        });
    };
    const auto markCountAt = [&model](lizzie::core::Point markPoint) {
        return std::ranges::count_if(model.root().marks, [markPoint](const lizzie::core::BoardMark& mark) {
            return mark.point == markPoint;
        });
    };
    require(editor.setLabel(root, point, "A"), "game editor should set labels");
    std::optional<std::string> label = editor.labelAt(root, point);
    require(label.has_value() && *label == "A", "game editor should read labels");
    require(!editor.setLabel(root, point, "A"), "reapplying the same label should report no change");

    require(editor.setLabel(root, point, "B"), "game editor should update existing labels");
    label = editor.labelAt(root, point);
    require(label.has_value() && *label == "B", "game editor should replace label text");
    model.root().labels.push_back(lizzie::core::BoardLabel{point, "stale"});
    model.root().labels.push_back(lizzie::core::BoardLabel{point, "duplicate"});
    model.root().labels.push_back(lizzie::core::BoardLabel{otherPoint, "keep"});
    require(editor.setLabel(root, point, "C"), "game editor should canonicalize duplicate labels");
    require(labelCountAt(point) == 1, "game editor should keep one label at the edited point");
    require(labelCountAt(otherPoint) == 1, "game editor should preserve labels at other points");
    label = editor.labelAt(root, point);
    require(label.has_value() && *label == "C", "game editor should keep the requested duplicate label text");
    model.root().labels.push_back(lizzie::core::BoardLabel{point, "stale clear"});
    require(editor.setLabel(root, point, ""), "game editor should clear duplicate labels at the edited point");
    require(labelCountAt(point) == 0, "game editor should remove all duplicate labels at the edited point");
    require(labelCountAt(otherPoint) == 1, "game editor should preserve unrelated labels when clearing duplicates");

    require(editor.toggleMark(root, point, lizzie::core::BoardMarkShape::Circle), "game editor should add marks");
    require(markCountAt(point) == 1, "game editor should store one mark at the edited point");
    require(model.root().marks.front().shape == lizzie::core::BoardMarkShape::Circle, "game editor should store mark shape");

    require(editor.toggleMark(root, point, lizzie::core::BoardMarkShape::Square), "game editor should update marks");
    require(markCountAt(point) == 1, "game editor should update mark in place");
    require(model.root().marks.front().shape == lizzie::core::BoardMarkShape::Square, "game editor should replace mark shape");
    model.root().marks.push_back(lizzie::core::BoardMark{point, lizzie::core::BoardMarkShape::Circle});
    model.root().marks.push_back(lizzie::core::BoardMark{point, lizzie::core::BoardMarkShape::Triangle});
    model.root().marks.push_back(lizzie::core::BoardMark{otherPoint, lizzie::core::BoardMarkShape::Square});
    require(editor.toggleMark(root, point, lizzie::core::BoardMarkShape::Circle), "game editor should canonicalize duplicate marks");
    require(markCountAt(point) == 1, "game editor should keep one mark at the edited point");
    require(markCountAt(otherPoint) == 1, "game editor should preserve marks at other points");
    require(
        std::ranges::any_of(model.root().marks, [point](const lizzie::core::BoardMark& mark) {
            return mark.point == point && mark.shape == lizzie::core::BoardMarkShape::Circle;
        }),
        "game editor should keep the requested duplicate mark shape");

    require(editor.clearMarkupAt(root, point), "game editor should clear existing markup");
    require(editor.labelAt(root, point) == std::nullopt, "game editor should remove label when clearing markup");
    require(markCountAt(point) == 0, "game editor should remove marks at the edited point when clearing markup");
    require(markCountAt(otherPoint) == 1, "game editor should preserve unrelated marks when clearing markup");
    require(editor.clearMarkupAt(root, otherPoint), "game editor should clear unrelated markup explicitly");
    require(model.root().marks.empty(), "game editor should remove all marks after clearing remaining markup");
    require(!editor.clearMarkupAt(root, point), "clearing absent markup should report no change");

    require(editor.toggleMark(root, point, lizzie::core::BoardMarkShape::Square), "game editor should re-add marks");
    require(editor.toggleMark(root, point, lizzie::core::BoardMarkShape::Square), "game editor should toggle marks off");
    require(model.root().marks.empty(), "game editor should remove matching mark");

    require(!editor.setLabel(root, point, ""), "clearing an absent label should report no change");
}

void testGameEditorSgfWriteClearsStaleStructuredProperties()
{
    lizzie::core::SgfParser parser;
    lizzie::core::GameModel model = parser.parseGame(
        "(;GM[1]FF[4]SZ[9]C[keep]AB[aa][bb]AW[cc]AE[dd]LB[ee:old]"
        "CR[ff]SQ[gg]TR[hh]MA[ii]ZZ[keep])");
    lizzie::app::GameEditor editor(&model);
    const lizzie::core::NodeId root = model.rootId();

    require(
        editor.editSetupStone(root, lizzie::core::Point{0, 0}, lizzie::app::SetupStoneEdit::Empty),
        "game editor SGF test should replace stale black setup with AE");
    require(
        editor.editSetupStone(root, lizzie::core::Point{2, 2}, lizzie::app::SetupStoneEdit::Black),
        "game editor SGF test should replace stale white setup with AB");
    require(
        editor.editSetupStone(root, lizzie::core::Point{3, 3}, lizzie::app::SetupStoneEdit::Empty),
        "game editor SGF test should toggle stale AE off");
    require(editor.setLabel(root, lizzie::core::Point{4, 4}, "new"), "game editor SGF test should update labels");
    require(editor.clearMarkupAt(root, lizzie::core::Point{5, 5}), "game editor SGF test should clear circle mark");
    require(editor.clearMarkupAt(root, lizzie::core::Point{6, 6}), "game editor SGF test should clear square mark");
    require(editor.clearMarkupAt(root, lizzie::core::Point{7, 7}), "game editor SGF test should clear triangle mark");
    require(editor.clearMarkupAt(root, lizzie::core::Point{8, 8}), "game editor SGF test should clear cross mark");
    require(
        editor.toggleMark(root, lizzie::core::Point{0, 4}, lizzie::core::BoardMarkShape::Circle),
        "game editor SGF test should add a fresh mark");

    const std::string written = lizzie::core::writeSgf(model);
    require(written.find("C[keep]") != std::string::npos, "editor SGF write should preserve user comments");
    require(written.find("ZZ[keep]") != std::string::npos, "editor SGF write should preserve unrelated properties");
    require(written.find("AB[aa]") == std::string::npos, "editor SGF write should remove stale black setup points");
    require(written.find("AW[cc]") == std::string::npos, "editor SGF write should remove stale white setup points");
    require(written.find("AE[dd]") == std::string::npos, "editor SGF write should remove stale empty setup points");
    require(written.find("LB[ee:old]") == std::string::npos, "editor SGF write should remove stale label text");
    require(written.find("LB[ee:new]") != std::string::npos, "editor SGF write should write updated label text");
    require(written.find("CR[ff]") == std::string::npos, "editor SGF write should remove stale circle marks");
    require(written.find("SQ[gg]") == std::string::npos, "editor SGF write should remove stale square marks");
    require(written.find("TR[hh]") == std::string::npos, "editor SGF write should remove stale triangle marks");
    require(written.find("MA[ii]") == std::string::npos, "editor SGF write should remove stale cross marks");
    require(written.find("CR[ae]") != std::string::npos, "editor SGF write should write fresh marks");

    const lizzie::core::GameModel restored = parser.parseGame(written);
    require(
        containsPoint(restored.root().setupStones.black, lizzie::core::Point{1, 1}) &&
            containsPoint(restored.root().setupStones.black, lizzie::core::Point{2, 2}),
        "editor SGF write should round-trip fresh black setup points");
    require(restored.root().setupStones.white.empty(), "editor SGF write should not round-trip stale white setup");
    require(
        restored.root().setupStones.empty.size() == 1 &&
            containsPoint(restored.root().setupStones.empty, lizzie::core::Point{0, 0}),
        "editor SGF write should round-trip fresh AE setup points only");
    require(
        restored.root().labels.size() == 1 && restored.root().labels.front().text == "new",
        "editor SGF write should round-trip updated labels");
    require(
        restored.root().marks.size() == 1 &&
            restored.root().marks.front().shape == lizzie::core::BoardMarkShape::Circle &&
            restored.root().marks.front().point == lizzie::core::Point{0, 4},
        "editor SGF write should round-trip fresh marks only");
}

void testGameEditorEditsSetupAndTree()
{
    lizzie::core::GameModel model(lizzie::core::BoardSize::square(9));
    lizzie::app::GameEditor editor(&model);
    const lizzie::core::NodeId root = model.rootId();
    const lizzie::core::Point point{1, 1};
    const lizzie::core::Point otherPoint{2, 2};
    const auto setupPointCount = [](const std::vector<lizzie::core::Point>& points, lizzie::core::Point setupPoint) {
        return std::ranges::count(points, setupPoint);
    };

    require(editor.editSetupStone(root, point, lizzie::app::SetupStoneEdit::Black), "game editor should set black setup stones");
    require(containsPoint(model.root().setupStones.black, point), "black setup stone should be present");
    require(model.root().setupStones.white.empty(), "white setup stones should stay empty");
    require(model.root().setupStones.empty.empty(), "empty setup stones should stay empty");
    require(
        !editor.editSetupStone(root, point, lizzie::app::SetupStoneEdit::Black),
        "reapplying the same black setup stone should report no change");
    require(model.root().setupStones.black.size() == 1, "no-op black setup edit should not duplicate stones");

    require(editor.editSetupStone(root, point, lizzie::app::SetupStoneEdit::White), "game editor should set white setup stones");
    require(model.root().setupStones.black.empty(), "black setup stone should be replaced");
    require(containsPoint(model.root().setupStones.white, point), "white setup stone should be present");
    require(model.root().setupStones.empty.empty(), "empty setup stones should stay empty");
    require(
        !editor.editSetupStone(root, point, lizzie::app::SetupStoneEdit::White),
        "reapplying the same white setup stone should report no change");
    require(model.root().setupStones.white.size() == 1, "no-op white setup edit should not duplicate stones");

    require(editor.editSetupStone(root, point, lizzie::app::SetupStoneEdit::Empty), "game editor should set empty setup points");
    require(model.root().setupStones.black.empty(), "black setup stones should stay empty");
    require(model.root().setupStones.white.empty(), "white setup stone should be replaced");
    require(containsPoint(model.root().setupStones.empty, point), "empty setup point should be present");

    require(editor.editSetupStone(root, point, lizzie::app::SetupStoneEdit::Empty), "game editor should toggle empty setup points");
    require(model.root().setupStones.empty.empty(), "empty setup point should be removed when toggled");

    model.root().setupStones.black.push_back(point);
    model.root().setupStones.black.push_back(point);
    model.root().setupStones.white.push_back(point);
    model.root().setupStones.empty.push_back(point);
    model.root().setupStones.empty.push_back(point);
    model.root().setupStones.black.push_back(otherPoint);
    require(
        editor.editSetupStone(root, point, lizzie::app::SetupStoneEdit::Empty),
        "game editor should canonicalize conflicting setup stones to empty");
    require(setupPointCount(model.root().setupStones.black, point) == 0, "empty setup edit should clear stale black points");
    require(setupPointCount(model.root().setupStones.white, point) == 0, "empty setup edit should clear stale white points");
    require(setupPointCount(model.root().setupStones.empty, point) == 1, "empty setup edit should keep one empty point");
    require(
        containsPoint(model.root().setupStones.black, otherPoint),
        "empty setup canonicalization should preserve unrelated setup stones");

    model.root().setupStones.empty.push_back(point);
    require(
        editor.editSetupStone(root, point, lizzie::app::SetupStoneEdit::Empty),
        "game editor should canonicalize duplicate empty setup points");
    require(setupPointCount(model.root().setupStones.empty, point) == 1, "duplicate empty setup edit should keep one point");

    require(
        editor.editSetupStone(root, point, lizzie::app::SetupStoneEdit::Empty),
        "game editor should toggle canonical empty setup points off");
    require(setupPointCount(model.root().setupStones.empty, point) == 0, "canonical empty setup point should toggle off");

    const lizzie::core::NodeId first =
        model.addChild(root, lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{3, 3}));
    const lizzie::core::NodeId second =
        model.addChild(first, lizzie::core::Move::play(lizzie::core::Color::White, lizzie::core::Point{4, 4}));
    const lizzie::core::NodeId variation =
        model.addChild(first, lizzie::core::Move::play(lizzie::core::Color::White, lizzie::core::Point{5, 5}));

    require(editor.setCurrent(second), "game editor should select nodes");
    require(editor.selectParent(), "game editor should select parent nodes");
    require(model.currentId() == first, "game editor parent selection should update current node");
    require(editor.selectFirstChild(), "game editor should select first child nodes");
    require(model.currentId() == second, "game editor child selection should update current node");

    require(editor.setCurrent(variation), "game editor should select variation nodes");
    require(editor.promoteCurrentVariation(), "game editor should promote current variation");
    require(model.node(first).children.front() == variation, "promoted variation should become first child");

    require(editor.deleteCurrentSubtree(), "game editor should delete current subtree");
    require(model.currentId() == first, "deleting current subtree should move current node to parent");
    require(model.findNode(variation) == nullptr, "deleted variation should be removed");
}

void testNewGameSetupAppliesStandardHandicapStones()
{
    lizzie::core::GameInfo info;
    info.handicap = 4;
    info.komi = 0.5;
    info.rules = "Japanese";
    lizzie::core::GameModel model =
        lizzie::app::createNewGameModel(lizzie::core::BoardSize::square(19), info);

    require(model.gameInfo().handicap == 4, "new game setup should preserve handicap game info");
    require(model.root().setupStones.black.size() == 4, "new 19x19 handicap game should place four black stones");
    require(
        containsPoint(model.root().setupStones.black, lizzie::core::Point{3, 15}),
        "new 19x19 handicap game should place lower-left star point");
    require(
        containsPoint(model.root().setupStones.black, lizzie::core::Point{15, 3}),
        "new 19x19 handicap game should place upper-right star point");
    require(
        containsPoint(model.root().setupStones.black, lizzie::core::Point{15, 15}),
        "new 19x19 handicap game should place lower-right star point");
    require(
        containsPoint(model.root().setupStones.black, lizzie::core::Point{3, 3}),
        "new 19x19 handicap game should place upper-left star point");
    require(
        model.currentPosition().sideToMove() == lizzie::core::Color::White,
        "new handicap game should infer white to move");

    const std::vector<lizzie::core::Point> nineByNine =
        lizzie::app::standardHandicapStones(lizzie::core::BoardSize::square(9), 5);
    require(nineByNine.size() == 5, "9x9 standard handicap should support center stones");
    require(containsPoint(nineByNine, lizzie::core::Point{2, 6}), "9x9 handicap should use third-line stars");
    require(containsPoint(nineByNine, lizzie::core::Point{4, 4}), "odd handicap should include center");

    lizzie::core::GameInfo rectangularInfo;
    rectangularInfo.handicap = 4;
    lizzie::core::GameModel rectangular =
        lizzie::app::createNewGameModel(lizzie::core::BoardSize{9, 13}, rectangularInfo);
    require(rectangular.root().setupStones.black.empty(), "rectangular new game should not invent non-standard handicap stones");
    require(
        !rectangular.gameInfo().handicap.has_value(),
        "rectangular new game should not preserve unsupported handicap metadata");

    lizzie::core::GameInfo invalidInfo;
    invalidInfo.handicap = 1;
    lizzie::core::GameModel invalid =
        lizzie::app::createNewGameModel(lizzie::core::BoardSize::square(19), invalidInfo);
    require(invalid.root().setupStones.black.empty(), "invalid new game handicap should not place stones");
    require(
        !invalid.gameInfo().handicap.has_value(),
        "invalid new game handicap should not preserve unsupported handicap metadata");
}

void testGameEditorAddsCurrentMoves()
{
    lizzie::core::GameModel model(lizzie::core::BoardSize::square(9));
    lizzie::app::GameEditor editor(&model);

    std::string error;
    const std::optional<lizzie::core::Color> rootSide = editor.currentSideToMove(&error);
    require(rootSide == lizzie::core::Color::Black, "game editor should report root side to move");
    require(error.empty(), "game editor side-to-move query should clear successful errors");

    const lizzie::app::MoveEditResult blackMove = editor.tryPlayAt(lizzie::core::Point{2, 2});
    require(blackMove.accepted, "game editor should add legal play moves");
    require(blackMove.nodeId.has_value(), "game editor should report added move node id");
    require(model.currentId() == *blackMove.nodeId, "game editor should select the added move");
    require(model.node(*blackMove.nodeId).move->color == lizzie::core::Color::Black, "game editor should use side to move");

    require(editor.setCurrent(model.rootId()), "test should return to root before replaying an existing branch");
    const lizzie::app::MoveEditResult replayedMove = editor.tryPlayAt(lizzie::core::Point{2, 2});
    require(replayedMove.accepted, "game editor should accept replaying an existing branch move");
    require(replayedMove.nodeId == blackMove.nodeId, "game editor should navigate to an existing matching branch");
    require(model.root().children.size() == 1, "game editor should not duplicate matching branch moves");
    require(editor.setCurrent(*blackMove.nodeId), "test should restore the first move before legality checks");

    const lizzie::app::MoveEditResult occupiedMove = editor.tryPlayAt(lizzie::core::Point{2, 2});
    require(!occupiedMove.accepted, "game editor should reject occupied moves");
    require(!occupiedMove.error.empty(), "game editor should report occupied move errors");
    require(model.currentId() == *blackMove.nodeId, "rejected moves should not change current node");

    const lizzie::app::MoveEditResult wrongColor =
        editor.tryAddMove(lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{3, 3}));
    require(!wrongColor.accepted, "game editor should reject explicit wrong-color engine moves");
    require(!wrongColor.error.empty(), "game editor should report wrong-color move errors");

    const lizzie::app::MoveEditResult whitePass = editor.tryPass();
    require(whitePass.accepted, "game editor should add pass moves");
    require(model.node(*whitePass.nodeId).move->type == lizzie::core::MoveType::Pass, "game editor should store pass moves");
    require(model.node(*whitePass.nodeId).move->color == lizzie::core::Color::White, "game editor should pass for side to move");

    const lizzie::app::MoveEditResult blackResign = editor.tryResign();
    require(blackResign.accepted, "game editor should add resign moves");
    require(
        model.node(*blackResign.nodeId).move->type == lizzie::core::MoveType::Resign,
        "game editor should store resign moves");
    require(
        model.node(*blackResign.nodeId).move->color == lizzie::core::Color::Black,
        "game editor should resign for side to move");
}

void testGameEditorUpdatesGameInfoAndReportsAnalysisInvalidation()
{
    lizzie::core::GameModel model(lizzie::core::BoardSize::square(9));
    lizzie::app::GameEditor editor(&model);

    lizzie::core::GameInfo info;
    info.blackPlayer = "Black";
    info.whitePlayer = "White";
    lizzie::app::GameInfoEditResult result = editor.setGameInfo(info);
    require(result.changed, "game editor should report game-info changes");
    require(!result.analysisInvalidated, "player-only game-info edits should not invalidate analysis");
    require(model.gameInfo().blackPlayer == "Black", "game editor should update black player");
    require(model.gameInfo().whitePlayer == "White", "game editor should update white player");

    result = editor.setGameInfo(info);
    require(!result.changed, "game editor should report unchanged game info");
    require(!result.analysisInvalidated, "unchanged game info should not invalidate analysis");

    info.blackRank = "1d";
    result = editor.setGameInfo(info);
    require(result.changed, "game editor should report non-position game-info changes");
    require(!result.analysisInvalidated, "rank-only game-info edits should not invalidate analysis");

    info.rules = "Japanese";
    result = editor.setGameInfo(info);
    require(result.changed, "game editor should report rule changes");
    require(result.analysisInvalidated, "rule changes should invalidate analysis");

    info.komi = 6.5;
    result = editor.setGameInfo(info);
    require(result.changed, "game editor should report komi changes");
    require(result.analysisInvalidated, "komi changes should invalidate analysis");

    info.handicap = 2;
    result = editor.setGameInfo(info);
    require(result.changed, "game editor should report handicap changes");
    require(result.analysisInvalidated, "handicap changes should invalidate analysis");
}

}  // namespace

int main()
{
    try {
        testSelfPlayContinuesAfterNormalMove();
        testEngineGameContinuesAfterNormalMove();
        testResignStopsSelfPlay();
        testResignStopsHumanVsAi();
        testResignStopsEngineGame();
        testDoublePassStopsSelfPlay();
        testDoublePassStopsHumanVsAi();
        testHumanMovePlansHumanVsAiReply();
        testDoublePassStopsEngineGame();
        testHumanVsAiToggleStopsConflictingAutomatedModes();
        testSelfPlayToggleStartsMoveAndStopsConflicts();
        testEngineGameToggleStopsAnalysisAndConflictingModes();
        testAutomatedModeTogglesStopWhenAlreadyActive();
        testAnalysisStreamGateWaitsForSyncResponses();
        testAnalysisStreamGateStopsOnSyncFailure();
        testEngineCommandDispatcherRecordsSyncCommandIds();
        testEngineCommandDispatcherSkipsRejectedSyncIds();
        testEngineCommandDispatcherRejectsEmptyCommands();
        testEngineCommandDispatcherStopsGateWhenSenderUnavailable();
        testEngineCommandDispatcherParsesConsoleCommands();
        testPositionRequestGuardConsumesOnlyMatchingPosition();
        testAnalysisStoreWritesSnapshotAndClearsError();
        testAnalysisStoreAppliesBatchResponseWithPositionKey();
        testAnalysisStoreWritesFailuresWithPositionKey();
        testEngineManagerOwnsStableEngineSessions();
        testRealtimeAnalysisCommandPlannerBuildsFullSyncPlan();
        testRealtimeAnalysisCommandPlannerSkipsUnsupportedExtensionCommands();
        testRealtimeAnalysisCommandPlannerOmitsFinalCommandForFatalPosition();
        testGenMoveCommandPlannerBuildsFinalMoveCommand();
        testGenMoveCommandPlannerKeepsFatalSyncDiagnosable();
        testBatchAnalysisRunPlannerBuildsEstimateRequest();
        testBatchAnalysisRunPlannerBuildsFullBatchRequests();
        testBatchAnalysisRunPlannerKeepsMidGameSetupSnapshotRequest();
        testBatchAnalysisTrackerTracksPendingRequests();
        testBatchAnalysisCompletionPlannerWritesBatchOutput();
        testBatchAnalysisCompletionPlannerSkipsOutputWhenCancelledOrUnsaved();
        testGameEditorUpdatesCommentsAndMarkup();
        testGameEditorSgfWriteClearsStaleStructuredProperties();
        testGameEditorEditsSetupAndTree();
        testNewGameSetupAppliesStandardHandicapStones();
        testGameEditorAddsCurrentMoves();
        testGameEditorUpdatesGameInfoAndReportsAnalysisInvalidation();
    } catch (const std::exception& error) {
        std::cerr << "engine_automation_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "engine_automation_tests passed\n";
    return EXIT_SUCCESS;
}
