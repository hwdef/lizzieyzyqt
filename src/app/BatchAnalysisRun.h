#pragma once

#include "AnalysisJson.h"
#include "BatchAnalysis.h"
#include "GameDocumentSession.h"
#include "GameModel.h"
#include "PositionSync.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace lizzie::app {

struct BatchAnalysisRunPlan {
    std::vector<lizzie::engine::AnalysisRequest> requests;
    std::vector<lizzie::core::NodeId> nodesToClear;
    std::unordered_map<std::string, std::string> positionKeys;
    std::vector<std::string> warnings;

    [[nodiscard]] bool hasRequests() const;
};

enum class BatchAnalysisRunMode {
    None,
    Estimate,
    Batch,
};

enum class BatchAnalysisCompletionMessage {
    EstimateFailed,
    BatchFailed,
    EstimateCancelled,
    BatchCancelled,
    EstimateComplete,
    BatchComplete,
    BatchFailedWithOutput,
    BatchCompleteWithOutput,
};

struct BatchAnalysisOutputPlan {
    bool writeSidecar = false;
    QString path;
    bool normalizeSingleGameCollectionAfterSgfWrite = false;
    BatchAnalysisCompletionMessage message = BatchAnalysisCompletionMessage::BatchCompleteWithOutput;
};

struct BatchAnalysisCompletionPlan {
    std::optional<BatchAnalysisOutputPlan> output;
    BatchAnalysisCompletionMessage fallbackMessage = BatchAnalysisCompletionMessage::BatchComplete;
};

[[nodiscard]] lizzie::engine::BatchAnalysisOptions batchOptionsFromRealtimeOptions(
    lizzie::engine::RealtimeAnalysisOptions options,
    bool includeOwnership);

[[nodiscard]] BatchAnalysisRunPlan buildEstimateRunPlan(
    const lizzie::core::GameModel& model,
    lizzie::engine::RealtimeAnalysisOptions options);

[[nodiscard]] BatchAnalysisRunPlan buildBatchRunPlan(
    const lizzie::core::GameModel& model,
    lizzie::engine::RealtimeAnalysisOptions options);

[[nodiscard]] BatchAnalysisCompletionPlan planBatchAnalysisCompletion(
    BatchAnalysisRunMode mode,
    bool cancelled,
    bool failed,
    const GameDocumentSession& session,
    bool writeSidecarAfterBatch);

}  // namespace lizzie::app
