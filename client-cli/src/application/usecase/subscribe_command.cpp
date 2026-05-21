#include "bcmd/client/application/usecase/subscribe_command.hpp"

#include "bcmd/client/application/port/i_presenter.hpp"
#include "bcmd/client/application/port/i_server_gateway.hpp"
#include "bcmd/shared/result.hpp"

#include <memory>
#include <string_view>
#include <utility>

namespace bcmd::client::application::usecase {

SubscribeCommand::SubscribeCommand(std::shared_ptr<port::IServerGateway> gateway,
                                   std::shared_ptr<port::IPresenter> presenter)
    : gateway_(std::move(gateway)), presenter_(std::move(presenter)) {}

bcmd::VoidResult SubscribeCommand::execute(std::string_view client_id, std::string_view channel_id,
                                           std::uint32_t replay_count) {
    auto result =
        gateway_->subscribeToChannel(client_id, channel_id, replay_count,
                                     [presenter = presenter_](domain::InboxMessage message) {
                                         presenter->showMessage(std::move(message));
                                     });

    if (!result.has_value()) {
        presenter_->showError(bcmd::error_message(result.error()));
    }
    return result;
}

}  // namespace bcmd::client::application::usecase
