#pragma once

#include "bcmd/client/application/port/i_server_gateway.hpp"
#include "bcmd/shared/result.hpp"
#include "bcmd/v1/broadcast.grpc.pb.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace bcmd::client::adapter::grpc {

class GrpcServerGateway final : public application::port::IServerGateway {
public:
    explicit GrpcServerGateway(std::shared_ptr<::grpc::Channel> channel);

    bcmd::Result<std::string> connect(std::string_view username) override;
    bcmd::VoidResult disconnect(std::string_view client_id) override;
    bcmd::Result<std::vector<application::port::ChannelInfo>> listChannels() override;
    bcmd::Result<std::string> createChannel(std::string_view client_id,
                                            std::string_view channel_name) override;
    bcmd::VoidResult joinChannel(std::string_view client_id, std::string_view channel_id) override;
    bcmd::Result<std::string> joinChannelByName(std::string_view client_id,
                                                std::string_view channel_name) override;
    bcmd::VoidResult leaveChannel(std::string_view client_id, std::string_view channel_id) override;
    bcmd::Result<std::string> sendMessage(std::string_view client_id, std::string_view channel_id,
                                          std::string_view content) override;
    bcmd::VoidResult subscribeToChannel(std::string_view client_id, std::string_view channel_id,
                                        std::uint32_t replay_count,
                                        MessageCallback callback) override;

private:
    std::unique_ptr<bcmd::v1::BroadcastService::Stub> stub_{};

    static bcmd::Error grpc_status_to_error(const ::grpc::Status& status);
};

}  // namespace bcmd::client::adapter::grpc
