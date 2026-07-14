#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace lizzie::app {

struct TrackedAnalysisRequest {
    std::string requestId;
    std::string positionKey;
};

class BatchAnalysisTracker {
public:
    using PositionKeyMap = std::unordered_map<std::string, std::string>;

    void reset(PositionKeyMap positionKeys);
    void clear();
    [[nodiscard]] bool empty() const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] const std::string* positionKeyFor(const std::string& requestId) const;
    void remove(const std::string& requestId);
    [[nodiscard]] std::vector<TrackedAnalysisRequest> pendingRequests() const;

private:
    PositionKeyMap positionKeys_;
};

}  // namespace lizzie::app
