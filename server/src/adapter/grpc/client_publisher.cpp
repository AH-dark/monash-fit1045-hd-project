#include "bcmd/server/adapter/grpc/client_publisher.hpp"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <shared_mutex>

namespace bcmd::server::adapter::grpc {

namespace {

std::int64_t to_epoch_ms(std::chrono::system_clock::time_point time_point) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch())
        .count();
}

}  // namespace

void GrpcClientPublisher::registerSubscriber(const bcmd::ClientId& id,
                                             ::grpc::ServerWriter<bcmd::v1::ChannelEvent>* writer) {
    const std::unique_lock lock(mutex_);
    writers_.insert_or_assign(id.value(), writer);
}

void GrpcClientPublisher::unregisterSubscriber(const bcmd::ClientId& id) {
    const std::unique_lock lock(mutex_);
    writers_.erase(id.value());
}

void GrpcClientPublisher::publish(const bcmd::ClientId& recipient_id,
                                  const domain::Message& message, bool from_replay) {
    const std::shared_lock lock(mutex_);
    const auto found = writers_.find(recipient_id.value());
    if (found != writers_.end() && found->second != nullptr) {
        found->second->Write(message_to_event(message, from_replay));
    }
}

void GrpcClientPublisher::publishReplayComplete(const bcmd::ClientId& recipient_id,
                                                const bcmd::ChannelId& channel_id) {
    const std::shared_lock lock(mutex_);
    const auto found = writers_.find(recipient_id.value());
    if (found == writers_.end() || found->second == nullptr) {
        return;
    }

    bcmd::v1::ChannelEvent event;
    auto* replay_complete = event.mutable_replay_complete();
    replay_complete->set_channel_id(channel_id.value());
    replay_complete->set_replayed_count(0);
    found->second->Write(event);
}

void GrpcClientPublisher::broadcastEvent(const bcmd::ChannelId& /*channel_id*/,
                                         const bcmd::v1::ChannelEvent& event) {
    const std::shared_lock lock(mutex_);
    for (const auto& [_, writer] : writers_) {
        if (writer != nullptr) {
            writer->Write(event);
        }
    }
}

bcmd::v1::ChannelEvent GrpcClientPublisher::message_to_event(const domain::Message& message,
                                                             bool from_replay) {
    bcmd::v1::ChannelEvent event;
    auto* message_event = event.mutable_message();
    message_event->set_message_id(message.id().value());
    message_event->set_channel_id(message.channelId().value());
    message_event->set_sender_id(message.senderId().value());
    message_event->set_content(message.content().value());
    message_event->set_sent_at_ms(to_epoch_ms(message.sentAt()));
    message_event->set_from_replay(from_replay);
    return event;
}

}  // namespace bcmd::server::adapter::grpc
