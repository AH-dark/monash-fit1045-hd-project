#include "bcmd/server/application/usecase/internal/remove_member_broadcast.hpp"

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/domain/model/client_session.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <expected>

namespace bcmd::server::application::usecase::internal {

bcmd::VoidResult removeMemberAndBroadcast(port::IChannelRepository& channels,
                                          port::IMessagePublisher& publisher,
                                          const domain::ClientSession& session,
                                          const bcmd::ChannelId& channel_id) {
    auto channel = channels.findById(channel_id);
    if (!channel.has_value()) {
        return std::unexpected(channel.error());  // STRICT: propagate ChannelNotFound
    }
    if (auto removed = channel->removeMember(session.id()); !removed.has_value()) {
        return std::unexpected(removed.error());  // STRICT: propagate NotAMember
    }
    if (auto saved = channels.save(*channel); !saved.has_value()) {
        return std::unexpected(saved.error());
    }
    publisher.broadcastMemberLeft(channel_id, session.id(), session.username());
    return {};
}

}  // namespace bcmd::server::application::usecase::internal
