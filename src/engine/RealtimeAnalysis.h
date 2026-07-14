#pragma once

#include "GtpProtocol.h"
#include "Types.h"

#include <optional>
#include <string_view>
#include <vector>

namespace lizzie::engine {

[[nodiscard]] std::optional<lizzie::core::Move> moveFromGtpToken(
    std::string_view token,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color color);

[[nodiscard]] std::optional<lizzie::core::MoveCandidate> candidateFromKataAnalyzeInfo(
    const KataAnalyzeInfo& info,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color toMove);

class RealtimeAnalysisAccumulator {
public:
    RealtimeAnalysisAccumulator(
        lizzie::core::BoardSize boardSize = lizzie::core::BoardSize::square(19),
        lizzie::core::Color toMove = lizzie::core::Color::Black);

    void reset(lizzie::core::BoardSize boardSize, lizzie::core::Color toMove);
    [[nodiscard]] std::optional<lizzie::core::AnalysisSnapshot> processUpdate(const KataAnalyzeLine& update);
    [[nodiscard]] std::optional<lizzie::core::AnalysisSnapshot> processInfo(const KataAnalyzeInfo& info);
    [[nodiscard]] std::optional<lizzie::core::AnalysisSnapshot> processLine(std::string_view line);
    [[nodiscard]] const lizzie::core::AnalysisSnapshot& snapshot() const;

private:
    void mergeCandidate(lizzie::core::MoveCandidate candidate);
    void applyRootInfo(const KataAnalyzeRootInfo& rootInfo);
    void refreshRootInfo();

    lizzie::core::BoardSize boardSize_;
    lizzie::core::Color toMove_;
    lizzie::core::AnalysisSnapshot snapshot_;
    bool hasExplicitRootWinrate_ = false;
    bool hasExplicitRootScoreMean_ = false;
    bool hasExplicitRootVisits_ = false;
};

}  // namespace lizzie::engine
