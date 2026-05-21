#pragma once

#include <memory>
#include <string_view>

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/application/port/i_message_repository.hpp"
#include "bcmd/server/domain/service/message_router.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

namespace bcmd::server::application::usecase {

class SendMessage {
public:
    SendMessage(std::shared_ptr<port::IChannelRepository> channels,
                std::shared_ptr<port::IClientRegistry> clients,
                std::shared_ptr<port::IMessageRepository> messages,
                std::shared_ptr<port::IMessagePublisher> publisher);

    // `echo_policy` follows MessageRouter semantics; ExcludeSender by default.
    bcmd::Result<bcmd::MessageId> execute(
        const bcmd::ClientId& sender_id,
        const bcmd::ChannelId& channel_id,
        std::string_view content,
        domain::EchoPolicy echo_policy = domain::EchoPolicy::ExcludeSender);

private:
    std::shared_ptr<port::IChannelRepository> channels_;
    std::shared_ptr<port::IClientRegistry> clients_;
    std::shared_ptr<port::IMessageRepository> messages_;
    std::shared_ptr<port::IMessagePublisher> publisher_;
};

}  // namespace bcmd::server::application::usecase
