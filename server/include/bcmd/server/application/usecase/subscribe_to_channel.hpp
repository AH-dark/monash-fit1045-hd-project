#pragma once

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/application/port/i_message_repository.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <cstdint>
#include <memory>

namespace bcmd::server::application::usecase {

class SubscribeToChannel {
public:
    // Same server-side hard cap as GetRecentMessages.
    static constexpr std::uint32_t kServerMaxReplay = 200;

    SubscribeToChannel(std::shared_ptr<port::IChannelRepository> channels,
                       std::shared_ptr<port::IMessageRepository> messages,
                       std::shared_ptr<port::IMessagePublisher> publisher);

    // Pushes up to `replay_count` historical messages to the subscriber (each
    // marked from_replay=true), then a replay-complete marker. Live delivery
    // is wired up by Phase 5's gRPC adapter.
    bcmd::VoidResult execute(const bcmd::ClientId& client_id, const bcmd::ChannelId& channel_id,
                             std::uint32_t replay_count);

private:
    std::shared_ptr<port::IChannelRepository> channels_{};
    std::shared_ptr<port::IMessageRepository> messages_{};
    std::shared_ptr<port::IMessagePublisher> publisher_{};
};

}  // namespace bcmd::server::application::usecase
