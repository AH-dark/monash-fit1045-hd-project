#include "bcmd/server/adapter/grpc/broadcast_service_impl.hpp"

#include "bcmd/server/adapter/grpc/client_publisher.hpp"
#include "bcmd/server/adapter/grpc/detail/error_to_status.hpp"
#include "bcmd/server/application/port/i_channel_list_publisher.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/application/usecase/create_channel.hpp"
#include "bcmd/server/application/usecase/join_channel.hpp"
#include "bcmd/server/application/usecase/leave_channel.hpp"
#include "bcmd/server/application/usecase/list_channels.hpp"
#include "bcmd/server/application/usecase/list_messages.hpp"
#include "bcmd/server/application/usecase/send_message.hpp"
#include "bcmd/server/application/usecase/subscribe_to_channel.hpp"
#include "bcmd/server/application/usecase/subscribe_to_channel_list.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/server/domain/service/message_router.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"
#include "bcmd/shared/string_utils.hpp"
#include "bcmd/v1/broadcast.pb.h"

#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace bcmd::server::adapter::grpc {

BroadcastServiceImpl::BroadcastServiceImpl(
    std::shared_ptr<application::usecase::JoinChannel> join_channel,
    std::shared_ptr<application::usecase::LeaveChannel> leave_channel,
    std::shared_ptr<application::usecase::SendMessage> send_message,
    std::shared_ptr<application::usecase::ListChannels> list_channels,
    std::shared_ptr<application::usecase::CreateChannel> create_channel,
    std::shared_ptr<application::usecase::ListMessages> list_messages,
    std::shared_ptr<application::usecase::SubscribeToChannel> subscribe,
    std::shared_ptr<GrpcClientPublisher> publisher,
    std::shared_ptr<application::port::IClientRegistry> client_registry,
    std::shared_ptr<application::usecase::SubscribeToChannelList> subscribe_channel_list,
    std::shared_ptr<application::port::IChannelListPublisher> channel_list_publisher)
    : join_channel_(std::move(join_channel)),
      leave_channel_(std::move(leave_channel)),
      send_message_(std::move(send_message)),
      list_channels_(std::move(list_channels)),
      create_channel_(std::move(create_channel)),
      list_messages_(std::move(list_messages)),
      subscribe_(std::move(subscribe)),
      publisher_(std::move(publisher)),
      client_registry_(std::move(client_registry)),
      subscribe_channel_list_(std::move(subscribe_channel_list)),
      channel_list_publisher_(std::move(channel_list_publisher)) {}

::grpc::Status BroadcastServiceImpl::Connect(::grpc::ServerContext* /*context*/,
                                             const bcmd::v1::ConnectRequest* request,
                                             bcmd::v1::ConnectResponse* response) {
    const auto USERNAME = domain::Username::create(bcmd::trim(request->username()));
    if (!USERNAME.has_value()) {
        return detail::errorToStatus(bcmd::Error::InvalidUsername);
    }

    auto session = client_registry_->registerClient(*USERNAME);
    if (!session.has_value()) {
        return detail::errorToStatus(session.error());
    }
    response->set_client_id(session->id().value());
    return ::grpc::Status::OK;
}

::grpc::Status BroadcastServiceImpl::Disconnect(::grpc::ServerContext* /*context*/,
                                                const bcmd::v1::DisconnectRequest* request,
                                                bcmd::v1::DisconnectResponse* /*response*/) {
    const auto CLIENT_ID = bcmd::ClientId::parse(bcmd::trim(request->client_id()));
    if (!CLIENT_ID.has_value()) {
        return detail::errorToStatus(bcmd::Error::ClientNotFound);
    }
    auto session = client_registry_->findById(*CLIENT_ID);
    if (!session.has_value()) {
        return detail::errorToStatus(session.error());
    }
    // Snapshot channel ids — joined_channels_ is not stable after LeaveChannel mutates.
    const std::vector<bcmd::ChannelId> channels(session->joinedChannels().begin(),
                                                session->joinedChannels().end());
    for (const auto& channel_id : channels) {
        const auto left = leave_channel_->execute(*CLIENT_ID, channel_id);
        if (!left.has_value() && left.error() != bcmd::Error::NotAMember &&
            left.error() != bcmd::Error::ChannelNotFound) {
            spdlog::warn("disconnect: leave channel {} for client {} failed: {}",
                         channel_id.value(), CLIENT_ID->value(), bcmd::error_message(left.error()));
        }
    }
    const auto REMOVED = client_registry_->remove(*CLIENT_ID);
    if (!REMOVED.has_value()) {
        return detail::errorToStatus(REMOVED.error());
    }
    publisher_->unregisterSubscriber(*CLIENT_ID);
    return ::grpc::Status::OK;
}

