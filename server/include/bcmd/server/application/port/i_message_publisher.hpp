#pragma once

#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"

#include <unordered_set>

namespace bcmd::server::application::port {

// Outbound delivery port: hands a live message to a single subscribed client.
class IMessagePublisher {
public:
    virtual ~IMessagePublisher() = default;

    virtual void publish(const bcmd::ClientId& recipient_id, const domain::Message& message) = 0;

    virtual void broadcastMemberJoined(const bcmd::ChannelId& channel_id,
                                       const std::unordered_set<bcmd::ClientId>& recipients,
                                       const bcmd::ClientId& client_id,
                                       const domain::Username& username) = 0;

    // Broadcasts a member-left notification to subscribers whose client id is
    // present in the caller-assembled channel membership snapshot.
    virtual void broadcastMemberLeft(const bcmd::ChannelId& channel_id,
                                     const std::unordered_set<bcmd::ClientId>& recipients,
                                     const bcmd::ClientId& client_id,
                                     const domain::Username& username) = 0;

    // Removes the subscriber from any outbound delivery routing.
    virtual void unregisterSubscriber(const bcmd::ClientId& client_id) = 0;

protected:
    IMessagePublisher() = default;
    IMessagePublisher(const IMessagePublisher&) = default;
    IMessagePublisher& operator=(const IMessagePublisher&) = default;
    IMessagePublisher(IMessagePublisher&&) = default;
    IMessagePublisher& operator=(IMessagePublisher&&) = default;
};

}  // namespace bcmd::server::application::port
