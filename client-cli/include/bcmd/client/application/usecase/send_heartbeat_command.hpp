#pragma once

#include "bcmd/client/application/port/i_server_gateway.hpp"
#include "bcmd/shared/result.hpp"

#include <memory>
#include <string_view>

namespace bcmd::client::application::usecase {

class SendHeartbeatCommand {
public:
    explicit SendHeartbeatCommand(std::shared_ptr<port::IServerGateway> gateway);

    bcmd::VoidResult execute(std::string_view client_id);

private:
    std::shared_ptr<port::IServerGateway> gateway_{};
};

}  // namespace bcmd::client::application::usecase
