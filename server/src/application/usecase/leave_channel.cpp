#include "bcmd/server/application/usecase/leave_channel.hpp"

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/application/usecase/internal/remove_member_broadcast.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <expected>
#include <memory>
#include <utility>

namespace bcmd::server::application::usecase {

LeaveChannel::LeaveChannel(std::shared_ptr<port::IChannelRepository> channels,
                           std::shared_ptr<port::IClientRegistry> clients,
                           std::shared_ptr<port::IMessagePublisher> publisher)
    : channels_(std::move(channels)),
      clients_(std::move(clients)),
      publisher_(std::move(publisher)) {}

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
    session->leaveChannel(channel_id);
    if (auto saved = clients_->save(*session); !saved.has_value()) {
        return std::unexpected(saved.error());
    }
    return {};
}

}  // namespace bcmd::server::application::usecase
