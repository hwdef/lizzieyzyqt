#pragma once

#include "Types.h"

#include <QJsonObject>

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lizzie::engine {

struct InitialStone {
    lizzie::core::Color color = lizzie::core::Color::Black;
    lizzie::core::Point point{};
};

struct AnalysisRequest {
    std::string id;
    lizzie::core::BoardSize boardSize = lizzie::core::BoardSize::square(19);
    std::vector<lizzie::core::Move> moves;
    std::vector<InitialStone> initialStones;
    std::string rules = "Chinese";
    double komi = 7.5;
    std::optional<int> maxVisits;
    std::optional<int> maxPlayouts;
    std::optional<double> maxTime;
    bool includeOwnership = true;
    bool includePVVisits = true;
    std::optional<double> playoutDoublingAdvantage;
    std::optional<double> wideRootNoise;
    lizzie::core::Color initialPlayer = lizzie::core::Color::Black;

    [[nodiscard]] QJsonObject toJson() const;
    [[nodiscard]] std::string toJsonLine() const;
};

struct AnalysisResponse {
    std::string id;
    std::vector<lizzie::core::MoveCandidate> moveInfos;
    std::vector<double> ownership;
    double rootWinrate = 0.0;
    double rootScoreMean = 0.0;
    int rootVisits = 0;
    lizzie::core::Color winratePerspective = lizzie::core::Color::Black;
    std::string errorMessage;
    std::string rawJson;
};

[[nodiscard]] std::optional<AnalysisResponse> parseAnalysisResponse(
    std::string_view jsonLine,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color toMove);

}  // namespace lizzie::engine
