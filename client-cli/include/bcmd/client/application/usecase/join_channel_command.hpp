#pragma once

#include "bcmd/client/application/port/i_server_gateway.hpp"
#include "bcmd/shared/result.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace bcmd::client::application::usecase {

class JoinChannelCommand {
public:
    explicit JoinChannelCommand(std::shared_ptr<port::IServerGateway> gateway);

    // Joins by channel name through the gateway and returns the resolved channel id.
    bcmd::Result<std::string> execute(std::string_view client_id, std::string_view channel_name);

private:
    std::shared_ptr<port::IServerGateway> gateway_{};
};

}  // namespace bcmd::client::application::usecase
