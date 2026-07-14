#include "GtpCommandQueue.h"
#include "RealtimeAnalysis.h"

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

void testCommandQueue()
{
    GtpCommandQueue queue;
    const GtpCommand first = queue.enqueue(GtpCommand{std::nullopt, "name", {}});
    const GtpCommand second = queue.enqueue(GtpCommand{42, "version", {}});

    require(first.id == 1, "queue should assign first id");
    require(second.id == 42, "queue should preserve explicit id");
    require(queue.nextCommandId() == 43, "queue should advance past explicit id");
    require(queue.pendingCount() == 2, "queue should track pending commands");

    GtpResponse versionResponse;
    versionResponse.success = true;
    versionResponse.id = 42;
    versionResponse.payload = "1.15";
    const QueuedGtpResponse matchedVersion = queue.resolve(versionResponse);
    require(matchedVersion.command.has_value(), "response id should match a queued command");
    require(matchedVersion.command->name == "version", "matched command name should be preserved");
    require(queue.pendingCount() == 1, "matched command should be removed");

    GtpResponse nameResponse;
    nameResponse.success = true;
    nameResponse.payload = "KataGo";
    const QueuedGtpResponse matchedName = queue.resolve(nameResponse);
    require(matchedName.command.has_value() && matchedName.command->name == "name", "un-id response should match oldest");
    require(queue.pendingCount() == 0, "queue should be empty");
}

