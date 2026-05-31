#include "bcmd/server/application/usecase/join_channel.hpp"

#include "bcmd/server/application/port/i_channel_list_publisher.hpp"
#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/application/usecase/internal/remove_member_broadcast.hpp"
#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/server/domain/model/client_session.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"
#include "bcmd/shared/string_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace bcmd::server::application::usecase {

JoinChannel::JoinChannel(std::shared_ptr<port::IChannelRepository> channels,
                         std::shared_ptr<port::IClientRegistry> clients,
                         std::shared_ptr<port::IMessagePublisher> message_publisher,
                         std::shared_ptr<port::IChannelListPublisher> channel_list_publisher)
    : channels_(std::move(channels)),
      clients_(std::move(clients)),
      message_publisher_(std::move(message_publisher)),
      channel_list_publisher_(std::move(channel_list_publisher)) {}

bcmd::VoidResult JoinChannel::leaveOtherChannels(domain::ClientSession& session,
                                                 const bcmd::ChannelId& target_channel_id) {
    std::vector<bcmd::ChannelId> other_channels;
    other_channels.reserve(session.joinedChannels().size());
    for (const auto& joined : session.joinedChannels()) {
        if (joined != target_channel_id) {
            other_channels.push_back(joined);
        }
    }

    for (const auto& other_channel_id : other_channels) {
        if (auto removed = internal::removeMemberAndBroadcast(*channels_, *message_publisher_,
                                                              session, other_channel_id);
            !removed.has_value()) {
            if (removed.error() != bcmd::Error::NotAMember &&
                removed.error() != bcmd::Error::ChannelNotFound) {
                return std::unexpected(removed.error());
            }
        }
        session.leaveChannel(other_channel_id);
        if (auto other = channels_->findById(other_channel_id); other.has_value()) {
            channel_list_publisher_->publishMemberCountChanged(
                other_channel_id, static_cast<std::int32_t>(other->memberCount()));
        }
    }
    return {};
}

bcmd::VoidResult JoinChannel::execute(const bcmd::ClientId& client_id,
                                      const bcmd::ChannelId& channel_id) {
    auto session = clients_->findById(client_id);
    if (!session.has_value()) {
        return std::unexpected(session.error());
    }
    auto channel = channels_->findById(channel_id);
    if (!channel.has_value()) {
        return std::unexpected(channel.error());
    }

    const bool had_other_channels = std::ranges::any_of(
        session->joinedChannels(), [&](const auto& id) { return id != channel_id; });
    if (auto left = leaveOtherChannels(*session, channel_id); !left.has_value()) {
        return std::unexpected(left.error());
    }

    if (channel->hasMember(client_id)) {
        if (had_other_channels) {
            if (auto saved = clients_->save(*session); !saved.has_value()) {
                return std::unexpected(saved.error());
            }
        }
        return {};
    }

    if (auto added = channel->addMember(client_id); !added.has_value()) {
        return std::unexpected(added.error());
    }
    session->joinChannel(channel_id);

    if (auto saved = channels_->save(*channel); !saved.has_value()) {
        return std::unexpected(saved.error());
    }
    if (auto saved = clients_->save(*session); !saved.has_value()) {
        return std::unexpected(saved.error());
    }
    message_publisher_->broadcastMemberJoined(channel->id(), channel->members(), client_id,
                                              session->username());
    channel_list_publisher_->publishMemberCountChanged(
        channel->id(), static_cast<std::int32_t>(channel->memberCount()));
    return {};
}

bcmd::Result<bcmd::ChannelId> JoinChannel::executeByName(const bcmd::ClientId& client_id,
                                                         std::string_view channel_name) {
    auto session = clients_->findById(client_id);
    if (!session.has_value()) {
        return std::unexpected(session.error());
    }

    const auto trimmed_name = bcmd::trim(channel_name);
    auto validated_name = domain::ChannelName::create(trimmed_name);
    if (!validated_name.has_value()) {
        return std::unexpected(bcmd::Error::InvalidChannelName);
    }

    auto existing = channels_->findByName(*validated_name);
    if (!existing.has_value()) {
        return std::unexpected(existing.error());
    }
    auto channel = *existing;

    const bool had_other_channels = std::ranges::any_of(
        session->joinedChannels(), [&](const auto& id) { return id != channel.id(); });
    if (auto left = leaveOtherChannels(*session, channel.id()); !left.has_value()) {
        return std::unexpected(left.error());
    }

    if (channel.hasMember(client_id)) {
        if (had_other_channels) {
            if (auto saved = clients_->save(*session); !saved.has_value()) {
                return std::unexpected(saved.error());
            }
        }
        return channel.id();
    }

    if (auto added = channel.addMember(client_id); !added.has_value()) {
        return std::unexpected(added.error());
    }
    session->joinChannel(channel.id());

    if (auto saved = channels_->save(channel); !saved.has_value()) {
        return std::unexpected(saved.error());
    }
    if (auto saved = clients_->save(*session); !saved.has_value()) {
        return std::unexpected(saved.error());
    }
    message_publisher_->broadcastMemberJoined(channel.id(), channel.members(), client_id,
                                              session->username());
    channel_list_publisher_->publishMemberCountChanged(
        channel.id(), static_cast<std::int32_t>(channel.memberCount()));
    return channel.id();
}

}  // namespace bcmd::server::application::usecase
