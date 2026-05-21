#pragma once

#include <memory>
#include <string_view>

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

namespace bcmd::server::application::usecase {

class JoinChannel {
public:
    JoinChannel(std::shared_ptr<port::IChannelRepository> channels,
                std::shared_ptr<port::IClientRegistry> clients);

    bcmd::VoidResult execute(const bcmd::ClientId& client_id,
                             const bcmd::ChannelId& channel_id);

    // Joins by channel name, creating the channel if it does not yet exist.
    // Returns the resolved id (existing or freshly created).
    bcmd::Result<bcmd::ChannelId> executeByName(const bcmd::ClientId& client_id,
                                                std::string_view channel_name);

private:
    std::shared_ptr<port::IChannelRepository> channels_;
    std::shared_ptr<port::IClientRegistry> clients_;
};

}  // namespace bcmd::server::application::usecase
