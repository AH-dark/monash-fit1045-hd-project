#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "bcmd/client/application/port/i_presenter.hpp"
#include "bcmd/client/application/port/i_server_gateway.hpp"
#include "bcmd/shared/result.hpp"

namespace bcmd::client::application::usecase {

class SubscribeCommand {
public:
    SubscribeCommand(std::shared_ptr<port::IServerGateway> gateway,
                     std::shared_ptr<port::IPresenter> presenter);

    // Starts subscription on the caller thread; intended for a background network thread.
    bcmd::VoidResult execute(std::string_view client_id,
                             std::string_view channel_id,
                             std::uint32_t replay_count);

private:
    std::shared_ptr<port::IServerGateway> gateway_;
    std::shared_ptr<port::IPresenter> presenter_;
};

}  // namespace bcmd::client::application::usecase