void testCandidateConversion()
{
    const auto info = parseKataAnalyzeInfo(
        "info move D4 visits 100 winrate 0.53 scoreLead 1.25 scoreStdev 7.5 prior 0.12 pv D4 Q16 pass pvVisits 100 60 20 ownership 0.1 -0.1");
    require(info.has_value(), "analysis info should parse");
    const auto candidate = candidateFromKataAnalyzeInfo(*info, BoardSize::square(19), Color::Black);
    require(candidate.has_value(), "analysis info should convert to candidate");
    require(candidate->move.type == MoveType::Play && candidate->move.point == Point{3, 15}, "candidate move should parse");
    require(candidate->scoreMean == 1.25, "scoreLead should convert to candidate score mean");
    require(candidate->policy == 0.12, "prior should convert to candidate policy");
    require(candidate->pv.size() == 3, "PV should convert");
    require(candidate->pv[0].color == Color::Black, "PV should start with side to move");
    require(candidate->pv[1].color == Color::White, "PV should alternate colors");
    require(candidate->pv[2].type == MoveType::Pass, "PV pass should convert");
    require(candidate->pvVisits.size() == 3 && candidate->pvVisits[1] == 60, "PV visits should convert");
    require(candidate->ownership.empty(), "incomplete candidate ownership should be discarded during conversion");

    const auto completeOwnershipInfo = parseKataAnalyzeInfo(
        "info move A2 visits 10 winrate 0.50 scoreMean 0.0 scoreStdev 1.0 policy 0.2 pv A2 "
        "ownership 0.1 -0.1 0.2 -0.2");
    require(completeOwnershipInfo.has_value(), "complete candidate ownership info should parse");
    const auto completeOwnershipCandidate =
        candidateFromKataAnalyzeInfo(*completeOwnershipInfo, BoardSize::square(2), Color::Black);
    require(completeOwnershipCandidate.has_value(), "complete candidate ownership info should convert");
    require(
        completeOwnershipCandidate->ownership.size() == 4 &&
            completeOwnershipCandidate->ownership[0] == 0.1 &&
            completeOwnershipCandidate->ownership[1] == -0.1,
        "complete candidate ownership should be preserved during conversion");

    KataAnalyzeInfo directNegativeVisits;
    directNegativeVisits.move = "A1";
    directNegativeVisits.visits = -1;
    require(
        !candidateFromKataAnalyzeInfo(directNegativeVisits, BoardSize::square(2), Color::Black).has_value(),
        "direct candidate conversion should reject negative visits");

    KataAnalyzeInfo invalidBoardInfo;
    invalidBoardInfo.move = "pass";
    invalidBoardInfo.visits = 5;
    require(
        !candidateFromKataAnalyzeInfo(invalidBoardInfo, BoardSize{0, 53}, Color::Black).has_value(),
        "direct candidate conversion should reject invalid board sizes");

    KataAnalyzeInfo directNoisyInfo;
    directNoisyInfo.move = "A1";
    directNoisyInfo.visits = 4;
    directNoisyInfo.winrate = std::numeric_limits<double>::infinity();
    directNoisyInfo.scoreMean = std::numeric_limits<double>::quiet_NaN();
    directNoisyInfo.scoreStdev = -std::numeric_limits<double>::infinity();
    directNoisyInfo.policy = std::numeric_limits<double>::infinity();
    directNoisyInfo.pv = {"A1", "pass", "B2"};
    directNoisyInfo.pvVisits = {4, -1, 2};
    directNoisyInfo.ownership = {0.1, std::numeric_limits<double>::infinity(), 0.2, -0.2};
    const auto directNoisyCandidate =
        candidateFromKataAnalyzeInfo(directNoisyInfo, BoardSize::square(2), Color::Black);
    require(directNoisyCandidate.has_value(), "direct candidate with finite visits should convert");
    require(directNoisyCandidate->winrate == 0.0, "direct non-finite winrate should be sanitized");
    require(directNoisyCandidate->scoreMean == 0.0, "direct non-finite score should be sanitized");
    require(directNoisyCandidate->scoreStdev == 0.0, "direct non-finite score stdev should be sanitized");
    require(directNoisyCandidate->policy == 0.0, "direct non-finite policy should be sanitized");
    require(directNoisyCandidate->ownership.empty(), "direct non-finite ownership should be discarded");
    require(directNoisyCandidate->pv.size() == 3, "direct valid PV should be preserved");
    require(directNoisyCandidate->pvVisits.empty(), "direct negative PV visits should discard pvVisits");

    KataAnalyzeInfo directMalformedPvInfo;
    directMalformedPvInfo.move = "A1";
    directMalformedPvInfo.visits = 4;
    directMalformedPvInfo.pv = {"A1", "not-a-point", "pass"};
    directMalformedPvInfo.pvVisits = {4, 2, 1};
    const auto directMalformedPvCandidate =
        candidateFromKataAnalyzeInfo(directMalformedPvInfo, BoardSize::square(2), Color::Black);
    require(directMalformedPvCandidate.has_value(), "direct candidate with malformed PV should still convert");
    require(directMalformedPvCandidate->pv.empty(), "direct malformed PV should be discarded");
    require(directMalformedPvCandidate->pvVisits.empty(), "direct PV visits should be discarded after malformed PV");

    KataAnalyzeInfo mismatchedPvVisitsInfo;
    mismatchedPvVisitsInfo.move = "A1";
    mismatchedPvVisitsInfo.visits = 4;
    mismatchedPvVisitsInfo.pv = {"A1", "pass"};
    mismatchedPvVisitsInfo.pvVisits = {4};
    const auto mismatchedPvVisitsCandidate =
        candidateFromKataAnalyzeInfo(mismatchedPvVisitsInfo, BoardSize::square(2), Color::Black);
    require(mismatchedPvVisitsCandidate.has_value(), "direct candidate with mismatched PV visits should convert");
    require(
        mismatchedPvVisitsCandidate->pv.size() == 2,
        "direct mismatched PV visits should keep valid PV");
    require(
        mismatchedPvVisitsCandidate->pvVisits.empty(),
        "direct mismatched PV visits should be discarded");

    const auto legacyScaleInfo = parseKataAnalyzeInfo(
        "info move Q16 visits 50 winrate 5234 scoreLead 0.5 scoreStdev 6.0 prior 1234 pv Q16");
    require(legacyScaleInfo.has_value(), "legacy-scale analysis info should parse");
    const auto legacyScaleCandidate = candidateFromKataAnalyzeInfo(*legacyScaleInfo, BoardSize::square(19), Color::White);
    require(legacyScaleCandidate.has_value(), "legacy-scale analysis info should convert to candidate");
    require(
        legacyScaleCandidate->winrate > 0.5233 && legacyScaleCandidate->winrate < 0.5235,
        "legacy-scale GTP winrate should normalize to 0..1");
    require(
        legacyScaleCandidate->policy > 0.1233 && legacyScaleCandidate->policy < 0.1235,
        "legacy-scale GTP prior should normalize to 0..1 policy");

    const auto percentScaleInfo = parseKataAnalyzeInfo(
        "info move C3 visits 70 winrate 52.34 scoreLead 0.4 scoreStdev 5.0 prior 12.34 pv C3");
    require(percentScaleInfo.has_value(), "percent-scale analysis info should parse");
    const auto percentScaleCandidate = candidateFromKataAnalyzeInfo(*percentScaleInfo, BoardSize::square(19), Color::Black);
    require(percentScaleCandidate.has_value(), "percent-scale analysis info should convert to candidate");
    require(
        percentScaleCandidate->winrate > 0.5233 && percentScaleCandidate->winrate < 0.5235,
        "percent-scale GTP winrate should normalize to 0..1");
    require(
        percentScaleCandidate->policy > 0.1233 && percentScaleCandidate->policy < 0.1235,
        "percent-scale GTP prior should normalize to policy");

    const auto malformedPvInfo = parseKataAnalyzeInfo(
        "info move D4 visits 42 winrate 0.52 scoreMean 1.0 scoreStdev 4.0 prior 0.1 pv D4 ?? Q16 pass");
    require(malformedPvInfo.has_value(), "analysis info with malformed PV token should parse");
    const auto malformedPvCandidate =
        candidateFromKataAnalyzeInfo(*malformedPvInfo, BoardSize::square(19), Color::Black);
    require(malformedPvCandidate.has_value(), "analysis info with malformed PV token should convert");
    require(malformedPvCandidate->pv.empty(), "malformed realtime PV should be discarded");
    require(malformedPvCandidate->pvVisits.empty(), "malformed realtime PV should not retain PV visits");

    const auto mixedPass = moveFromGtpToken("PaSs", BoardSize::square(19), Color::White);
    require(mixedPass.has_value(), "mixed-case pass token should convert");
    require(
        mixedPass->type == MoveType::Pass && mixedPass->color == Color::White,
        "mixed-case pass token should preserve color");
    const auto mixedResign = moveFromGtpToken("ReSiGn", BoardSize::square(19), Color::Black);
    require(mixedResign.has_value(), "mixed-case resign token should convert");
    require(
        mixedResign->type == MoveType::Resign && mixedResign->color == Color::Black,
        "mixed-case resign token should preserve color");

    const auto mixedCaseSpecialInfo = parseKataAnalyzeInfo(
        "info move ReSiGn visits 9 winrate 0.25 scoreLead -8.0 scoreStdev 3.0 prior 0.01 pv PaSs ReSiGn");
    require(mixedCaseSpecialInfo.has_value(), "mixed-case special move analysis info should parse");
    const auto mixedCaseSpecialCandidate =
        candidateFromKataAnalyzeInfo(*mixedCaseSpecialInfo, BoardSize::square(19), Color::White);
    require(mixedCaseSpecialCandidate.has_value(), "mixed-case special move analysis info should convert");
    require(
        mixedCaseSpecialCandidate->move.type == MoveType::Resign &&
            mixedCaseSpecialCandidate->move.color == Color::White,
        "mixed-case resign candidate move should preserve side to move");
    require(mixedCaseSpecialCandidate->pv.size() == 2, "mixed-case special PV should convert");
    require(
        mixedCaseSpecialCandidate->pv[0].type == MoveType::Pass &&
            mixedCaseSpecialCandidate->pv[0].color == Color::White,
        "mixed-case pass PV move should preserve current side");
    require(
        mixedCaseSpecialCandidate->pv[1].type == MoveType::Resign &&
            mixedCaseSpecialCandidate->pv[1].color == Color::Black,
        "mixed-case resign PV move should alternate side");
}

