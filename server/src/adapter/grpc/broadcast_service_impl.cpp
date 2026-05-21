#include "bcmd/server/adapter/grpc/broadcast_service_impl.hpp"

#include <expected>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "bcmd/server/domain/model/message_content.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/result.hpp"

namespace bcmd::server::adapter::grpc {

BroadcastServiceImpl::BroadcastServiceImpl(
    std::shared_ptr<application::usecase::JoinChannel> join_channel,
    std::shared_ptr<application::usecase::LeaveChannel> leave_channel,
    std::shared_ptr<application::usecase::SendMessage> send_message,
    std::shared_ptr<application::usecase::ListChannels> list_channels,
    std::shared_ptr<application::usecase::GetRecentMessages> get_recent,
    std::shared_ptr<application::usecase::SubscribeToChannel> subscribe,
    std::shared_ptr<GrpcClientPublisher> publisher,
    std::shared_ptr<application::port::IClientRegistry> client_registry)
    : join_channel_(std::move(join_channel)),
      leave_channel_(std::move(leave_channel)),
      send_message_(std::move(send_message)),
      list_channels_(std::move(list_channels)),
      get_recent_(std::move(get_recent)),
      subscribe_(std::move(subscribe)),
      publisher_(std::move(publisher)),
      client_registry_(std::move(client_registry)) {}

::grpc::Status BroadcastServiceImpl::Connect(::grpc::ServerContext* /*context*/,
                                             const bcmd::v1::ConnectRequest* request,
                                             bcmd::v1::ConnectResponse* response) {
    const auto USERNAME = domain::Username::create(request->username());
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
    const auto CLIENT_ID = bcmd::ClientId::parse(request->client_id());
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

::grpc::Status BroadcastServiceImpl::JoinChannel(::grpc::ServerContext* /*context*/,
                                                 const bcmd::v1::JoinChannelRequest* request,
                                                 bcmd::v1::JoinChannelResponse* /*response*/) {
    const auto CLIENT_ID = bcmd::ClientId::parse(request->client_id());
    if (!CLIENT_ID.has_value()) {
        return error_to_status(bcmd::Error::ClientNotFound);
    }

    if (const auto CHANNEL_ID = bcmd::ChannelId::parse(request->channel_id()); CHANNEL_ID.has_value()) {
        const auto JOINED = join_channel_->execute(*CLIENT_ID, *CHANNEL_ID);
        return JOINED.has_value() ? ::grpc::Status::OK : error_to_status(JOINED.error());
    }

    auto joined = join_channel_->executeByName(*CLIENT_ID, request->channel_id());
    return joined.has_value() ? ::grpc::Status::OK : error_to_status(joined.error());
}

::grpc::Status BroadcastServiceImpl::LeaveChannel(::grpc::ServerContext* /*context*/,
                                                  const bcmd::v1::LeaveChannelRequest* request,
                                                  bcmd::v1::LeaveChannelResponse* /*response*/) {
    const auto CLIENT_ID = bcmd::ClientId::parse(request->client_id());
    const auto CHANNEL_ID = bcmd::ChannelId::parse(request->channel_id());
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
    const auto CLIENT_ID = bcmd::ClientId::parse(request->client_id());
    const auto CHANNEL_ID = bcmd::ChannelId::parse(request->channel_id());
    if (!CLIENT_ID.has_value()) {
        return error_to_status(bcmd::Error::ClientNotFound);
    }
    if (!CHANNEL_ID.has_value()) {
        return error_to_status(bcmd::Error::ChannelNotFound);
    }

    auto message_id = send_message_->execute(*CLIENT_ID, *CHANNEL_ID, request->content());
    if (!message_id.has_value()) {
        return error_to_status(message_id.error());
    }

    response->set_message_id(message_id->value());
    return ::grpc::Status::OK;
}

::grpc::Status BroadcastServiceImpl::SubscribeToChannel(
    ::grpc::ServerContext* context,
    const bcmd::v1::SubscribeRequest* request,
    ::grpc::ServerWriter<bcmd::v1::ChannelEvent>* writer) {
    const auto CLIENT_ID = bcmd::ClientId::parse(request->client_id());
    const auto CHANNEL_ID = bcmd::ChannelId::parse(request->channel_id());
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
    switch (error) {
        case bcmd::Error::ChannelNotFound:
        case bcmd::Error::ClientNotFound:
        case bcmd::Error::NotAMember:
            return {::grpc::StatusCode::NOT_FOUND, std::string(bcmd::error_message(error))};
        case bcmd::Error::AlreadyMember:
        case bcmd::Error::ChannelAlreadyExists:
        case bcmd::Error::ClientAlreadyExists:
            return {::grpc::StatusCode::ALREADY_EXISTS, std::string(bcmd::error_message(error))};
        case bcmd::Error::InvalidUsername:
        case bcmd::Error::InvalidChannelName:
        case bcmd::Error::MessageEmpty:
        case bcmd::Error::MessageTooLong:
            return {::grpc::StatusCode::INVALID_ARGUMENT, std::string(bcmd::error_message(error))};
        default:
            return {::grpc::StatusCode::INTERNAL, std::string(bcmd::error_message(error))};
    }
}

}  // namespace bcmd::server::adapter::grpc
