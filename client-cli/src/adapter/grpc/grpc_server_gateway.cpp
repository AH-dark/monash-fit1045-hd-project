#include "bcmd/client/adapter/grpc/grpc_server_gateway.hpp"

#include "bcmd/client/application/port/i_server_gateway.hpp"
#include "bcmd/client/domain/inbox_message.hpp"
#include "bcmd/shared/result.hpp"
#include "bcmd/shared/string_utils.hpp"
#include "bcmd/v1/broadcast.grpc.pb.h"
#include "bcmd/v1/broadcast.pb.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace bcmd::client::adapter::grpc {

GrpcServerGateway::GrpcServerGateway(std::shared_ptr<::grpc::Channel> channel)
    : stub_(bcmd::v1::BroadcastService::NewStub(std::move(channel))) {}

bcmd::Result<std::string> GrpcServerGateway::connect(std::string_view username) {
    ::grpc::ClientContext context;
    bcmd::v1::ConnectRequest request;
    bcmd::v1::ConnectResponse response;
    request.set_username(bcmd::trim_copy(username));

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
    request.set_client_id(bcmd::trim_copy(client_id));

    const auto status = stub_->Disconnect(&context, request, &response);
    if (!status.ok()) {
        return std::unexpected(grpc_status_to_error(status));
    }
    return {};
}

bcmd::Result<std::string> GrpcServerGateway::createChannel(std::string_view client_id,
                                                           std::string_view channel_name) {
    ::grpc::ClientContext context;
    bcmd::v1::CreateChannelRequest request;
    bcmd::v1::CreateChannelResponse response;
    request.set_client_id(bcmd::trim_copy(client_id));
    request.set_channel_name(bcmd::trim_copy(channel_name));

    const auto status = stub_->CreateChannel(&context, request, &response);
    if (!status.ok()) {
        return std::unexpected(grpc_status_to_error(status));
    }
    return response.channel_id();
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
        channels.push_back(application::port::ChannelInfo{
            .id = channel.id(), .name = channel.name(), .member_count = channel.member_count()});
    }
    return channels;
}

bcmd::VoidResult GrpcServerGateway::joinChannel(std::string_view client_id,
                                                std::string_view channel_id) {
    ::grpc::ClientContext context;
    bcmd::v1::JoinChannelRequest request;
    bcmd::v1::JoinChannelResponse response;
    request.set_client_id(bcmd::trim_copy(client_id));
    request.set_channel_id(bcmd::trim_copy(channel_id));

    const auto status = stub_->JoinChannel(&context, request, &response);
    if (!status.ok()) {
        return std::unexpected(grpc_status_to_error(status));
    }
    return {};
}

bcmd::Result<std::string> GrpcServerGateway::joinChannelByName(std::string_view client_id,
                                                               std::string_view channel_name) {
    const auto trimmed_name = bcmd::trim(channel_name);

    const auto channels = listChannels();
    if (!channels.has_value()) {
        return std::unexpected(channels.error());
    }

    for (const auto& channel : *channels) {
        if (channel.name == trimmed_name) {
            const auto joined = joinChannel(client_id, channel.id);
            if (!joined.has_value()) {
                return std::unexpected(joined.error());
            }
            return channel.id;
        }
    }

    return std::unexpected(bcmd::Error::ChannelNotFound);
}

bcmd::VoidResult GrpcServerGateway::leaveChannel(std::string_view client_id,
                                                 std::string_view channel_id) {
    ::grpc::ClientContext context;
    bcmd::v1::LeaveChannelRequest request;
    bcmd::v1::LeaveChannelResponse response;
    request.set_client_id(bcmd::trim_copy(client_id));
    request.set_channel_id(bcmd::trim_copy(channel_id));

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
    request.set_client_id(bcmd::trim_copy(client_id));
    request.set_channel_id(bcmd::trim_copy(channel_id));
    request.set_content(bcmd::trim_copy(content));

    const auto status = stub_->SendMessage(&context, request, &response);
    if (!status.ok()) {
        return std::unexpected(grpc_status_to_error(status));
    }
    return response.message_id();
}

