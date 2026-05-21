#include "bcmd/client/adapter/tui/inbox_queue.hpp"

#include "bcmd/client/domain/inbox_message.hpp"

#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

namespace bcmd::client::adapter::tui {

void InboxQueue::push(domain::InboxMessage message) {
    std::scoped_lock lock{mutex_};
    messages_.push(std::move(message));
}

bool InboxQueue::tryPop(domain::InboxMessage& out) {
    std::scoped_lock lock{mutex_};
    if (messages_.empty()) {
        // NOLINTNEXTLINE(readability-simplify-boolean-expr)
        return false;
    }
    out = std::move(messages_.front());
    messages_.pop();
    return true;
}

void InboxQueue::drainTo(std::vector<domain::InboxMessage>& out, std::size_t max_count) {
    std::scoped_lock lock{mutex_};
    for (std::size_t count{0}; count < max_count && !messages_.empty(); ++count) {
        out.push_back(std::move(messages_.front()));
        messages_.pop();
    }
}

}  // namespace bcmd::client::adapter::tui
