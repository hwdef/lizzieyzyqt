#include "RealtimeAnalysis.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

namespace lizzie::engine {

namespace {

[[nodiscard]] bool sameMove(const lizzie::core::Move& left, const lizzie::core::Move& right)
{
    return left.color == right.color && left.type == right.type && (!left.isPlay() || left.point == right.point);
}

[[nodiscard]] std::string moveKey(const lizzie::core::Move& move, lizzie::core::BoardSize boardSize)
{
    switch (move.type) {
    case lizzie::core::MoveType::Play:
        return formatPointForGtp(move.point, boardSize);
    case lizzie::core::MoveType::Pass:
        return "pass";
    case lizzie::core::MoveType::Resign:
        return "resign";
    }
    return {};
}

[[nodiscard]] std::string lowercase(std::string_view text)
{
    std::string result(text);
    std::ranges::transform(result, result.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return result;
}

[[nodiscard]] double normalizedRate(double value)
{
    if (!std::isfinite(value)) {
        return 0.0;
    }
    const double magnitude = std::abs(value);
    if (magnitude > 100.0) {
        value /= 10000.0;
    } else if (magnitude > 1.0) {
        value /= 100.0;
    }
    return std::clamp(value, 0.0, 1.0);
}

[[nodiscard]] double finiteOrZero(double value)
{
    return std::isfinite(value) ? value : 0.0;
}

[[nodiscard]] bool hasCompleteFiniteOwnership(
    const std::vector<double>& ownership,
    lizzie::core::BoardSize boardSize)
{
    if (!boardSize.isValid()) {
        return false;
    }
    return ownership.size() == static_cast<std::size_t>(boardSize.pointCount()) &&
        std::ranges::all_of(ownership, [](double value) { return std::isfinite(value); });
}

}  // namespace

std::optional<lizzie::core::Move> moveFromGtpToken(
    std::string_view token,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color color)
{
    const std::string text = lowercase(token);
    if (text == "pass") {
        return lizzie::core::Move::pass(color);
    }
    if (text == "resign") {
        return lizzie::core::Move::resign(color);
    }
    if (const auto point = parseGtpPoint(token, boardSize)) {
        return lizzie::core::Move::play(color, *point);
    }
    return std::nullopt;
}

std::optional<lizzie::core::MoveCandidate> candidateFromKataAnalyzeInfo(
    const KataAnalyzeInfo& info,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color toMove)
{
    if (!boardSize.isValid()) {
        return std::nullopt;
    }
    const std::optional<lizzie::core::Move> move = moveFromGtpToken(info.move, boardSize, toMove);
    if (!move.has_value() || info.visits < 0) {
        return std::nullopt;
    }

    lizzie::core::MoveCandidate candidate{*move};
    candidate.visits = info.visits;
    candidate.winrate = normalizedRate(info.winrate);
    candidate.scoreMean = finiteOrZero(info.scoreMean);
    candidate.scoreStdev = finiteOrZero(info.scoreStdev);
    candidate.policy = normalizedRate(info.policy);
    if (hasCompleteFiniteOwnership(info.ownership, boardSize)) {
        candidate.ownership = info.ownership;
    }

    lizzie::core::Color pvColor = toMove;
    candidate.pv.reserve(info.pv.size());
    bool validPv = true;
    for (const std::string& token : info.pv) {
        if (const std::optional<lizzie::core::Move> pvMove = moveFromGtpToken(token, boardSize, pvColor)) {
            candidate.pv.push_back(*pvMove);
            pvColor = lizzie::core::opponent(pvColor);
        } else {
            validPv = false;
            candidate.pv.clear();
            break;
        }
    }
    if (validPv && std::ranges::all_of(info.pvVisits, [](int visits) { return visits >= 0; })) {
        candidate.pvVisits = info.pvVisits;
        if (!candidate.pvVisits.empty() && candidate.pvVisits.size() != candidate.pv.size()) {
            candidate.pvVisits.clear();
        }
    }
    return candidate;
}

RealtimeAnalysisAccumulator::RealtimeAnalysisAccumulator(lizzie::core::BoardSize boardSize, lizzie::core::Color toMove)
    : boardSize_(boardSize)
    , toMove_(toMove)
{
    snapshot_.winratePerspective = toMove_;
}

void RealtimeAnalysisAccumulator::reset(lizzie::core::BoardSize boardSize, lizzie::core::Color toMove)
{
    boardSize_ = boardSize;
    toMove_ = toMove;
    snapshot_ = lizzie::core::AnalysisSnapshot{};
    snapshot_.winratePerspective = toMove_;
    hasExplicitRootWinrate_ = false;
    hasExplicitRootScoreMean_ = false;
    hasExplicitRootVisits_ = false;
}

std::optional<lizzie::core::AnalysisSnapshot> RealtimeAnalysisAccumulator::processLine(std::string_view line)
{
    const KataAnalyzeLine update = parseKataAnalyzeLine(line);
    return processUpdate(update);
}

std::optional<lizzie::core::AnalysisSnapshot> RealtimeAnalysisAccumulator::processUpdate(const KataAnalyzeLine& update)
{
    if (!boardSize_.isValid()) {
        return std::nullopt;
    }

    bool changed = false;
    for (const KataAnalyzeInfo& info : update.infos) {
        const std::optional<lizzie::core::MoveCandidate> candidate =
            candidateFromKataAnalyzeInfo(info, boardSize_, toMove_);
        if (!candidate.has_value()) {
            continue;
        }
        mergeCandidate(*candidate);
        changed = true;
    }

    if (update.rootInfo.has_value()) {
        applyRootInfo(*update.rootInfo);
        changed = true;
    } else if (changed) {
        refreshRootInfo();
    }
    if (hasCompleteFiniteOwnership(update.ownership, boardSize_)) {
        snapshot_.ownership = update.ownership;
        changed = true;
    }

    if (!changed) {
        return std::nullopt;
    }
    return snapshot_;
}

std::optional<lizzie::core::AnalysisSnapshot> RealtimeAnalysisAccumulator::processInfo(const KataAnalyzeInfo& info)
{
    const std::optional<lizzie::core::MoveCandidate> candidate =
        candidateFromKataAnalyzeInfo(info, boardSize_, toMove_);
    if (!candidate.has_value()) {
        return std::nullopt;
    }

    mergeCandidate(*candidate);
    refreshRootInfo();
    return snapshot_;
}

const lizzie::core::AnalysisSnapshot& RealtimeAnalysisAccumulator::snapshot() const
{
    return snapshot_;
}

void RealtimeAnalysisAccumulator::mergeCandidate(lizzie::core::MoveCandidate candidate)
{
    if (hasCompleteFiniteOwnership(candidate.ownership, boardSize_)) {
        snapshot_.ownership = candidate.ownership;
    }

    const auto iter = std::ranges::find_if(snapshot_.candidates, [&](const lizzie::core::MoveCandidate& existing) {
        return sameMove(existing.move, candidate.move);
    });
    if (iter == snapshot_.candidates.end()) {
        snapshot_.candidates.push_back(std::move(candidate));
    } else {
        *iter = std::move(candidate);
    }

    std::ranges::sort(snapshot_.candidates, [&](const auto& left, const auto& right) {
        if (left.visits != right.visits) {
            return left.visits > right.visits;
        }
        return moveKey(left.move, boardSize_) < moveKey(right.move, boardSize_);
    });
}

void RealtimeAnalysisAccumulator::applyRootInfo(const KataAnalyzeRootInfo& rootInfo)
{
    if (rootInfo.hasWinrate && std::isfinite(rootInfo.winrate)) {
        snapshot_.rootWinrate = normalizedRate(rootInfo.winrate);
        hasExplicitRootWinrate_ = true;
    }
    if (rootInfo.hasScoreMean && std::isfinite(rootInfo.scoreMean)) {
        snapshot_.rootScoreMean = rootInfo.scoreMean;
        hasExplicitRootScoreMean_ = true;
    }
    if (rootInfo.hasVisits && rootInfo.visits >= 0) {
        snapshot_.rootVisits = rootInfo.visits;
        hasExplicitRootVisits_ = true;
    }
    if (hasCompleteFiniteOwnership(rootInfo.ownership, boardSize_)) {
        snapshot_.ownership = rootInfo.ownership;
    }
    refreshRootInfo();
}

void RealtimeAnalysisAccumulator::refreshRootInfo()
{
    if (snapshot_.candidates.empty()) {
        if (!hasExplicitRootWinrate_) {
            snapshot_.rootWinrate = 0.0;
        }
        if (!hasExplicitRootScoreMean_) {
            snapshot_.rootScoreMean = 0.0;
        }
        if (!hasExplicitRootVisits_) {
            snapshot_.rootVisits = 0;
        }
        return;
    }

    const lizzie::core::MoveCandidate& best = snapshot_.candidates.front();
    if (!hasExplicitRootWinrate_) {
        snapshot_.rootWinrate = best.winrate;
    }
    if (!hasExplicitRootScoreMean_) {
        snapshot_.rootScoreMean = best.scoreMean;
    }
    if (!hasExplicitRootVisits_) {
        snapshot_.rootVisits = best.visits;
    }
}

}  // namespace lizzie::engine
