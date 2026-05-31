#pragma once

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <memory>

namespace bcmd::server::application::usecase {

class SubscribeToChannel {
public:
    SubscribeToChannel(std::shared_ptr<port::IChannelRepository> channels);

    // Validates that the client is a member of the channel. Live delivery is
    // wired up by the gRPC adapter; history is served separately via
    // ListMessages.
    bcmd::VoidResult execute(const bcmd::ClientId& client_id, const bcmd::ChannelId& channel_id);

private:
    std::shared_ptr<port::IChannelRepository> channels_{};
};

}  // namespace bcmd::server::application::usecase
