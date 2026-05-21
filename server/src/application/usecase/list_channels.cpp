#include "bcmd/server/application/usecase/list_channels.hpp"

#include <memory>
#include <utility>
#include <vector>

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/domain/model/channel.hpp"

namespace bcmd::server::application::usecase {

ListChannels::ListChannels(std::shared_ptr<port::IChannelRepository> channels)
    : channels_(std::move(channels)) {}

std::vector<domain::Channel> ListChannels::execute() {
    return channels_->listAll();
}

}  // namespace bcmd::server::application::usecase
