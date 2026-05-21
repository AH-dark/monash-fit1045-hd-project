#include "bcmd/client/application/usecase/join_channel_command.hpp"

#include "bcmd/client/application/port/i_server_gateway.hpp"
#include "bcmd/shared/result.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace bcmd::client::application::usecase {

JoinChannelCommand::JoinChannelCommand(std::shared_ptr<port::IServerGateway> gateway)
    : gateway_(std::move(gateway)) {}

bcmd::Result<std::string> JoinChannelCommand::execute(std::string_view client_id,
                                                      std::string_view channel_name) {
    return gateway_->joinChannelByName(client_id, channel_name);
}

}  // namespace bcmd::client::application::usecase
