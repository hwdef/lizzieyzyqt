#pragma once

#include "GtpProtocol.h"

#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace lizzie::engine {

struct QueuedCommand {
    int id = 0;
    std::string name;
    std::vector<std::string> arguments;
};

struct QueuedGtpResponse {
    GtpResponse response;
    std::optional<QueuedCommand> command;
};

class GtpCommandQueue {
public:
    [[nodiscard]] int nextCommandId() const;
    [[nodiscard]] std::size_t pendingCount() const;

    [[nodiscard]] GtpCommand enqueue(GtpCommand command);
    [[nodiscard]] QueuedGtpResponse resolve(const GtpResponse& response);
    void reset();

private:
    std::deque<QueuedCommand> pending_;
    int nextId_ = 1;
};

}  // namespace lizzie::engine
