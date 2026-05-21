#pragma once

#include "bcmd/server/adapter/grpc/client_publisher.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/application/usecase/get_recent_messages.hpp"
#include "bcmd/server/application/usecase/join_channel.hpp"
#include "bcmd/server/application/usecase/leave_channel.hpp"
#include "bcmd/server/application/usecase/list_channels.hpp"
#include "bcmd/server/application/usecase/send_message.hpp"
#include "bcmd/server/application/usecase/subscribe_to_channel.hpp"
#include "bcmd/v1/broadcast.grpc.pb.h"

#include <grpcpp/grpcpp.h>

#include <memory>

namespace bcmd::server::adapter::grpc {

class BroadcastServiceImpl final : public bcmd::v1::BroadcastService::Service {
public:
    BroadcastServiceImpl(std::shared_ptr<application::usecase::JoinChannel> join_channel,
                         std::shared_ptr<application::usecase::LeaveChannel> leave_channel,
                         std::shared_ptr<application::usecase::SendMessage> send_message,
                         std::shared_ptr<application::usecase::ListChannels> list_channels,
                         std::shared_ptr<application::usecase::GetRecentMessages> get_recent,
                         std::shared_ptr<application::usecase::SubscribeToChannel> subscribe,
                         std::shared_ptr<GrpcClientPublisher> publisher,
                         std::shared_ptr<application::port::IClientRegistry> client_registry);

    ::grpc::Status Connect(::grpc::ServerContext* context, const bcmd::v1::ConnectRequest* request,
                           bcmd::v1::ConnectResponse* response) override;
    ::grpc::Status Disconnect(::grpc::ServerContext* context,
                              const bcmd::v1::DisconnectRequest* request,
                              bcmd::v1::DisconnectResponse* response) override;
    ::grpc::Status ListChannels(::grpc::ServerContext* context,
                                const bcmd::v1::ListChannelsRequest* request,
                                bcmd::v1::ListChannelsResponse* response) override;
    ::grpc::Status JoinChannel(::grpc::ServerContext* context,
                               const bcmd::v1::JoinChannelRequest* request,
                               bcmd::v1::JoinChannelResponse* response) override;
    ::grpc::Status LeaveChannel(::grpc::ServerContext* context,
                                const bcmd::v1::LeaveChannelRequest* request,
                                bcmd::v1::LeaveChannelResponse* response) override;
    ::grpc::Status SendMessage(::grpc::ServerContext* context,
                               const bcmd::v1::SendMessageRequest* request,
                               bcmd::v1::SendMessageResponse* response) override;
    ::grpc::Status SubscribeToChannel(
        ::grpc::ServerContext* context, const bcmd::v1::SubscribeRequest* request,
        ::grpc::ServerWriter<bcmd::v1::ChannelEvent>* writer) override;

private:
    std::shared_ptr<application::usecase::JoinChannel> join_channel_;
    std::shared_ptr<application::usecase::LeaveChannel> leave_channel_;
    std::shared_ptr<application::usecase::SendMessage> send_message_;
    std::shared_ptr<application::usecase::ListChannels> list_channels_;
    std::shared_ptr<application::usecase::GetRecentMessages> get_recent_;
    std::shared_ptr<application::usecase::SubscribeToChannel> subscribe_;
    std::shared_ptr<GrpcClientPublisher> publisher_;
    std::shared_ptr<application::port::IClientRegistry> client_registry_;

    static ::grpc::Status error_to_status(bcmd::Error error);
};

}  // namespace bcmd::server::adapter::grpc
