#pragma once

#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/v1/broadcast.grpc.pb.h"

#include <grpcpp/grpcpp.h>

#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace bcmd::server::adapter::grpc {

class GrpcClientPublisher final : public application::port::IMessagePublisher {
public:
    void registerSubscriber(const bcmd::ClientId& id,
                            ::grpc::ServerWriter<bcmd::v1::ChannelEvent>* writer);
    void unregisterSubscriber(const bcmd::ClientId& id);

    void publish(const bcmd::ClientId& recipient_id, const domain::Message& message,
                 bool from_replay = false) override;

    void publishReplayComplete(const bcmd::ClientId& recipient_id,
                               const bcmd::ChannelId& channel_id) override;

    void broadcastEvent(const bcmd::ChannelId& channel_id, const bcmd::v1::ChannelEvent& event);

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, ::grpc::ServerWriter<bcmd::v1::ChannelEvent>*> writers_;

    static bcmd::v1::ChannelEvent message_to_event(const domain::Message& message,
                                                   bool from_replay = false);
};

}  // namespace bcmd::server::adapter::grpc
