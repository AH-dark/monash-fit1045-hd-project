#pragma once

#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/v1/broadcast.pb.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/support/sync_stream.h>

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace bcmd::server::adapter::grpc {

class GrpcClientPublisher final : public application::port::IMessagePublisher {
public:
    explicit GrpcClientPublisher(
        std::shared_ptr<application::port::IClientRegistry> client_registry = nullptr);

    void registerSubscriber(const bcmd::ClientId& id,
                            ::grpc::ServerWriterInterface<bcmd::v1::ChannelEvent>* writer);
    void unregisterSubscriber(const bcmd::ClientId& id) override;

    void publish(const bcmd::ClientId& recipient_id, const domain::Message& message,
                 bool from_replay = false) override;

    void publishReplayComplete(const bcmd::ClientId& recipient_id,
                               const bcmd::ChannelId& channel_id) override;

    void broadcastMemberJoined(const bcmd::ChannelId& channel_id,
                               const std::unordered_set<bcmd::ClientId>& recipients,
                               const bcmd::ClientId& client_id,
                               const domain::Username& username) override;

    void broadcastMemberLeft(const bcmd::ChannelId& channel_id,
                             const std::unordered_set<bcmd::ClientId>& recipients,
                             const bcmd::ClientId& client_id,
                             const domain::Username& username) override;

    void broadcastEvent(const bcmd::ChannelId& channel_id, const bcmd::v1::ChannelEvent& event);

private:
    struct WriterEntry {
        ::grpc::ServerWriterInterface<bcmd::v1::ChannelEvent>* writer{nullptr};
        std::mutex write_mutex;
    };

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<WriterEntry>> writers_;
    std::shared_ptr<application::port::IClientRegistry> client_registry_{};

    bcmd::v1::ChannelEvent message_to_event(const domain::Message& message,
                                            bool from_replay = false) const;
};

}  // namespace bcmd::server::adapter::grpc
