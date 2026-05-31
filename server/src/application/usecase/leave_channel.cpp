#include "bcmd/server/application/usecase/leave_channel.hpp"

#include "bcmd/server/application/port/i_channel_list_publisher.hpp"
#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/application/usecase/internal/remove_member_broadcast.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <cstdint>
#include <expected>
#include <memory>
#include <utility>

namespace bcmd::server::application::usecase {

LeaveChannel::LeaveChannel(std::shared_ptr<port::IChannelRepository> channels,
                           std::shared_ptr<port::IClientRegistry> clients,
                           std::shared_ptr<port::IMessagePublisher> publisher,
                           std::shared_ptr<port::IChannelListPublisher> channel_list_publisher)
    : channels_(std::move(channels)),
      clients_(std::move(clients)),
      publisher_(std::move(publisher)),
      channel_list_publisher_(std::move(channel_list_publisher)) {}

bcmd::VoidResult LeaveChannel::execute(const bcmd::ClientId& client_id,
                                       const bcmd::ChannelId& channel_id) {
    auto session = clients_->findById(client_id);
    if (!session.has_value()) {
        return std::unexpected(session.error());
    }
    if (auto removed =
            internal::removeMemberAndBroadcast(*channels_, *publisher_, *session, channel_id);
        !removed.has_value()) {
        return std::unexpected(removed.error());
    }
    if (auto left = clients_->leaveChannelAtomic(client_id, channel_id);
        !left.has_value() && left.error() != bcmd::Error::ClientNotFound) {
        return std::unexpected(left.error());
    }
    auto channel = channels_->findById(channel_id);
    if (!channel.has_value()) {
        return std::unexpected(channel.error());
    }
    channel_list_publisher_->publishMemberCountChanged(
        channel_id, static_cast<std::int32_t>(channel->memberCount()));
    return {};
}

}  // namespace bcmd::server::application::usecase
