#pragma once

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <memory>
#include <string_view>

namespace bcmd::server::application::usecase {

class CreateChannel {
public:
    CreateChannel(std::shared_ptr<port::IChannelRepository> channels,
                  std::shared_ptr<port::IClientRegistry> clients);

    bcmd::Result<bcmd::ChannelId> execute(const bcmd::ClientId& client_id,
                                          std::string_view channel_name);

private:
    std::shared_ptr<port::IChannelRepository> channels_{};
    std::shared_ptr<port::IClientRegistry> clients_{};
};

}  // namespace bcmd::server::application::usecase
