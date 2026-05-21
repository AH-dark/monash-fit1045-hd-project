#pragma once

#include "bcmd/client/application/port/i_server_gateway.hpp"
#include "bcmd/shared/result.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace bcmd::client::application::usecase {

class ConnectToServer {
public:
    explicit ConnectToServer(std::shared_ptr<port::IServerGateway> gateway);

    bcmd::Result<std::string> execute(std::string_view username);

private:
    std::shared_ptr<port::IServerGateway> gateway_;
};

}  // namespace bcmd::client::application::usecase
