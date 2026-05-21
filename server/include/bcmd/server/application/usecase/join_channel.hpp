#pragma once

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <memory>
#include <string_view>

namespace bcmd::server::application::usecase {

class JoinChannel {
public:
    JoinChannel(std::shared_ptr<port::IChannelRepository> channels,
                std::shared_ptr<port::IClientRegistry> clients);

    bcmd::VoidResult execute(const bcmd::ClientId& client_id, const bcmd::ChannelId& channel_id);

    // Joins an existing channel by name. Returns Error::ChannelNotFound when the
    // channel does not exist; use CreateChannel to create one first.
    bcmd::Result<bcmd::ChannelId> executeByName(const bcmd::ClientId& client_id,
                                                std::string_view channel_name);

private:
    std::shared_ptr<port::IChannelRepository> channels_{};
    std::shared_ptr<port::IClientRegistry> clients_{};
};

}  // namespace bcmd::server::application::usecase
