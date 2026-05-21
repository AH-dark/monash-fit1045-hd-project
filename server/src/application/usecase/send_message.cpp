#include "bcmd/server/application/usecase/send_message.hpp"

#include <expected>
#include <memory>
#include <string_view>
#include <utility>

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/application/port/i_message_repository.hpp"
#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/server/domain/model/message_content.hpp"
#include "bcmd/server/domain/service/message_router.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

namespace bcmd::server::application::usecase {

SendMessage::SendMessage(std::shared_ptr<port::IChannelRepository> channels,
                         std::shared_ptr<port::IClientRegistry> clients,
                         std::shared_ptr<port::IMessageRepository> messages,
                         std::shared_ptr<port::IMessagePublisher> publisher)
    : channels_(std::move(channels)),
      clients_(std::move(clients)),
      messages_(std::move(messages)),
      publisher_(std::move(publisher)) {}

bcmd::Result<bcmd::MessageId> SendMessage::execute(const bcmd::ClientId& sender_id,
                                                    const bcmd::ChannelId& channel_id,
                                                    std::string_view content,
                                                    domain::EchoPolicy echo_policy) {
    auto sender = clients_->findById(sender_id);
    if (!sender.has_value()) {
        return std::unexpected(sender.error());
    }
    auto channel = channels_->findById(channel_id);
    if (!channel.has_value()) {
        return std::unexpected(channel.error());
    }
    if (!channel->hasMember(sender_id)) {
        return std::unexpected(bcmd::Error::NotAMember);
    }

    auto validated_content = domain::MessageContent::create(content);
    if (!validated_content.has_value()) {
        return std::unexpected(content.find_first_not_of(" \t\n\r\f\v") ==
                                       std::string_view::npos
                                   ? bcmd::Error::MessageEmpty
                                   : bcmd::Error::MessageTooLong);
    }

    domain::Message message{bcmd::MessageId::generate(), sender_id, channel_id,
                            *validated_content};

    if (auto saved = messages_->save(message); !saved.has_value()) {
        return std::unexpected(saved.error());
    }

    for (const auto& recipient : domain::MessageRouter::recipientsFor(*channel, message,
                                                                       echo_policy)) {
        publisher_->publish(recipient, message, /*from_replay=*/false);
    }

    return message.id();
}

}  // namespace bcmd::server::application::usecase
