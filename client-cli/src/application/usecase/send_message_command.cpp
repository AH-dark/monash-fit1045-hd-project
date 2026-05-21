#include "bcmd/client/application/usecase/send_message_command.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "bcmd/client/application/port/i_server_gateway.hpp"

namespace bcmd::client::application::usecase {

SendMessageCommand::SendMessageCommand(std::shared_ptr<port::IServerGateway> gateway)
    : gateway_(std::move(gateway)) {}

bcmd::Result<std::string> SendMessageCommand::execute(std::string_view client_id,
                                                       std::string_view channel_id,
                                                       std::string_view content) {
    return gateway_->sendMessage(client_id, channel_id, content);
}

}  // namespace bcmd::client::application::usecase
