#include "bcmd/server/application/usecase/create_channel.hpp"

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"
#include "bcmd/shared/string_utils.hpp"

#include <expected>
#include <memory>
#include <string_view>
#include <utility>

namespace bcmd::server::application::usecase {

CreateChannel::CreateChannel(std::shared_ptr<port::IChannelRepository> channels,
                             std::shared_ptr<port::IClientRegistry> clients)
    : channels_(std::move(channels)), clients_(std::move(clients)) {}

bcmd::Result<bcmd::ChannelId> CreateChannel::execute(const bcmd::ClientId& client_id,
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

    auto created = channels_->create(*validated_name);
    if (!created.has_value()) {
        return std::unexpected(created.error());
    }
    return created->id();
}

}  // namespace bcmd::server::application::usecase
