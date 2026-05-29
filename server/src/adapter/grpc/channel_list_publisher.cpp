#include "bcmd/server/adapter/grpc/channel_list_publisher.hpp"

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/v1/broadcast.pb.h"

#include <grpcpp/support/sync_stream.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <vector>

namespace bcmd::server::adapter::grpc {

namespace {

void populate_channel_summary(bcmd::v1::ChannelSummary* summary, const domain::Channel& channel) {
    summary->set_id(channel.id().to_string());
    summary->set_name(channel.name().value());
    summary->set_member_count(static_cast<std::int32_t>(channel.memberCount()));
}

}  // namespace

GrpcChannelListPublisher::GrpcChannelListPublisher(
    std::shared_ptr<application::port::IChannelRepository> channels_repo)
    : channels_(std::move(channels_repo)) {}

void GrpcChannelListPublisher::registerSubscriberWithSnapshot(
    const bcmd::ClientId& subscriber_id,
    ::grpc::ServerWriterInterface<bcmd::v1::ChannelListEvent>* writer) {
    auto entry = std::make_shared<GrpcChannelListPublisher::WriterEntry>();
    entry->writer = writer;

    const std::unique_lock lock(mutex_);
    writers_.insert_or_assign(subscriber_id.to_string(), entry);

    bcmd::v1::ChannelListEvent event;
    auto* snapshot = event.mutable_snapshot();
    for (const auto& channel : channels_->listAll()) {
        populate_channel_summary(snapshot->add_channels(), channel);
    }

    const std::scoped_lock write_lock(entry->write_mutex);
    if (entry->writer != nullptr) {
        entry->writer->Write(event);
    }
}

void GrpcChannelListPublisher::unregisterSubscriber(const bcmd::ClientId& subscriber_id) {
    std::shared_ptr<GrpcChannelListPublisher::WriterEntry> entry;
    {
        const std::unique_lock lock(mutex_);
        const auto found = writers_.find(subscriber_id.to_string());
        if (found == writers_.end()) {
            return;
        }
        entry = std::move(found->second);
        writers_.erase(found);
    }

    if (entry) {
        const std::scoped_lock write_lock(entry->write_mutex);
        entry->writer = nullptr;
    }
}

void GrpcChannelListPublisher::publishChannelCreated(const domain::Channel& channel) {
    std::vector<std::shared_ptr<GrpcChannelListPublisher::WriterEntry>> targets;
    {
        const std::shared_lock lock(mutex_);
        targets.reserve(writers_.size());
        for (const auto& [_, entry] : writers_) {
            if (entry) {
                targets.push_back(entry);
            }
        }
    }

    for (const auto& entry : targets) {
        bcmd::v1::ChannelListEvent event;
        auto* created = event.mutable_created();
        populate_channel_summary(created->mutable_channel(), channel);

        const std::scoped_lock write_lock(entry->write_mutex);
        if (entry->writer != nullptr) {
            entry->writer->Write(event);
        }
    }
}

void GrpcChannelListPublisher::publishMemberCountChanged(const bcmd::ChannelId& channel_id,
                                                         std::int32_t member_count) {
    std::vector<std::shared_ptr<GrpcChannelListPublisher::WriterEntry>> targets;
    {
        const std::shared_lock lock(mutex_);
        targets.reserve(writers_.size());
        for (const auto& [_, entry] : writers_) {
            if (entry) {
                targets.push_back(entry);
            }
        }
    }

    for (const auto& entry : targets) {
        bcmd::v1::ChannelListEvent event;
        auto* changed = event.mutable_member_count_changed();
        changed->set_channel_id(channel_id.to_string());
        changed->set_member_count(member_count);

        const std::scoped_lock write_lock(entry->write_mutex);
        if (entry->writer != nullptr) {
            entry->writer->Write(event);
        }
    }
}

}  // namespace bcmd::server::adapter::grpc
