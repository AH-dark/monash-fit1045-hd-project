#include "bcmd/server/application/usecase/subscribe_to_channel.hpp"

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/application/port/i_message_repository.hpp"
#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <memory>
#include <utility>

namespace bcmd::server::application::usecase {

SubscribeToChannel::SubscribeToChannel(std::shared_ptr<port::IChannelRepository> channels,
                                       std::shared_ptr<port::IMessageRepository> messages,
                                       std::shared_ptr<port::IMessagePublisher> publisher)
    : channels_(std::move(channels)),
      messages_(std::move(messages)),
      publisher_(std::move(publisher)) {}

bcmd::VoidResult SubscribeToChannel::execute(const bcmd::ClientId& client_id,
                                             const bcmd::ChannelId& channel_id,
                                             std::uint32_t replay_count) {
    auto channel = channels_->findById(channel_id);
    if (!channel.has_value()) {
        return std::unexpected(channel.error());
    }
    if (!channel->hasMember(client_id)) {
        return std::unexpected(bcmd::Error::NotAMember);
    }

    const auto capped = std::min(replay_count, kServerMaxReplay);
    if (capped > 0) {
        const auto history = messages_->recent(channel_id, capped);
        for (const auto& message : history) {
            publisher_->publish(client_id, message, /*from_replay=*/true);
        }
    }
    publisher_->publishReplayComplete(client_id, channel_id);
    return {};
}

}  // namespace bcmd::server::application::usecase
