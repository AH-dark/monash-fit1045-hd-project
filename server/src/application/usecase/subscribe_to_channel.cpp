#include "bcmd/server/application/usecase/subscribe_to_channel.hpp"

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <expected>
#include <memory>
#include <utility>

namespace bcmd::server::application::usecase {

SubscribeToChannel::SubscribeToChannel(std::shared_ptr<port::IChannelRepository> channels)
    : channels_(std::move(channels)) {}

bcmd::VoidResult SubscribeToChannel::execute(const bcmd::ClientId& client_id,
                                             const bcmd::ChannelId& channel_id) {
    auto channel = channels_->findById(channel_id);
    if (!channel.has_value()) {
        return std::unexpected(channel.error());
    }
    if (!channel->hasMember(client_id)) {
        return std::unexpected(bcmd::Error::NotAMember);
    }
    return {};
}

}  // namespace bcmd::server::application::usecase
