#include "GtpCommandQueue.h"

#include <algorithm>
#include <limits>

namespace lizzie::engine {

namespace {

bool commandIdIsUsable(int id)
{
    return id > 0 && id < std::numeric_limits<int>::max();
}

bool containsPendingId(const std::deque<QueuedCommand>& pending, int id)
{
    return std::ranges::any_of(pending, [id](const QueuedCommand& command) {
        return command.id == id;
    });
}

int nextCandidateId(int id)
{
    if (id <= 0 || id >= std::numeric_limits<int>::max() - 1) {
        return 1;
    }
    return id + 1;
}

int nextAvailableCommandId(const std::deque<QueuedCommand>& pending, int start)
{
    int candidate = commandIdIsUsable(start) ? start : 1;
    for (std::size_t attempts = 0; attempts <= pending.size(); ++attempts) {
        if (!containsPendingId(pending, candidate)) {
            return candidate;
        }
        candidate = nextCandidateId(candidate);
    }
    return 1;
}

}  // namespace

int GtpCommandQueue::nextCommandId() const
{
    return nextId_;
}

std::size_t GtpCommandQueue::pendingCount() const
{
    return pending_.size();
}

GtpCommand GtpCommandQueue::enqueue(GtpCommand command)
{
    if (!command.id.has_value() || !commandIdIsUsable(*command.id) || containsPendingId(pending_, *command.id)) {
        command.id = nextAvailableCommandId(pending_, nextId_);
    }

    pending_.push_back(QueuedCommand{*command.id, command.name, command.arguments});
    nextId_ = nextAvailableCommandId(pending_, nextCandidateId(*command.id));
    return command;
}

QueuedGtpResponse GtpCommandQueue::resolve(const GtpResponse& response)
{
    QueuedGtpResponse result{response, std::nullopt};
    if (pending_.empty()) {
        return result;
    }

    if (response.id.has_value()) {
        const auto iter = std::ranges::find_if(pending_, [&](const QueuedCommand& command) {
            return command.id == *response.id;
        });
        if (iter == pending_.end()) {
            return result;
        }

        result.command = *iter;
        pending_.erase(iter);
        return result;
    }

    result.command = pending_.front();
    pending_.pop_front();
    return result;
}

void GtpCommandQueue::reset()
{
    pending_.clear();
    nextId_ = 1;
}

}  // namespace lizzie::engine