void testAccumulator()
{
    RealtimeAnalysisAccumulator accumulator(BoardSize::square(2), Color::White);
    require(!accumulator.processLine("engine log").has_value(), "non-analysis line should be ignored");

    const auto first = accumulator.processLine(
        "info move A2 visits 20 winrate 0.44 scoreMean -1.5 scoreStdev 6.0 policy 0.08 pv A2 "
        "ownership 0.2 -0.2 0.1 -0.1 "
        "rootInfo visits 88 winrate 0.62 scoreLead 2.4 ownership 0.3 -0.3 0.2 -0.2");
    require(first.has_value(), "first candidate should produce a snapshot");
    require(first->candidates.size() == 1, "snapshot should contain first candidate");
    require(first->ownership.size() == 4, "root ownership should update from full-board ownership");
    require(first->ownership[0] == 0.3 && first->ownership[1] == -0.3, "rootInfo ownership should drive snapshot ownership");
    require(first->rootVisits == 88, "root visits should use rootInfo when present");
    require(first->rootWinrate == 0.62, "root winrate should use rootInfo when present");
    require(first->rootScoreMean == 2.4, "root score should use rootInfo when present");

    const auto invalidVisits = accumulator.processLine(
        "info move B1 visits -1 winrate 0.45 scoreMean -0.9 scoreStdev 5.0 policy 0.10 pv B1");
    require(!invalidVisits.has_value(), "candidate with invalid realtime visits should be ignored");
    require(accumulator.snapshot().candidates.size() == 1, "invalid realtime visits should not add a candidate");

    const auto nonFiniteNumeric = accumulator.processLine(
        "info move B1 visits 33 winrate nan scoreLead inf scoreStdev -inf prior inf pv B1 "
        "rootInfo winrate nan scoreLead inf ownership nan");
    require(nonFiniteNumeric.has_value(), "candidate with finite visits should update despite non-finite scalar fields");
    require(nonFiniteNumeric->candidates.front().move.point == Point{1, 1}, "finite candidate should be retained");
    require(nonFiniteNumeric->candidates.front().visits == 33, "finite candidate visits should be retained");
    require(nonFiniteNumeric->candidates.front().winrate == 0.0, "non-finite candidate winrate should not enter snapshot");
    require(nonFiniteNumeric->candidates.front().scoreMean == 0.0, "non-finite candidate score should not enter snapshot");
    require(
        nonFiniteNumeric->candidates.front().scoreStdev == 0.0,
        "non-finite candidate score stdev should not enter snapshot");
    require(nonFiniteNumeric->candidates.front().policy == 0.0, "non-finite candidate policy should not enter snapshot");
    require(nonFiniteNumeric->rootVisits == 88, "non-finite rootInfo should not overwrite prior root visits");
    require(nonFiniteNumeric->rootWinrate == 0.62, "non-finite rootInfo should not overwrite prior root winrate");
    require(nonFiniteNumeric->rootScoreMean == 2.4, "non-finite rootInfo should not overwrite prior root score");

    const auto invalidRootVisits = accumulator.processLine(
        "info move B1 visits 35 winrate 0.48 scoreMean -0.4 scoreStdev 5.0 policy 0.12 pv B1 "
        "rootInfo visits -3 winrate 0.66 scoreLead 3.0");
    require(invalidRootVisits.has_value(), "valid candidate with invalid root visits should still update");
    require(invalidRootVisits->rootVisits == 88, "invalid rootInfo visits should not overwrite prior root visits");
    require(invalidRootVisits->rootWinrate == 0.66, "rootInfo winrate should remain usable with invalid visits");
    require(invalidRootVisits->rootScoreMean == 3.0, "rootInfo score should remain usable with invalid visits");

    const auto topLevelOwnership = accumulator.processLine(
        "ownership 0.4 -0.4 0.2 -0.2 "
        "info move A2 visits 30 winrate 0.49 scoreMean -0.2 scoreStdev 5.5 policy 0.11 pv A2 "
        "rootInfo visits 88 winrate 0.62 scoreLead 2.4");
    require(topLevelOwnership.has_value(), "top-level ownership line should produce a snapshot");
    require(
        topLevelOwnership->ownership.size() == 4 &&
            topLevelOwnership->ownership[0] == 0.4 &&
            topLevelOwnership->ownership[1] == -0.4,
        "top-level ownership should drive snapshot ownership");
    require(topLevelOwnership->rootVisits == 88, "top-level ownership line should still parse rootInfo");

    const auto topLevelOwnershipPrecedence = accumulator.processLine(
        "ownership 0.45 -0.45 0.25 -0.25 "
        "rootInfo visits 89 winrate 0.63 scoreLead 2.6 ownership 0.1 -0.1 0.05 -0.05");
    require(topLevelOwnershipPrecedence.has_value(), "top-level ownership with rootInfo ownership should update");
    require(
        topLevelOwnershipPrecedence->ownership.size() == 4 &&
            topLevelOwnershipPrecedence->ownership[0] == 0.45 &&
            topLevelOwnershipPrecedence->ownership[1] == -0.45,
        "standard top-level ownership should remain the snapshot ownership when rootInfo also has ownership");
    require(topLevelOwnershipPrecedence->rootVisits == 89, "rootInfo should still update root visits");

    const auto candidateOwnershipPrecedence = accumulator.processLine(
        "info move A2 visits 32 winrate 0.50 scoreMean 0.0 scoreStdev 5.2 policy 0.10 pv A2 "
        "movesOwnership 0.6 -0.6 0.3 -0.3 ownership 0.1 -0.1 0.05 -0.05");
    require(candidateOwnershipPrecedence.has_value(), "candidate with ownership aliases should update");
    require(
        candidateOwnershipPrecedence->ownership.size() == 4 &&
            candidateOwnershipPrecedence->ownership[0] == 0.6 &&
            candidateOwnershipPrecedence->ownership[1] == -0.6,
        "movesOwnership should remain the snapshot ownership when a compatible alias follows it");

    const auto ownershipOnly = accumulator.processLine("ownership -0.4 0.4 -0.2 0.2");
    require(ownershipOnly.has_value(), "ownership-only update should produce a snapshot");
    require(
        ownershipOnly->ownership.size() == 4 &&
            ownershipOnly->ownership[0] == -0.4 &&
            ownershipOnly->ownership[1] == 0.4,
        "ownership-only update should drive snapshot ownership");
    require(ownershipOnly->rootVisits == 89, "ownership-only update should preserve rootInfo visits");

    const auto second = accumulator.processLine(
        "info move B1 visits 40 winrate 0.51 scoreMean 0.5 scoreStdev 5.0 policy 0.15 pv B1 A1");
    require(second.has_value(), "second candidate should produce a snapshot");
    require(second->candidates.size() == 2, "snapshot should contain two candidates");
    require(second->candidates.front().visits == 40, "candidates should sort by visits");
    require(second->rootVisits == 89, "candidate-only updates should not overwrite rootInfo visits");
    require(second->rootWinrate == 0.63, "candidate-only updates should not overwrite rootInfo winrate");
    require(second->rootScoreMean == 2.6, "candidate-only updates should not overwrite rootInfo score");

    const auto updated = accumulator.processLine(
        "info move B1 visits 60 winrate 0.55 scoreMean 1.0 scoreStdev 4.0 policy 0.2 pv B1 A1");
    require(updated.has_value(), "candidate update should produce a snapshot");
    require(updated->candidates.size() == 2, "candidate update should replace, not duplicate");
    require(updated->candidates.front().visits == 60, "updated candidate should be used");
    require(updated->rootVisits == 89, "candidate refresh should preserve last rootInfo visits");
    require(updated->rootWinrate == 0.63, "candidate refresh should preserve last rootInfo winrate");
    require(updated->rootScoreMean == 2.6, "candidate refresh should preserve last rootInfo score");

    const auto legacyScale = accumulator.processLine(
        "info move A2 visits 100 winrate 6123 scoreMean 1.2 scoreStdev 4.0 prior 2222 pv A2 "
        "rootInfo visits 120 winrate 6345 scoreLead 1.4");
    require(legacyScale.has_value(), "legacy-scale realtime line should produce a snapshot");
    require(legacyScale->rootVisits == 120, "new rootInfo should replace preserved root visits");
    require(legacyScale->rootScoreMean == 1.4, "new rootInfo should replace preserved root score");
    require(
        legacyScale->rootWinrate > 0.6344 && legacyScale->rootWinrate < 0.6346,
        "legacy-scale root winrate should normalize to 0..1");
    require(
        legacyScale->candidates.front().winrate > 0.6122 && legacyScale->candidates.front().winrate < 0.6124,
        "legacy-scale candidate winrate should normalize to 0..1");
    require(
        legacyScale->candidates.front().policy > 0.2221 && legacyScale->candidates.front().policy < 0.2223,
        "legacy-scale candidate prior should normalize to policy");

    const auto percentScale = accumulator.processLine(
        "info move B1 visits 140 winrate 61.23 scoreMean 1.6 scoreStdev 4.0 prior 22.22 pv B1 "
        "rootInfo visits 160 winrate 63.45 scoreLead 1.8");
    require(percentScale.has_value(), "percent-scale realtime line should produce a snapshot");
    require(percentScale->rootVisits == 160, "percent-scale rootInfo should replace preserved root visits");
    require(percentScale->rootScoreMean == 1.8, "percent-scale rootInfo should replace preserved root score");
    require(
        percentScale->rootWinrate > 0.6344 && percentScale->rootWinrate < 0.6346,
        "percent-scale root winrate should normalize to 0..1");
    require(
        percentScale->candidates.front().winrate > 0.6122 && percentScale->candidates.front().winrate < 0.6124,
        "percent-scale candidate winrate should normalize to 0..1");
    require(
        percentScale->candidates.front().policy > 0.2221 && percentScale->candidates.front().policy < 0.2223,
        "percent-scale candidate prior should normalize to policy");

    accumulator.reset(BoardSize::square(2), Color::Black);
    const auto fallback = accumulator.processLine(
        "info move A1 visits 7 winrate 0.41 scoreMean -0.7 scoreStdev 2.0 policy 0.2 pv A1");
    require(fallback.has_value(), "candidate-only line after reset should produce a snapshot");
    require(fallback->winratePerspective == Color::Black, "reset should update winrate perspective");
    require(fallback->rootVisits == 7, "reset should restore candidate root fallback");
    require(fallback->rootWinrate == 0.41, "candidate fallback winrate should work before rootInfo");
    require(fallback->rootScoreMean == -0.7, "candidate fallback score should work before rootInfo");

    RealtimeAnalysisAccumulator partialRootAccumulator(BoardSize::square(2), Color::Black);
    const auto partialRoot = partialRootAccumulator.processLine(
        "rootInfo visits 200 ownership 0.1 -0.1 0.2 -0.2");
    require(partialRoot.has_value(), "partial rootInfo should produce a snapshot");
    require(partialRoot->rootVisits == 200, "partial rootInfo visits should be applied immediately");
    require(partialRoot->rootWinrate == 0.0, "missing rootInfo winrate should stay neutral before candidates");
    require(partialRoot->rootScoreMean == 0.0, "missing rootInfo score should stay neutral before candidates");

    const auto partialRootCandidate = partialRootAccumulator.processLine(
        "info move A1 visits 60 winrate 0.54 scoreMean 1.6 scoreStdev 3.0 policy 0.12 pv A1");
    require(partialRootCandidate.has_value(), "candidate after partial rootInfo should update");
    require(partialRootCandidate->rootVisits == 200, "explicit root visits should remain authoritative");
    require(
        partialRootCandidate->rootWinrate == 0.54,
        "missing rootInfo winrate should fall back to the best candidate");
    require(
        partialRootCandidate->rootScoreMean == 1.6,
        "missing rootInfo score should fall back to the best candidate");

    const auto rootWinrateOnly = partialRootAccumulator.processLine("rootInfo winrate 0.61");
    require(rootWinrateOnly.has_value(), "rootInfo with only winrate should update");
    require(rootWinrateOnly->rootVisits == 200, "rootInfo missing visits should preserve explicit visits");
    require(rootWinrateOnly->rootWinrate == 0.61, "explicit root winrate should replace fallback winrate");
    require(rootWinrateOnly->rootScoreMean == 1.6, "missing root score should keep best-candidate fallback");

    const auto betterCandidate = partialRootAccumulator.processLine(
        "info move B1 visits 80 winrate 0.49 scoreMean -0.2 scoreStdev 2.8 policy 0.10 pv B1");
    require(betterCandidate.has_value(), "better candidate after partial rootInfo should update");
    require(betterCandidate->rootVisits == 200, "explicit root visits should survive candidate resorting");
    require(betterCandidate->rootWinrate == 0.61, "explicit root winrate should survive candidate resorting");
    require(
        betterCandidate->rootScoreMean == -0.2,
        "missing root score should continue following the best candidate");

    RealtimeAnalysisAccumulator directOwnershipAccumulator(BoardSize::square(2), Color::Black);
    KataAnalyzeLine directOwnership;
    directOwnership.ownership = {0.2, -0.2, 0.1, -0.1};
    const auto directOwnershipSnapshot = directOwnershipAccumulator.processUpdate(directOwnership);
    require(directOwnershipSnapshot.has_value(), "direct complete top-level ownership should update");
    require(
        directOwnershipSnapshot->ownership.size() == 4 && directOwnershipSnapshot->ownership[0] == 0.2 &&
            directOwnershipSnapshot->ownership[1] == -0.2,
        "direct complete top-level ownership should enter the snapshot");

    KataAnalyzeLine directNoisyOwnership;
    directNoisyOwnership.ownership = {0.5, std::numeric_limits<double>::infinity(), 0.3, -0.3};
    require(
        !directOwnershipAccumulator.processUpdate(directNoisyOwnership).has_value(),
        "direct non-finite top-level ownership alone should not update");
    require(
        directOwnershipAccumulator.snapshot().ownership.size() == 4 &&
            directOwnershipAccumulator.snapshot().ownership[0] == 0.2,
        "direct non-finite top-level ownership should not replace previous ownership");

    KataAnalyzeLine directNoisyRootOwnership;
    KataAnalyzeRootInfo noisyRootInfo;
    noisyRootInfo.hasVisits = true;
    noisyRootInfo.visits = 12;
    noisyRootInfo.ownership = {0.6, -0.6, std::numeric_limits<double>::quiet_NaN(), -0.4};
    directNoisyRootOwnership.rootInfo = noisyRootInfo;
    const auto noisyRootOwnershipSnapshot = directOwnershipAccumulator.processUpdate(directNoisyRootOwnership);
    require(noisyRootOwnershipSnapshot.has_value(), "direct rootInfo visits should update despite noisy ownership");
    require(noisyRootOwnershipSnapshot->rootVisits == 12, "direct rootInfo visits should be applied");
    require(
        noisyRootOwnershipSnapshot->ownership.size() == 4 && noisyRootOwnershipSnapshot->ownership[0] == 0.2,
        "direct non-finite rootInfo ownership should not replace previous ownership");

    KataAnalyzeLine directFiniteRootOwnership;
    KataAnalyzeRootInfo finiteRootInfo;
    finiteRootInfo.hasVisits = true;
    finiteRootInfo.visits = 13;
    finiteRootInfo.hasWinrate = true;
    finiteRootInfo.winrate = 0.58;
    finiteRootInfo.hasScoreMean = true;
    finiteRootInfo.scoreMean = 1.4;
    finiteRootInfo.ownership = {-0.3, 0.3, -0.1, 0.1};
    directFiniteRootOwnership.rootInfo = finiteRootInfo;
    const auto finiteRootOwnershipSnapshot = directOwnershipAccumulator.processUpdate(directFiniteRootOwnership);
    require(finiteRootOwnershipSnapshot.has_value(), "direct finite rootInfo ownership should update");
    require(finiteRootOwnershipSnapshot->rootVisits == 13, "direct finite rootInfo visits should replace prior visits");
    require(finiteRootOwnershipSnapshot->rootWinrate == 0.58, "direct finite rootInfo winrate should replace prior winrate");
    require(finiteRootOwnershipSnapshot->rootScoreMean == 1.4, "direct finite rootInfo score should replace prior score");
    require(
        finiteRootOwnershipSnapshot->ownership.size() == 4 &&
            finiteRootOwnershipSnapshot->ownership[0] == -0.3 &&
            finiteRootOwnershipSnapshot->ownership[1] == 0.3,
        "direct finite rootInfo ownership should replace previous ownership");

    KataAnalyzeLine directNoisyRootScalars;
    KataAnalyzeRootInfo noisyRootScalars;
    noisyRootScalars.hasVisits = true;
    noisyRootScalars.visits = -5;
    noisyRootScalars.hasWinrate = true;
    noisyRootScalars.winrate = std::numeric_limits<double>::quiet_NaN();
    noisyRootScalars.hasScoreMean = true;
    noisyRootScalars.scoreMean = std::numeric_limits<double>::infinity();
    directNoisyRootScalars.rootInfo = noisyRootScalars;
    const auto noisyRootScalarSnapshot = directOwnershipAccumulator.processUpdate(directNoisyRootScalars);
    require(noisyRootScalarSnapshot.has_value(), "direct noisy rootInfo should remain diagnosable");
    require(noisyRootScalarSnapshot->rootVisits == 13, "direct negative root visits should not replace prior visits");
    require(noisyRootScalarSnapshot->rootWinrate == 0.58, "direct non-finite root winrate should not replace prior winrate");
    require(noisyRootScalarSnapshot->rootScoreMean == 1.4, "direct non-finite root score should not replace prior score");
}

