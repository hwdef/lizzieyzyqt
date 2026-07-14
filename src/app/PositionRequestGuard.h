#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace lizzie::app {

class PositionRequestGuard {
public:
    void start(std::string positionKey);
    [[nodiscard]] bool consumeIfMatches(std::string_view currentPositionKey);
    void clear();

    [[nodiscard]] bool hasPendingRequest() const;

private:
    std::optional<std::string> positionKey_;
};

}  // namespace lizzie::app
