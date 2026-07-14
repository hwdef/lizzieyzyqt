#pragma once

#include "AnalysisJson.h"
#include "GameModel.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace lizzie::engine {

struct BatchAnalysisOptions {
    std::optional<int> maxVisits;
    std::optional<int> maxPlayouts;
    std::optional<double> maxTime;
    std::optional<double> playoutDoublingAdvantage;
    std::optional<double> wideRootNoise;
    bool includeOwnership = true;
    std::optional<std::string> rulesOverride;
    std::optional<double> komiOverride;
};

struct BatchAnalysisItem {
    lizzie::core::NodeId nodeId = 0;
    AnalysisRequest request;
};

struct BatchAnalysisPlan {
    std::vector<BatchAnalysisItem> items;
    std::vector<std::string> warnings;
};

[[nodiscard]] std::vector<lizzie::core::NodeId> collectAnalysisNodeIds(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId root);

[[nodiscard]] BatchAnalysisPlan buildBatchAnalysisPlan(
    const lizzie::core::GameModel& model,
    const std::vector<lizzie::core::NodeId>& nodes,
    const BatchAnalysisOptions& options = {});

[[nodiscard]] std::string analysisRequestId(lizzie::core::NodeId nodeId);
[[nodiscard]] std::optional<lizzie::core::NodeId> nodeIdFromAnalysisRequestId(const std::string& id);
[[nodiscard]] std::string analysisPositionKey(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId);
[[nodiscard]] bool analysisPositionKeyMatches(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId,
    std::string_view positionKey);

[[nodiscard]] lizzie::core::AnalysisSnapshot analysisSnapshotFromResponse(
    const lizzie::core::GameModel& model,
    const AnalysisResponse& response);

[[nodiscard]] bool applyAnalysisResponse(
    lizzie::core::GameModel* model,
    const AnalysisResponse& response,
    std::string_view expectedPositionKey = {});

[[nodiscard]] QJsonObject analysisSidecarToJson(
    const lizzie::core::GameModel& model,
    const std::string& sgfPath = {},
    std::optional<int> collectionGameIndex = std::nullopt);

[[nodiscard]] int applyAnalysisSidecarJson(
    lizzie::core::GameModel* model,
    const QJsonObject& root,
    std::vector<std::string>* warnings = nullptr,
    std::optional<int> expectedCollectionGameIndex = std::nullopt);

[[nodiscard]] int importLegacyLizzieAnalysis(
    lizzie::core::GameModel* model,
    std::vector<std::string>* warnings = nullptr);

}  // namespace lizzie::engine
