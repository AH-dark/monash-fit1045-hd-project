#include "bcmd/server/adapter/grpc/broadcast_service_impl.hpp"

#include "bcmd/server/adapter/grpc/client_publisher.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/application/usecase/create_channel.hpp"
#include "bcmd/server/application/usecase/get_recent_messages.hpp"
#include "bcmd/server/application/usecase/join_channel.hpp"
#include "bcmd/server/application/usecase/leave_channel.hpp"
#include "bcmd/server/application/usecase/list_channels.hpp"
#include "bcmd/server/application/usecase/send_message.hpp"
#include "bcmd/server/application/usecase/subscribe_to_channel.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/server/domain/service/message_router.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"
#include "bcmd/shared/string_utils.hpp"
#include "bcmd/v1/broadcast.pb.h"

#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>

#include <chrono>
#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <thread>
#include <utility>

namespace bcmd::server::adapter::grpc {

BroadcastServiceImpl::BroadcastServiceImpl(
    std::shared_ptr<application::usecase::JoinChannel> join_channel,
    std::shared_ptr<application::usecase::LeaveChannel> leave_channel,
    std::shared_ptr<application::usecase::SendMessage> send_message,
    std::shared_ptr<application::usecase::ListChannels> list_channels,
    std::shared_ptr<application::usecase::CreateChannel> create_channel,
    std::shared_ptr<application::usecase::GetRecentMessages> get_recent,
    std::shared_ptr<application::usecase::SubscribeToChannel> subscribe,
    std::shared_ptr<GrpcClientPublisher> publisher,
    std::shared_ptr<application::port::IClientRegistry> client_registry)
    : join_channel_(std::move(join_channel)),
      leave_channel_(std::move(leave_channel)),
      send_message_(std::move(send_message)),
      list_channels_(std::move(list_channels)),
      create_channel_(std::move(create_channel)),
      get_recent_(std::move(get_recent)),
      subscribe_(std::move(subscribe)),
      publisher_(std::move(publisher)),
      client_registry_(std::move(client_registry)) {}

::grpc::Status BroadcastServiceImpl::Connect(::grpc::ServerContext* /*context*/,
                                             const bcmd::v1::ConnectRequest* request,
                                             bcmd::v1::ConnectResponse* response) {
    const auto USERNAME = domain::Username::create(bcmd::trim(request->username()));
    if (!USERNAME.has_value()) {
        return error_to_status(bcmd::Error::InvalidUsername);
    }

    auto session = client_registry_->registerClient(*USERNAME);
    if (!session.has_value()) {
        return error_to_status(session.error());
    }
    response->set_client_id(session->id().value());
    return ::grpc::Status::OK;
}

::grpc::Status BroadcastServiceImpl::Disconnect(::grpc::ServerContext* /*context*/,
                                                const bcmd::v1::DisconnectRequest* request,
                                                bcmd::v1::DisconnectResponse* /*response*/) {
    const auto CLIENT_ID = bcmd::ClientId::parse(bcmd::trim(request->client_id()));
    if (!CLIENT_ID.has_value()) {
        return error_to_status(bcmd::Error::ClientNotFound);
    }
    const auto REMOVED = client_registry_->remove(*CLIENT_ID);
    if (!REMOVED.has_value()) {
        return error_to_status(REMOVED.error());
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
        return error_to_status(bcmd::Error::ClientNotFound);
    }

    const auto trimmed_name = bcmd::trim(request->channel_name());
    auto created = create_channel_->execute(*CLIENT_ID, trimmed_name);
    if (!created.has_value()) {
        return error_to_status(created.error());
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
        return error_to_status(bcmd::Error::ClientNotFound);
    }

    const auto trimmed_channel = bcmd::trim(request->channel_id());
    if (const auto CHANNEL_ID = bcmd::ChannelId::parse(trimmed_channel); CHANNEL_ID.has_value()) {
        const auto JOINED = join_channel_->execute(*CLIENT_ID, *CHANNEL_ID);
        return JOINED.has_value() ? ::grpc::Status::OK : error_to_status(JOINED.error());
    }

    auto joined = join_channel_->executeByName(*CLIENT_ID, trimmed_channel);
    return joined.has_value() ? ::grpc::Status::OK : error_to_status(joined.error());
}

::grpc::Status BroadcastServiceImpl::LeaveChannel(::grpc::ServerContext* /*context*/,
                                                  const bcmd::v1::LeaveChannelRequest* request,
                                                  bcmd::v1::LeaveChannelResponse* /*response*/) {
    const auto CLIENT_ID = bcmd::ClientId::parse(bcmd::trim(request->client_id()));
    const auto CHANNEL_ID = bcmd::ChannelId::parse(bcmd::trim(request->channel_id()));
    if (!CLIENT_ID.has_value()) {
        return error_to_status(bcmd::Error::ClientNotFound);
    }
    if (!CHANNEL_ID.has_value()) {
        return error_to_status(bcmd::Error::ChannelNotFound);
    }

    const auto LEFT = leave_channel_->execute(*CLIENT_ID, *CHANNEL_ID);
    return LEFT.has_value() ? ::grpc::Status::OK : error_to_status(LEFT.error());
}

::grpc::Status BroadcastServiceImpl::SendMessage(::grpc::ServerContext* /*context*/,
                                                 const bcmd::v1::SendMessageRequest* request,
                                                 bcmd::v1::SendMessageResponse* response) {
    const auto CLIENT_ID = bcmd::ClientId::parse(bcmd::trim(request->client_id()));
    const auto CHANNEL_ID = bcmd::ChannelId::parse(bcmd::trim(request->channel_id()));
    if (!CLIENT_ID.has_value()) {
        return error_to_status(bcmd::Error::ClientNotFound);
    }
    if (!CHANNEL_ID.has_value()) {
        return error_to_status(bcmd::Error::ChannelNotFound);
    }

    const auto trimmed_content = bcmd::trim(request->content());
    auto message_id = send_message_->execute(*CLIENT_ID, *CHANNEL_ID, trimmed_content,
                                             domain::EchoPolicy::IncludeSender);
    if (!message_id.has_value()) {
        return error_to_status(message_id.error());
    }

    response->set_message_id(message_id->value());
    return ::grpc::Status::OK;
}

::grpc::Status BroadcastServiceImpl::Heartbeat(::grpc::ServerContext* /*context*/,
                                               const bcmd::v1::HeartbeatRequest* request,
                                               bcmd::v1::HeartbeatResponse* /*response*/) {
    const auto CLIENT_ID = bcmd::ClientId::parse(bcmd::trim(request->client_id()));
    if (!CLIENT_ID.has_value()) {
        return error_to_status(bcmd::Error::ClientNotFound);
    }
    const auto TOUCHED = client_registry_->touchHeartbeat(*CLIENT_ID);
    if (!TOUCHED.has_value()) {
        return error_to_status(TOUCHED.error());
    }
    return ::grpc::Status::OK;
}

::grpc::Status BroadcastServiceImpl::SubscribeToChannel(
    ::grpc::ServerContext* context, const bcmd::v1::SubscribeRequest* request,
    ::grpc::ServerWriter<bcmd::v1::ChannelEvent>* writer) {
    const auto CLIENT_ID = bcmd::ClientId::parse(bcmd::trim(request->client_id()));
    const auto CHANNEL_ID = bcmd::ChannelId::parse(bcmd::trim(request->channel_id()));
    if (!CLIENT_ID.has_value()) {
        return error_to_status(bcmd::Error::ClientNotFound);
    }
    if (!CHANNEL_ID.has_value()) {
        return error_to_status(bcmd::Error::ChannelNotFound);
    }

    publisher_->registerSubscriber(*CLIENT_ID, writer);
    const auto REPLAYED = subscribe_->execute(*CLIENT_ID, *CHANNEL_ID, request->replay_count());
    if (!REPLAYED.has_value()) {
        publisher_->unregisterSubscriber(*CLIENT_ID);
        return error_to_status(REPLAYED.error());
    }

    while (!context->IsCancelled()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    publisher_->unregisterSubscriber(*CLIENT_ID);
    return ::grpc::Status::OK;
}

::grpc::Status BroadcastServiceImpl::error_to_status(bcmd::Error error) {
    ::grpc::StatusCode code{::grpc::StatusCode::INTERNAL};
    if (error == bcmd::Error::ChannelNotFound || error == bcmd::Error::ClientNotFound ||
        error == bcmd::Error::NotAMember) {
        code = ::grpc::StatusCode::NOT_FOUND;
    } else if (error == bcmd::Error::AlreadyMember || error == bcmd::Error::ChannelAlreadyExists ||
               error == bcmd::Error::ClientAlreadyExists) {
        code = ::grpc::StatusCode::ALREADY_EXISTS;
    } else if (error == bcmd::Error::InvalidUsername || error == bcmd::Error::InvalidChannelName ||
               error == bcmd::Error::MessageEmpty || error == bcmd::Error::MessageTooLong) {
        code = ::grpc::StatusCode::INVALID_ARGUMENT;
    }
    return {code, std::string(bcmd::error_message(error))};
}

}  // namespace bcmd::server::adapter::grpc
