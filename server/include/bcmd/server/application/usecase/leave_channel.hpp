#pragma once

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <memory>

namespace bcmd::server::application::usecase {

class LeaveChannel {
public:
    LeaveChannel(std::shared_ptr<port::IChannelRepository> channels,
                 std::shared_ptr<port::IClientRegistry> clients);

    bcmd::VoidResult execute(const bcmd::ClientId& client_id, const bcmd::ChannelId& channel_id);

private:
    std::shared_ptr<port::IChannelRepository> channels_;
    std::shared_ptr<port::IClientRegistry> clients_;
};

}  // namespace bcmd::server::application::usecase