bcmd::VoidResult GrpcServerGateway::sendHeartbeat(std::string_view client_id) {
    ::grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    bcmd::v1::HeartbeatRequest request;
    bcmd::v1::HeartbeatResponse response;
    request.set_client_id(bcmd::trim_copy(client_id));

    const auto status = stub_->Heartbeat(&context, request, &response);
    if (!status.ok()) {
        return std::unexpected(grpc_status_to_error(status));
    }
    return {};
}

bcmd::VoidResult GrpcServerGateway::subscribeToChannel(std::string_view client_id,
                                                       std::string_view channel_id,
                                                       std::uint32_t replay_count,
                                                       MessageCallback callback) {
    ::grpc::ClientContext context;
    bcmd::v1::SubscribeRequest request;
    const auto trimmed_channel_id = bcmd::trim_copy(channel_id);
    request.set_client_id(bcmd::trim_copy(client_id));
    request.set_channel_id(trimmed_channel_id);
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
        } else if (event.has_member_left()) {
            const auto& left = event.member_left();
            // Compensate for the server's broadcastEvent ignoring channel_id (OOS bug).
            if (left.channel_id() != trimmed_channel_id) {
                continue;
            }
            domain::InboxMessage system_message;
            system_message.channel_id = left.channel_id();
            system_message.sender_name = "[system]";
            system_message.content = std::string{left.username()} + " left channel";
            system_message.sent_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                            std::chrono::system_clock::now().time_since_epoch())
                                            .count();
            system_message.is_history = false;
            callback(std::move(system_message));
        } else if (event.has_member_joined()) {
            const auto& joined = event.member_joined();
            if (joined.channel_id() != trimmed_channel_id) {
                continue;
            }
            domain::InboxMessage system_message;
            system_message.channel_id = joined.channel_id();
            system_message.sender_name = "[system]";
            system_message.content = std::string{joined.username()} + " joined channel";
            system_message.sent_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                            std::chrono::system_clock::now().time_since_epoch())
                                            .count();
            system_message.is_history = false;
            callback(std::move(system_message));
        }
    }

    const auto status = reader->Finish();
    if (!status.ok()) {
        return std::unexpected(grpc_status_to_error(status));
    }
    return {};
}

bcmd::Error GrpcServerGateway::grpc_status_to_error(const ::grpc::Status& status) {
    const std::string_view message{status.error_message()};

    switch (status.error_code()) {
        case ::grpc::StatusCode::NOT_FOUND:
            if (message.starts_with("client not found")) {
                return bcmd::Error::ClientNotFound;
            }
            if (message.starts_with("not a member")) {
                return bcmd::Error::NotAMember;
            }
            return bcmd::Error::ChannelNotFound;
        case ::grpc::StatusCode::ALREADY_EXISTS:
            if (message.starts_with("channel already exists")) {
                return bcmd::Error::ChannelAlreadyExists;
            }
            if (message.starts_with("client already exists")) {
                return bcmd::Error::ClientAlreadyExists;
            }
            return bcmd::Error::AlreadyMember;
        case ::grpc::StatusCode::INVALID_ARGUMENT:
            if (message.starts_with("invalid username")) {
                return bcmd::Error::InvalidUsername;
            }
            if (message.starts_with("message too long")) {
                return bcmd::Error::MessageTooLong;
            }
            if (message.starts_with("message is empty")) {
                return bcmd::Error::MessageEmpty;
            }
            return bcmd::Error::InvalidChannelName;
        case ::grpc::StatusCode::UNAVAILABLE:
        case ::grpc::StatusCode::UNKNOWN:
        case ::grpc::StatusCode::INTERNAL:
        default:
            return bcmd::Error::NetworkError;
    }
}

}  // namespace bcmd::client::adapter::grpc
