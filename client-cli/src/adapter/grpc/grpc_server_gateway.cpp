#include "bcmd/client/adapter/grpc/grpc_server_gateway.hpp"

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "bcmd/client/domain/inbox_message.hpp"
#include "bcmd/shared/result.hpp"
#include "bcmd/v1/broadcast.grpc.pb.h"

namespace bcmd::client::adapter::grpc {

GrpcServerGateway::GrpcServerGateway(std::shared_ptr<::grpc::Channel> channel)
    : stub_(bcmd::v1::BroadcastService::NewStub(std::move(channel))) {}

bcmd::Result<std::string> GrpcServerGateway::connect(std::string_view username) {
    ::grpc::ClientContext context;
    bcmd::v1::ConnectRequest request;
    bcmd::v1::ConnectResponse response;
    request.set_username(std::string{username});

    const auto status = stub_->Connect(&context, request, &response);
    if (!status.ok()) {
        return std::unexpected(grpc_status_to_error(status));
    }
    return response.client_id();
}

bcmd::VoidResult GrpcServerGateway::disconnect(std::string_view client_id) {
    ::grpc::ClientContext context;
    bcmd::v1::DisconnectRequest request;
    bcmd::v1::DisconnectResponse response;
    request.set_client_id(std::string{client_id});

    const auto status = stub_->Disconnect(&context, request, &response);
    if (!status.ok()) {
        return std::unexpected(grpc_status_to_error(status));
    }
    return {};
}

bcmd::Result<std::vector<application::port::ChannelInfo>> GrpcServerGateway::listChannels() {
    ::grpc::ClientContext context;
    bcmd::v1::ListChannelsRequest request;
    bcmd::v1::ListChannelsResponse response;

    const auto status = stub_->ListChannels(&context, request, &response);
    if (!status.ok()) {
        return std::unexpected(grpc_status_to_error(status));
    }

    std::vector<application::port::ChannelInfo> channels;
    channels.reserve(static_cast<std::size_t>(response.channels_size()));
    for (const auto& channel : response.channels()) {
        channels.push_back(application::port::ChannelInfo{.id = channel.id(),
                                                          .name = channel.name(),
                                                          .member_count = channel.member_count()});
    }
    return channels;
}

bcmd::VoidResult GrpcServerGateway::joinChannel(std::string_view client_id,
                                                std::string_view channel_id) {
    ::grpc::ClientContext context;
    bcmd::v1::JoinChannelRequest request;
    bcmd::v1::JoinChannelResponse response;
    request.set_client_id(std::string{client_id});
    request.set_channel_id(std::string{channel_id});

    const auto status = stub_->JoinChannel(&context, request, &response);
    if (!status.ok()) {
        return std::unexpected(grpc_status_to_error(status));
    }
    return {};
}

bcmd::Result<std::string> GrpcServerGateway::joinChannelByName(std::string_view client_id,
                                                               std::string_view channel_name) {
    const auto channels = listChannels();
    if (!channels.has_value()) {
        return std::unexpected(channels.error());
    }

    for (const auto& channel : *channels) {
        if (channel.name == channel_name) {
            const auto joined = joinChannel(client_id, channel.id);
            if (!joined.has_value()) {
                return std::unexpected(joined.error());
            }
            return channel.id;
        }
    }

    const auto joined = joinChannel(client_id, channel_name);
    if (!joined.has_value()) {
        return std::unexpected(joined.error());
    }
    return std::string{channel_name};
}

bcmd::VoidResult GrpcServerGateway::leaveChannel(std::string_view client_id,
                                                 std::string_view channel_id) {
    ::grpc::ClientContext context;
    bcmd::v1::LeaveChannelRequest request;
    bcmd::v1::LeaveChannelResponse response;
    request.set_client_id(std::string{client_id});
    request.set_channel_id(std::string{channel_id});

    const auto status = stub_->LeaveChannel(&context, request, &response);
    if (!status.ok()) {
        return std::unexpected(grpc_status_to_error(status));
    }
    return {};
}

bcmd::Result<std::string> GrpcServerGateway::sendMessage(std::string_view client_id,
                                                         std::string_view channel_id,
                                                         std::string_view content) {
    ::grpc::ClientContext context;
    bcmd::v1::SendMessageRequest request;
    bcmd::v1::SendMessageResponse response;
    request.set_client_id(std::string{client_id});
    request.set_channel_id(std::string{channel_id});
    request.set_content(std::string{content});

    const auto status = stub_->SendMessage(&context, request, &response);
    if (!status.ok()) {
        return std::unexpected(grpc_status_to_error(status));
    }
    return response.message_id();
}

bcmd::VoidResult GrpcServerGateway::subscribeToChannel(std::string_view client_id,
                                                       std::string_view channel_id,
                                                       std::uint32_t replay_count,
                                                       MessageCallback callback) {
    ::grpc::ClientContext context;
    bcmd::v1::SubscribeRequest request;
    request.set_client_id(std::string{client_id});
    request.set_channel_id(std::string{channel_id});
    request.set_replay_count(replay_count);

    auto reader = stub_->SubscribeToChannel(&context, request);
    bcmd::v1::ChannelEvent event;
    bool in_history = replay_count > 0;
    while (reader->Read(&event)) {
        if (event.has_message()) {
            const auto& message = event.message();
            domain::InboxMessage inbox_message;
            inbox_message.message_id = message.message_id();
            inbox_message.channel_id = message.channel_id();
            inbox_message.sender_name = message.sender_name();
            inbox_message.content = message.content();
            inbox_message.sent_at_ms = message.sent_at_ms();
            inbox_message.is_history = in_history || message.from_replay();
            callback(std::move(inbox_message));
        } else if (event.has_replay_complete()) {
            in_history = false;
        }
    }

    const auto status = reader->Finish();
    if (!status.ok()) {
        return std::unexpected(grpc_status_to_error(status));
    }
    return {};
}

bcmd::Error GrpcServerGateway::grpc_status_to_error(const ::grpc::Status& status) {
    switch (status.error_code()) {
        case ::grpc::StatusCode::NOT_FOUND:       return bcmd::Error::ChannelNotFound;
        case ::grpc::StatusCode::ALREADY_EXISTS:  return bcmd::Error::AlreadyMember;
        case ::grpc::StatusCode::INVALID_ARGUMENT:return bcmd::Error::InvalidChannelName;
        case ::grpc::StatusCode::UNAVAILABLE:
        case ::grpc::StatusCode::UNKNOWN:
        case ::grpc::StatusCode::INTERNAL:
        default:                                  return bcmd::Error::NetworkError;
    }
}

}  // namespace bcmd::client::adapter::grpc
