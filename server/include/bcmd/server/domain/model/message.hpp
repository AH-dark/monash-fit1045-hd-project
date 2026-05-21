#pragma once

#include "bcmd/server/domain/model/message_content.hpp"
#include "bcmd/shared/ids.hpp"

#include <chrono>
#include <utility>

namespace bcmd::server::domain {

// Immutable broadcast message. All fields are captured at construction; the
// type exposes only const accessors.
class Message {
public:
    Message(bcmd::MessageId id, bcmd::ClientId sender_id, bcmd::ChannelId channel_id,
            MessageContent content,
            std::chrono::system_clock::time_point sent_at = std::chrono::system_clock::now())
        : id_(std::move(id)),
          sender_id_(std::move(sender_id)),
          channel_id_(std::move(channel_id)),
          content_(std::move(content)),
          sent_at_(sent_at) {}

    const bcmd::MessageId& id() const noexcept { return id_; }
    const bcmd::ClientId& senderId() const noexcept { return sender_id_; }
    const bcmd::ChannelId& channelId() const noexcept { return channel_id_; }
    const MessageContent& content() const noexcept { return content_; }
    std::chrono::system_clock::time_point sentAt() const noexcept { return sent_at_; }

private:
    bcmd::MessageId id_;
    bcmd::ClientId sender_id_;
    bcmd::ChannelId channel_id_;
    MessageContent content_;
    std::chrono::system_clock::time_point sent_at_;
};

}  // namespace bcmd::server::domain