::grpc::Status BroadcastServiceImpl::ListChannels(::grpc::ServerContext* /*context*/,
                                                  const bcmd::v1::ListChannelsRequest* /*request*/,
                                                  bcmd::v1::ListChannelsResponse* response) {
    for (const auto& channel : list_channels_->execute()) {
        auto* summary = response->add_channels();
        summary->set_id(channel.id().value());
        summary->set_name(channel.name().value());
        summary->set_member_count(static_cast<std::int32_t>(channel.memberCount()));
    }
    return ::grpc::Status::OK;
}

::grpc::Status BroadcastServiceImpl::CreateChannel(::grpc::ServerContext* /*context*/,
                                                   const bcmd::v1::CreateChannelRequest* request,
                                                   bcmd::v1::CreateChannelResponse* response) {
    const auto CLIENT_ID = bcmd::ClientId::parse(bcmd::trim(request->client_id()));
    if (!CLIENT_ID.has_value()) {
        return detail::errorToStatus(bcmd::Error::ClientNotFound);
    }

    const auto trimmed_name = bcmd::trim(request->channel_name());
    auto created = create_channel_->execute(*CLIENT_ID, trimmed_name);
    if (!created.has_value()) {
        return detail::errorToStatus(created.error());
    }
    response->set_channel_id(created->value());
    response->set_channel_name(std::string{trimmed_name});
    return ::grpc::Status::OK;
}

::grpc::Status BroadcastServiceImpl::JoinChannel(::grpc::ServerContext* /*context*/,
                                                 const bcmd::v1::JoinChannelRequest* request,
                                                 bcmd::v1::JoinChannelResponse* /*response*/) {
    const auto CLIENT_ID = bcmd::ClientId::parse(bcmd::trim(request->client_id()));
    if (!CLIENT_ID.has_value()) {
        return detail::errorToStatus(bcmd::Error::ClientNotFound);
    }

    const auto trimmed_channel = bcmd::trim(request->channel_id());
    if (const auto CHANNEL_ID = bcmd::ChannelId::parse(trimmed_channel); CHANNEL_ID.has_value()) {
        const auto JOINED = join_channel_->execute(*CLIENT_ID, *CHANNEL_ID);
        return JOINED.has_value() ? ::grpc::Status::OK : detail::errorToStatus(JOINED.error());
    }

    auto joined = join_channel_->executeByName(*CLIENT_ID, trimmed_channel);
    return joined.has_value() ? ::grpc::Status::OK : detail::errorToStatus(joined.error());
}

::grpc::Status BroadcastServiceImpl::LeaveChannel(::grpc::ServerContext* /*context*/,
                                                  const bcmd::v1::LeaveChannelRequest* request,
                                                  bcmd::v1::LeaveChannelResponse* /*response*/) {
    const auto CLIENT_ID = bcmd::ClientId::parse(bcmd::trim(request->client_id()));
    const auto CHANNEL_ID = bcmd::ChannelId::parse(bcmd::trim(request->channel_id()));
    if (!CLIENT_ID.has_value()) {
        return detail::errorToStatus(bcmd::Error::ClientNotFound);
    }
    if (!CHANNEL_ID.has_value()) {
        return detail::errorToStatus(bcmd::Error::ChannelNotFound);
    }

    const auto LEFT = leave_channel_->execute(*CLIENT_ID, *CHANNEL_ID);
    return LEFT.has_value() ? ::grpc::Status::OK : detail::errorToStatus(LEFT.error());
}

::grpc::Status BroadcastServiceImpl::SendMessage(::grpc::ServerContext* /*context*/,
                                                 const bcmd::v1::SendMessageRequest* request,
                                                 bcmd::v1::SendMessageResponse* response) {
    const auto CLIENT_ID = bcmd::ClientId::parse(bcmd::trim(request->client_id()));
    const auto CHANNEL_ID = bcmd::ChannelId::parse(bcmd::trim(request->channel_id()));
    if (!CLIENT_ID.has_value()) {
        return detail::errorToStatus(bcmd::Error::ClientNotFound);
    }
    if (!CHANNEL_ID.has_value()) {
        return detail::errorToStatus(bcmd::Error::ChannelNotFound);
    }

    const auto trimmed_content = bcmd::trim(request->content());
    auto message_id = send_message_->execute(*CLIENT_ID, *CHANNEL_ID, trimmed_content,
                                             domain::EchoPolicy::IncludeSender);
    if (!message_id.has_value()) {
        return detail::errorToStatus(message_id.error());
    }

    response->set_message_id(message_id->value());
    return ::grpc::Status::OK;
}

