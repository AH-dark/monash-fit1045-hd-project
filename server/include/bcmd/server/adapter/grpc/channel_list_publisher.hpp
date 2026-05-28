#pragma once

#include "bcmd/server/application/port/i_channel_list_publisher.hpp"
#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/v1/broadcast.pb.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/support/sync_stream.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace bcmd::server::adapter::grpc {

class GrpcChannelListPublisher final : public application::port::IChannelListPublisher {
public:
    explicit GrpcChannelListPublisher(
        std::shared_ptr<application::port::IChannelRepository> channels_repo);

    void registerSubscriberWithSnapshot(
        const bcmd::ClientId& subscriber_id,
        ::grpc::ServerWriterInterface<bcmd::v1::ChannelListEvent>* writer) override;

    void unregisterSubscriber(const bcmd::ClientId& subscriber_id) override;

    void publishChannelCreated(const bcmd::server::domain::Channel& channel) override;

    void publishMemberCountChanged(const bcmd::ChannelId& channel_id,
                                   std::int32_t member_count) override;

private:
    struct WriterEntry {
        ::grpc::ServerWriterInterface<bcmd::v1::ChannelListEvent>* writer{nullptr};
        std::mutex write_mutex;
    };

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<WriterEntry>> writers_;
    std::shared_ptr<application::port::IChannelRepository> channels_;
};

}  // namespace bcmd::server::adapter::grpc
