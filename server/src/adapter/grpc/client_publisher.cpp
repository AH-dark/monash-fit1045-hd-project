#include "bcmd/server/adapter/grpc/client_publisher.hpp"

#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/v1/broadcast.pb.h"

#include <grpcpp/support/sync_stream.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_set>
#include <utility>
#include <vector>

namespace bcmd::server::adapter::grpc {

namespace {

std::int64_t to_epoch_ms(std::chrono::system_clock::time_point time_point) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch())
        .count();
}

}  // namespace

GrpcClientPublisher::GrpcClientPublisher(
    std::shared_ptr<application::port::IClientRegistry> client_registry)
    : client_registry_(std::move(client_registry)) {}

void GrpcClientPublisher::registerSubscriber(
    const bcmd::ClientId& id, ::grpc::ServerWriterInterface<bcmd::v1::ChannelEvent>* writer) {
    auto entry = std::make_shared<GrpcClientPublisher::WriterEntry>();
    entry->writer = writer;

    const std::unique_lock lock(mutex_);
    writers_.insert_or_assign(id.value(), std::move(entry));
}

void GrpcClientPublisher::unregisterSubscriber(const bcmd::ClientId& id) {
    std::shared_ptr<GrpcClientPublisher::WriterEntry> entry;
    {
        const std::unique_lock lock(mutex_);
        const auto found = writers_.find(id.value());
        if (found == writers_.end()) {
            return;
        }
        entry = std::move(found->second);
        writers_.erase(found);
    }
    // Wait for any in-flight Write() on this writer to complete, then disarm the
    // entry so callers that still hold a shared_ptr observe a null writer and skip.
    if (entry) {
        const std::scoped_lock write_lock(entry->write_mutex);
        entry->writer = nullptr;
    }
}

void GrpcClientPublisher::publish(const bcmd::ClientId& recipient_id,
                                  const domain::Message& message, bool from_replay) {
    std::shared_ptr<GrpcClientPublisher::WriterEntry> entry;
    {
        const std::shared_lock lock(mutex_);
        const auto found = writers_.find(recipient_id.value());
        if (found == writers_.end()) {
            return;
        }
        entry = found->second;
    }
    if (!entry) {
        return;
    }
    const std::scoped_lock write_lock(entry->write_mutex);
    if (entry->writer != nullptr) {
        entry->writer->Write(message_to_event(message, from_replay));
    }
}

void GrpcClientPublisher::publishReplayComplete(const bcmd::ClientId& recipient_id,
                                                const bcmd::ChannelId& channel_id) {
    std::shared_ptr<GrpcClientPublisher::WriterEntry> entry;
    {
        const std::shared_lock lock(mutex_);
        const auto found = writers_.find(recipient_id.value());
        if (found == writers_.end()) {
            return;
        }
        entry = found->second;
    }
    if (!entry) {
        return;
    }

    bcmd::v1::ChannelEvent event;
    auto* replay_complete = event.mutable_replay_complete();
    replay_complete->set_channel_id(channel_id.value());
    replay_complete->set_replayed_count(0);

    const std::scoped_lock write_lock(entry->write_mutex);
    if (entry->writer != nullptr) {
        entry->writer->Write(event);
    }
}

void GrpcClientPublisher::broadcastMemberLeft(const bcmd::ChannelId& channel_id,
                                              const std::unordered_set<bcmd::ClientId>& recipients,
                                              const bcmd::ClientId& client_id,
                                              const domain::Username& username) {
    bcmd::v1::ChannelEvent event;
    auto* member_left = event.mutable_member_left();
    member_left->set_channel_id(channel_id.value());
    member_left->set_client_id(client_id.value());
    member_left->set_username(username.value());

    std::vector<std::shared_ptr<GrpcClientPublisher::WriterEntry>> targets;
    {
        const std::shared_lock lock(mutex_);
        targets.reserve(writers_.size());
        for (const auto& [client_id_str, entry] : writers_) {
            const auto recipient_id = bcmd::ClientId::parse(client_id_str);
            if (entry && recipient_id.has_value() && recipients.contains(*recipient_id)) {
                targets.push_back(entry);
            }
        }
    }
    for (const auto& entry : targets) {
        const std::scoped_lock write_lock(entry->write_mutex);
        if (entry->writer != nullptr) {
            entry->writer->Write(event);
        }
    }
}

void GrpcClientPublisher::broadcastEvent(const bcmd::ChannelId& /*channel_id*/,
                                         const bcmd::v1::ChannelEvent& event) {
    std::vector<std::shared_ptr<GrpcClientPublisher::WriterEntry>> targets;
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
        const std::scoped_lock write_lock(entry->write_mutex);
        if (entry->writer != nullptr) {
            entry->writer->Write(event);
        }
    }
}

bcmd::v1::ChannelEvent GrpcClientPublisher::message_to_event(const domain::Message& message,
                                                             bool from_replay) const {
    bcmd::v1::ChannelEvent event;
    auto* message_event = event.mutable_message();
    message_event->set_message_id(message.id().value());
    message_event->set_channel_id(message.channelId().value());
    message_event->set_sender_id(message.senderId().value());
    message_event->set_content(message.content().value());
    message_event->set_sent_at_ms(to_epoch_ms(message.sentAt()));
    message_event->set_from_replay(from_replay);

    if (client_registry_) {
        if (auto session = client_registry_->findById(message.senderId()); session.has_value()) {
            message_event->set_sender_name(session->username().value());
        }
    }

    return event;
}

}  // namespace bcmd::server::adapter::grpc
