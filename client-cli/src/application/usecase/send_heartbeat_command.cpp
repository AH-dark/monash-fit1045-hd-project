#include "bcmd/client/application/usecase/send_heartbeat_command.hpp"

#include "bcmd/client/application/port/i_server_gateway.hpp"
#include "bcmd/shared/result.hpp"

#include <memory>
#include <string_view>
#include <utility>

namespace bcmd::client::application::usecase {

SendHeartbeatCommand::SendHeartbeatCommand(std::shared_ptr<port::IServerGateway> gateway)
    : gateway_(std::move(gateway)) {}

bcmd::VoidResult SendHeartbeatCommand::execute(std::string_view client_id) {
    return gateway_->sendHeartbeat(client_id);
}

}  // namespace bcmd::client::application::usecase
