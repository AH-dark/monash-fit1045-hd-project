#include "bcmd/server/application/usecase/join_channel.hpp"

#include <expected>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/server/domain/model/client_session.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

namespace bcmd::server::application::usecase {

JoinChannel::JoinChannel(std::shared_ptr<port::IChannelRepository> channels,
                         std::shared_ptr<port::IClientRegistry> clients)
    : channels_(std::move(channels)), clients_(std::move(clients)) {}

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
    return {};
}

bcmd::Result<bcmd::ChannelId> JoinChannel::executeByName(const bcmd::ClientId& client_id,
                                                          std::string_view channel_name) {
    auto session = clients_->findById(client_id);
    if (!session.has_value()) {
        return std::unexpected(session.error());
    }

    auto validated_name = domain::ChannelName::create(channel_name);
    if (!validated_name.has_value()) {
        return std::unexpected(bcmd::Error::InvalidChannelName);
    }

    std::optional<domain::Channel> channel_opt;
    if (auto existing = channels_->findByName(*validated_name); existing.has_value()) {
        channel_opt.emplace(*existing);
    } else {
        auto created = channels_->create(*validated_name);
        if (!created.has_value()) {
            return std::unexpected(created.error());
        }
        channel_opt.emplace(*created);
    }
    auto& channel = *channel_opt;

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
    return channel.id();
}

}  // namespace bcmd::server::application::usecase
