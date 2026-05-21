#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "bcmd/client/application/port/i_server_gateway.hpp"
#include "bcmd/shared/result.hpp"

namespace bcmd::client::application::usecase {

class SendMessageCommand {
public:
    explicit SendMessageCommand(std::shared_ptr<port::IServerGateway> gateway);

    bcmd::Result<std::string> execute(std::string_view client_id,
                                      std::string_view channel_id,
                                      std::string_view content);

private:
    std::shared_ptr<port::IServerGateway> gateway_;
};

}  // namespace bcmd::client::application::usecase
