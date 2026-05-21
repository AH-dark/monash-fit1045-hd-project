#pragma once

#include <cstdint>
#include <string>

namespace bcmd::client::domain {

// Display-oriented value object: one entry in the client inbox.
struct InboxMessage {
    std::string message_id;
    std::string channel_id;
    std::string sender_name;
    std::string content;
    std::int64_t sent_at_ms{0};
    bool is_history{false};
};

}  // namespace bcmd::client::domain
