#include "BatchAnalysisRun.h"

namespace lizzie::app {

bool BatchAnalysisRunPlan::hasRequests() const
{
    return !requests.empty();
}

lizzie::engine::BatchAnalysisOptions batchOptionsFromRealtimeOptions(
    lizzie::engine::RealtimeAnalysisOptions options,
    bool includeOwnership)
{
    lizzie::engine::BatchAnalysisOptions result;
    result.includeOwnership = includeOwnership;
    result.maxVisits = options.maxVisits;
    result.maxPlayouts = options.maxPlayouts;
    result.maxTime = options.maxTimeSeconds;
    result.playoutDoublingAdvantage = options.playoutDoublingAdvantage;
    result.wideRootNoise = options.analysisWideRootNoise;
    return result;
}

namespace {

BatchAnalysisCompletionMessage fallbackCompletionMessage(
    BatchAnalysisRunMode mode,
    bool cancelled,
    bool failed)
{
    if (failed) {
        return mode == BatchAnalysisRunMode::Estimate
            ? BatchAnalysisCompletionMessage::EstimateFailed
            : BatchAnalysisCompletionMessage::BatchFailed;
    }
    if (cancelled) {
        return mode == BatchAnalysisRunMode::Estimate
            ? BatchAnalysisCompletionMessage::EstimateCancelled
            : BatchAnalysisCompletionMessage::BatchCancelled;
    }
    return mode == BatchAnalysisRunMode::Estimate
        ? BatchAnalysisCompletionMessage::EstimateComplete
        : BatchAnalysisCompletionMessage::BatchComplete;
}

BatchAnalysisRunPlan buildRunPlan(
    const lizzie::core::GameModel& model,
    const std::vector<lizzie::core::NodeId>& nodes,
    lizzie::engine::RealtimeAnalysisOptions options,
    bool includeOwnership)
{
    const lizzie::engine::BatchAnalysisPlan batchPlan =
        lizzie::engine::buildBatchAnalysisPlan(
            model,
            nodes,
            batchOptionsFromRealtimeOptions(options, includeOwnership));

    BatchAnalysisRunPlan result;
    result.warnings = batchPlan.warnings;
    result.requests.reserve(batchPlan.items.size());
    result.nodesToClear.reserve(batchPlan.items.size());
    result.positionKeys.reserve(batchPlan.items.size());
    for (const lizzie::engine::BatchAnalysisItem& item : batchPlan.items) {
        result.requests.push_back(item.request);
        result.nodesToClear.push_back(item.nodeId);
        result.positionKeys.emplace(
            item.request.id,
            lizzie::engine::analysisPositionKey(model, item.nodeId));
    }
    return result;
}

}  // namespace

BatchAnalysisRunPlan buildEstimateRunPlan(
    const lizzie::core::GameModel& model,
    lizzie::engine::RealtimeAnalysisOptions options)
{
    return buildRunPlan(model, {model.currentId()}, options, true);
}

BatchAnalysisRunPlan buildBatchRunPlan(
    const lizzie::core::GameModel& model,
    lizzie::engine::RealtimeAnalysisOptions options)
{
    return buildRunPlan(model, {}, options, options.includeOwnership);
}

BatchAnalysisCompletionPlan planBatchAnalysisCompletion(
    BatchAnalysisRunMode mode,
    bool cancelled,
    bool failed,
    const GameDocumentSession& session,
    bool writeSidecarAfterBatch)
{
    BatchAnalysisCompletionPlan plan;
    plan.fallbackMessage = fallbackCompletionMessage(mode, cancelled, failed);
    if (cancelled || mode != BatchAnalysisRunMode::Batch) {
        return plan;
    }

    const QString outputPath = batchAnalysisOutputPath(session, writeSidecarAfterBatch);
    if (outputPath.isEmpty()) {
        return plan;
    }

    plan.output = BatchAnalysisOutputPlan{
        .writeSidecar = writeSidecarAfterBatch,
        .path = outputPath,
        .normalizeSingleGameCollectionAfterSgfWrite = !writeSidecarAfterBatch && session.collection.size() <= 1,
        .message = failed
            ? BatchAnalysisCompletionMessage::BatchFailedWithOutput
            : BatchAnalysisCompletionMessage::BatchCompleteWithOutput,
    };
    return plan;
}

}  // namespace lizzie::app
