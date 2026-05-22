#pragma once

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/application/port/i_message_publisher.hpp"

#include <chrono>
#include <cstddef>
#include <memory>

namespace bcmd::server::application::usecase {

// Periodically called by HeartbeatSweeper.
// For each session whose lastHeartbeatAt() < deadline:
//   - For each joined channel: call internal::removeMemberAndBroadcast (strict);
//     silently swallow NotAMember/ChannelNotFound (expected races vs concurrent Disconnect).
//   - Remove from client registry (silently swallow ClientNotFound).
//   - Unregister subscriber via the publisher port (idempotent).
// Lock-order invariant: each step acquires/releases its own lock; this method
// MUST NOT hold registry lock when publishing or when calling unregisterSubscriber.
class ExpireInactiveClients {
public:
    ExpireInactiveClients(std::shared_ptr<port::IClientRegistry> clients,
                          std::shared_ptr<port::IChannelRepository> channels,
                          std::shared_ptr<port::IMessagePublisher> publisher);

    // Returns count of sessions expired.
    std::size_t run(std::chrono::steady_clock::time_point deadline);

private:
    std::shared_ptr<port::IClientRegistry> clients_;
    std::shared_ptr<port::IChannelRepository> channels_;
    std::shared_ptr<port::IMessagePublisher> publisher_;
};

}  // namespace bcmd::server::application::usecase
