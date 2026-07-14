#pragma once

#include "GameModel.h"

#include <optional>
#include <string>
#include <string_view>

namespace lizzie::engine {
struct AnalysisResponse;
}

namespace lizzie::app {

enum class AnalysisStoreUpdateStatus {
    Applied,
    MissingModel,
    MissingNode,
    InvalidRequestId,
    StalePosition,
    RejectedResponse,
};

struct AnalysisStoreUpdateResult {
    AnalysisStoreUpdateStatus status = AnalysisStoreUpdateStatus::RejectedResponse;
    std::optional<lizzie::core::NodeId> nodeId;

    [[nodiscard]] bool applied() const;
};

class AnalysisStore {
public:
    explicit AnalysisStore(lizzie::core::GameModel* model = nullptr);

    void setModel(lizzie::core::GameModel* model);

    [[nodiscard]] bool setSnapshot(
        lizzie::core::NodeId nodeId,
        lizzie::core::AnalysisSnapshot snapshot);
    [[nodiscard]] AnalysisStoreUpdateResult applyResponse(
        const lizzie::engine::AnalysisResponse& response,
        std::optional<std::string_view> expectedPositionKey = std::nullopt);
    [[nodiscard]] AnalysisStoreUpdateResult setErrorForRequest(
        std::string_view requestId,
        std::string diagnostic,
        std::optional<std::string_view> expectedPositionKey = std::nullopt);
    [[nodiscard]] AnalysisStoreUpdateResult setErrorForNode(
        lizzie::core::NodeId nodeId,
        std::string diagnostic,
        std::optional<std::string_view> expectedPositionKey = std::nullopt);

    bool clearError(lizzie::core::NodeId nodeId);
    bool clearAt(lizzie::core::NodeId nodeId);
    bool clearFrom(lizzie::core::NodeId nodeId);
    void clearAll();

private:
    lizzie::core::GameModel* model_ = nullptr;
};

}  // namespace lizzie::app
