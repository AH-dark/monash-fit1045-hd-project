#include "bcmd/client/application/usecase/connect_to_server.hpp"

#include "bcmd/client/application/port/i_server_gateway.hpp"
#include "bcmd/shared/result.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace bcmd::client::application::usecase {

ConnectToServer::ConnectToServer(std::shared_ptr<port::IServerGateway> gateway)
    : gateway_(std::move(gateway)) {}

bcmd::Result<std::string> ConnectToServer::execute(std::string_view username) {
    return gateway_->connect(username);
}

}  // namespace bcmd::client::application::usecase
