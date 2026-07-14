#include "BatchAnalysisTracker.h"

#include <utility>

namespace lizzie::app {

void BatchAnalysisTracker::reset(PositionKeyMap positionKeys)
{
    positionKeys_ = std::move(positionKeys);
}

void BatchAnalysisTracker::clear()
{
    positionKeys_.clear();
}

bool BatchAnalysisTracker::empty() const
{
    return positionKeys_.empty();
}

std::size_t BatchAnalysisTracker::size() const
{
    return positionKeys_.size();
}

const std::string* BatchAnalysisTracker::positionKeyFor(const std::string& requestId) const
{
    const auto iter = positionKeys_.find(requestId);
    if (iter == positionKeys_.end()) {
        return nullptr;
    }
    return &iter->second;
}

void BatchAnalysisTracker::remove(const std::string& requestId)
{
    positionKeys_.erase(requestId);
}

std::vector<TrackedAnalysisRequest> BatchAnalysisTracker::pendingRequests() const
{
    std::vector<TrackedAnalysisRequest> result;
    result.reserve(positionKeys_.size());
    for (const auto& [requestId, positionKey] : positionKeys_) {
        result.push_back(TrackedAnalysisRequest{requestId, positionKey});
    }
    return result;
}

}  // namespace lizzie::app