::grpc::Status BroadcastServiceImpl::ListMessages(::grpc::ServerContext* /*context*/,
                                                  const bcmd::v1::ListMessagesRequest* request,
                                                  bcmd::v1::ListMessagesResponse* response) {
    const auto CLIENT_ID = bcmd::ClientId::parse(bcmd::trim(request->client_id()));
    const auto CHANNEL_ID = bcmd::ChannelId::parse(bcmd::trim(request->channel_id()));
    if (!CLIENT_ID.has_value()) {
        return detail::errorToStatus(bcmd::Error::ClientNotFound);
    }
    if (!CHANNEL_ID.has_value()) {
        return detail::errorToStatus(bcmd::Error::ChannelNotFound);
    }

    std::optional<bcmd::MessageId> cursor;
    if (const auto trimmed_cursor = bcmd::trim(request->before_message_id());
        !trimmed_cursor.empty()) {
        auto parsed = bcmd::MessageId::parse(trimmed_cursor);
        if (!parsed.has_value()) {
            return detail::errorToStatus(bcmd::Error::InvalidArgument);
        }
        cursor = *parsed;
    }

    auto result = list_messages_->execute(*CLIENT_ID, *CHANNEL_ID, cursor, request->limit());
    if (!result.has_value()) {
        return detail::errorToStatus(result.error());
    }

    for (const auto& message : result->messages) {
        auto* event = response->add_messages();
        event->set_message_id(message.id().value());
        event->set_channel_id(message.channelId().value());
        event->set_sender_id(message.senderId().value());
        if (auto username = client_registry_->lookupUsername(message.senderId());
            username.has_value()) {
            event->set_sender_name(username->value());
        }
        event->set_content(message.content().value());
        event->set_sent_at_ms(std::chrono::duration_cast<std::chrono::milliseconds>(
                                  message.sentAt().time_since_epoch())
                                  .count());
    }
    response->set_has_more(result->has_more);
    return ::grpc::Status::OK;
}

::grpc::Status BroadcastServiceImpl::Heartbeat(::grpc::ServerContext* /*context*/,
                                               const bcmd::v1::HeartbeatRequest* request,
                                               bcmd::v1::HeartbeatResponse* /*response*/) {
    const auto CLIENT_ID = bcmd::ClientId::parse(bcmd::trim(request->client_id()));
    if (!CLIENT_ID.has_value()) {
        return detail::errorToStatus(bcmd::Error::ClientNotFound);
    }
    const auto TOUCHED = client_registry_->touchHeartbeat(*CLIENT_ID);
    if (!TOUCHED.has_value()) {
        return detail::errorToStatus(TOUCHED.error());
    }
    return ::grpc::Status::OK;
}

::grpc::Status BroadcastServiceImpl::SubscribeToChannel(
    ::grpc::ServerContext* context, const bcmd::v1::SubscribeRequest* request,
    ::grpc::ServerWriter<bcmd::v1::ChannelEvent>* writer) {
    const auto CLIENT_ID = bcmd::ClientId::parse(bcmd::trim(request->client_id()));
    const auto CHANNEL_ID = bcmd::ChannelId::parse(bcmd::trim(request->channel_id()));
    if (!CLIENT_ID.has_value()) {
        return detail::errorToStatus(bcmd::Error::ClientNotFound);
    }
    if (!CHANNEL_ID.has_value()) {
        return detail::errorToStatus(bcmd::Error::ChannelNotFound);
    }

    publisher_->registerSubscriber(*CLIENT_ID, writer);
    const auto AUTHORIZED = subscribe_->execute(*CLIENT_ID, *CHANNEL_ID);
    if (!AUTHORIZED.has_value()) {
        publisher_->unregisterSubscriber(*CLIENT_ID);
        return detail::errorToStatus(AUTHORIZED.error());
    }

    while (!context->IsCancelled()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    publisher_->unregisterSubscriber(*CLIENT_ID);
    return ::grpc::Status::OK;
}

::grpc::Status BroadcastServiceImpl::SubscribeToChannelList(
    ::grpc::ServerContext* context, const bcmd::v1::SubscribeChannelListRequest* request,
    ::grpc::ServerWriter<bcmd::v1::ChannelListEvent>* writer) {
    const auto CLIENT_ID = bcmd::ClientId::parse(bcmd::trim(request->client_id()));
    if (!CLIENT_ID.has_value()) {
        return detail::errorToStatus(bcmd::Error::ClientNotFound);
    }

    const auto SUBSCRIBED = subscribe_channel_list_->execute(*CLIENT_ID, writer);
    if (!SUBSCRIBED.has_value()) {
        return detail::errorToStatus(SUBSCRIBED.error());
    }

    while (!context->IsCancelled()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    channel_list_publisher_->unregisterSubscriber(*CLIENT_ID);
    return ::grpc::Status::OK;
}

}  // namespace bcmd::server::adapter::grpc
