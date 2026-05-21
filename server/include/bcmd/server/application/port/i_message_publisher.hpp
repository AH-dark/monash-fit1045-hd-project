#pragma once

#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/shared/ids.hpp"

namespace bcmd::server::application::port {

// Outbound delivery port: hands a message to a single subscribed client.
// `from_replay` marks historical (catch-up) deliveries so adapters can flag
// them on the wire (e.g. ChannelEvent.from_replay).
class IMessagePublisher {
public:
    virtual ~IMessagePublisher() = default;

    virtual void publish(const bcmd::ClientId& recipient_id,
                         const domain::Message& message,
                         bool from_replay = false) = 0;

    // Emitted after replay finishes so subscribers can switch from
    // catch-up to live mode.
    virtual void publishReplayComplete(const bcmd::ClientId& recipient_id,
                                       const bcmd::ChannelId& channel_id) = 0;

protected:
    IMessagePublisher() = default;
    IMessagePublisher(const IMessagePublisher&) = default;
    IMessagePublisher& operator=(const IMessagePublisher&) = default;
    IMessagePublisher(IMessagePublisher&&) = default;
    IMessagePublisher& operator=(IMessagePublisher&&) = default;
};

}  // namespace bcmd::server::application::port
