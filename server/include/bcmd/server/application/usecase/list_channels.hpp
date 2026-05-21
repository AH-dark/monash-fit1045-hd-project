#pragma once

#include <memory>
#include <vector>

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/domain/model/channel.hpp"

namespace bcmd::server::application::usecase {

class ListChannels {
public:
    explicit ListChannels(std::shared_ptr<port::IChannelRepository> channels);

    std::vector<domain::Channel> execute();

private:
    std::shared_ptr<port::IChannelRepository> channels_;
};

}  // namespace bcmd::server::application::usecase
