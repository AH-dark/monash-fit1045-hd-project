#pragma once

#include "bcmd/client/domain/inbox_message.hpp"

#include <cstddef>
#include <mutex>
#include <queue>
#include <vector>

namespace bcmd::client::adapter::tui {

class InboxQueue {
public:
    void push(domain::InboxMessage message);
    bool tryPop(domain::InboxMessage& out);
    void drainTo(std::vector<domain::InboxMessage>& out, std::size_t max_count = 50);

private:
    std::mutex mutex_;
    std::queue<domain::InboxMessage> messages_;
};

}  // namespace bcmd::client::adapter::tui