void testAccumulatorRejectsInvalidBoardSize()
{
    RealtimeAnalysisAccumulator accumulator(BoardSize{0, 53}, Color::Black);

    const auto snapshot = accumulator.processLine(
        "ownership 0.1 -0.1 "
        "info move pass visits 12 winrate 0.52 scoreMean 1.5 pv pass "
        "rootInfo visits 90 winrate 0.60 scoreLead 2.0 ownership 0.2 -0.2");
    require(!snapshot.has_value(), "invalid-board realtime update should not produce a snapshot");
    require(accumulator.snapshot().candidates.empty(), "invalid-board realtime update should not keep candidates");
    require(accumulator.snapshot().ownership.empty(), "invalid-board realtime update should not keep ownership");
    require(accumulator.snapshot().rootVisits == 0, "invalid-board realtime update should not keep root visits");
    require(accumulator.snapshot().rootWinrate == 0.0, "invalid-board realtime update should not keep root winrate");
    require(accumulator.snapshot().rootScoreMean == 0.0, "invalid-board realtime update should not keep root score");

    accumulator.reset(BoardSize::square(2), Color::White);
    require(
        accumulator.processLine("info move A2 visits 7 winrate 0.51 scoreMean 0.2 pv A2").has_value(),
        "reset to a valid board should resume realtime accumulation");
}

}  // namespace

int main()
{
    try {
        testCommandQueue();
        testCandidateConversion();
        testAccumulator();
        testAccumulatorRejectsInvalidBoardSize();
    } catch (const std::exception& error) {
        std::cerr << "realtime_analysis_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "realtime_analysis_tests passed\n";
    return EXIT_SUCCESS;
}
