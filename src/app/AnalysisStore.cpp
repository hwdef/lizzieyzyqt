#include "AnalysisStore.h"

#include "BatchAnalysis.h"
#include "AnalysisJson.h"

#include <utility>

namespace lizzie::app {

bool AnalysisStoreUpdateResult::applied() const
{
    return status == AnalysisStoreUpdateStatus::Applied;
}

AnalysisStore::AnalysisStore(lizzie::core::GameModel* model)
    : model_(model)
{
}

void AnalysisStore::setModel(lizzie::core::GameModel* model)
{
    model_ = model;
}

bool AnalysisStore::setSnapshot(
    lizzie::core::NodeId nodeId,
    lizzie::core::AnalysisSnapshot snapshot)
{
    if (model_ == nullptr) {
        return false;
    }
    lizzie::core::GameNode* node = model_->findNode(nodeId);
    if (node == nullptr) {
        return false;
    }
    node->analysis = std::move(snapshot);
    node->analysisError.clear();
    return true;
}

AnalysisStoreUpdateResult AnalysisStore::applyResponse(
    const lizzie::engine::AnalysisResponse& response,
    std::optional<std::string_view> expectedPositionKey)
{
    if (model_ == nullptr) {
        return {AnalysisStoreUpdateStatus::MissingModel, std::nullopt};
    }
    if (!response.errorMessage.empty()) {
        return {AnalysisStoreUpdateStatus::RejectedResponse, std::nullopt};
    }
    const std::optional<lizzie::core::NodeId> nodeId =
        lizzie::engine::nodeIdFromAnalysisRequestId(response.id);
    if (!nodeId.has_value()) {
        return {AnalysisStoreUpdateStatus::InvalidRequestId, std::nullopt};
    }
    lizzie::core::GameNode* node = model_->findNode(*nodeId);
    if (node == nullptr) {
        return {AnalysisStoreUpdateStatus::MissingNode, nodeId};
    }
    if (expectedPositionKey.has_value() &&
        !expectedPositionKey->empty() &&
        !lizzie::engine::analysisPositionKeyMatches(*model_, *nodeId, *expectedPositionKey)) {
        return {AnalysisStoreUpdateStatus::StalePosition, nodeId};
    }

    node->analysis = lizzie::engine::analysisSnapshotFromResponse(*model_, response);
    node->analysisError.clear();
    return {AnalysisStoreUpdateStatus::Applied, nodeId};
}

AnalysisStoreUpdateResult AnalysisStore::setErrorForRequest(
    std::string_view requestId,
    std::string diagnostic,
    std::optional<std::string_view> expectedPositionKey)
{
    const std::optional<lizzie::core::NodeId> nodeId =
        lizzie::engine::nodeIdFromAnalysisRequestId(std::string(requestId));
    if (!nodeId.has_value()) {
        return {AnalysisStoreUpdateStatus::InvalidRequestId, std::nullopt};
    }
    return setErrorForNode(*nodeId, std::move(diagnostic), expectedPositionKey);
}

AnalysisStoreUpdateResult AnalysisStore::setErrorForNode(
    lizzie::core::NodeId nodeId,
    std::string diagnostic,
    std::optional<std::string_view> expectedPositionKey)
{
    if (model_ == nullptr) {
        return {AnalysisStoreUpdateStatus::MissingModel, nodeId};
    }
    lizzie::core::GameNode* node = model_->findNode(nodeId);
    if (node == nullptr) {
        return {AnalysisStoreUpdateStatus::MissingNode, nodeId};
    }
    if (expectedPositionKey.has_value() &&
        !expectedPositionKey->empty() &&
        !lizzie::engine::analysisPositionKeyMatches(*model_, nodeId, *expectedPositionKey)) {
        return {AnalysisStoreUpdateStatus::StalePosition, nodeId};
    }

    node->analysis.reset();
    node->analysisError = std::move(diagnostic);
    return {AnalysisStoreUpdateStatus::Applied, nodeId};
}

bool AnalysisStore::clearError(lizzie::core::NodeId nodeId)
{
    if (model_ == nullptr) {
        return false;
    }
    lizzie::core::GameNode* node = model_->findNode(nodeId);
    if (node == nullptr) {
        return false;
    }
    node->analysisError.clear();
    return true;
}

bool AnalysisStore::clearAt(lizzie::core::NodeId nodeId)
{
    return model_ != nullptr && model_->clearAnalysisAt(nodeId);
}

bool AnalysisStore::clearFrom(lizzie::core::NodeId nodeId)
{
    return model_ != nullptr && model_->clearAnalysisFrom(nodeId);
}

void AnalysisStore::clearAll()
{
    if (model_ != nullptr) {
        model_->clearAllAnalysis();
    }
}

}  // namespace lizzie::app
