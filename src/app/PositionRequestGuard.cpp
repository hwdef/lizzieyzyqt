#include "PositionRequestGuard.h"

#include <utility>

namespace lizzie::app {

void PositionRequestGuard::start(std::string positionKey)
{
    positionKey_ = std::move(positionKey);
}

bool PositionRequestGuard::consumeIfMatches(std::string_view currentPositionKey)
{
    if (!positionKey_.has_value()) {
        return false;
    }
    const bool matches = *positionKey_ == currentPositionKey;
    positionKey_.reset();
    return matches;
}

void PositionRequestGuard::clear()
{
    positionKey_.reset();
}

bool PositionRequestGuard::hasPendingRequest() const
{
    return positionKey_.has_value();
}

}  // namespace lizzie::app
