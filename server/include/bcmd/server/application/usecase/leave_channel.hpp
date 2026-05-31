#pragma once

#include "bcmd/server/application/port/i_channel_list_publisher.hpp"
#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <utility>

namespace bcmd::server::application::usecase {

class LeaveChannel {
public:
    LeaveChannel(std::shared_ptr<port::IChannelRepository> channels,
                 std::shared_ptr<port::IClientRegistry> clients)
        : LeaveChannel(std::move(channels), std::move(clients), makeNullPublisher(),
                       makeNullChannelListPublisher()) {}

    LeaveChannel(std::shared_ptr<port::IChannelRepository> channels,
                 std::shared_ptr<port::IClientRegistry> clients,
                 std::shared_ptr<port::IMessagePublisher> publisher,
                 std::shared_ptr<port::IChannelListPublisher> channel_list_publisher);

    LeaveChannel(std::shared_ptr<port::IChannelRepository> channels,
                 std::shared_ptr<port::IClientRegistry> clients,
                 std::shared_ptr<port::IMessagePublisher> publisher)
        : LeaveChannel(std::move(channels), std::move(clients), std::move(publisher),
                       makeNullChannelListPublisher()) {}

    bcmd::VoidResult execute(const bcmd::ClientId& client_id, const bcmd::ChannelId& channel_id);

private:
    class NullMessagePublisher final : public port::IMessagePublisher {
    public:
        void publish(const bcmd::ClientId& recipient_id, const domain::Message& message) override {
            (void)recipient_id;
            (void)message;
        }

        void broadcastMemberJoined(const bcmd::ChannelId& channel_id,
                                   const std::unordered_set<bcmd::ClientId>& recipients,
                                   const bcmd::ClientId& client_id,
                                   const domain::Username& username) override {
            (void)channel_id;
            (void)recipients;
            (void)client_id;
            (void)username;
        }

        void broadcastMemberLeft(const bcmd::ChannelId& channel_id,
                                 const std::unordered_set<bcmd::ClientId>& recipients,
                                 const bcmd::ClientId& client_id,
                                 const domain::Username& username) override {
            (void)channel_id;
            (void)recipients;
            (void)client_id;
            (void)username;
        }

        void unregisterSubscriber(const bcmd::ClientId& client_id) override { (void)client_id; }
    };

    static std::shared_ptr<port::IMessagePublisher> makeNullPublisher() {
        static const auto publisher = std::make_shared<NullMessagePublisher>();
        return publisher;
    }

    class NullChannelListPublisher final : public port::IChannelListPublisher {
    public:
        void registerSubscriberWithSnapshot(
            const bcmd::ClientId& subscriber_id,
            ::grpc::ServerWriterInterface<bcmd::v1::ChannelListEvent>* writer) override {
            (void)subscriber_id;
            (void)writer;
        }

        void unregisterSubscriber(const bcmd::ClientId& subscriber_id) override {
            (void)subscriber_id;
        }

        void publishChannelCreated(const domain::Channel& channel) override { (void)channel; }

        void publishMemberCountChanged(const bcmd::ChannelId& channel_id,
                                       std::int32_t member_count) override {
            (void)channel_id;
            (void)member_count;
        }
    };

    static std::shared_ptr<port::IChannelListPublisher> makeNullChannelListPublisher() {
        static const auto publisher = std::make_shared<NullChannelListPublisher>();
        return publisher;
    }

    std::shared_ptr<port::IChannelRepository> channels_{};
    std::shared_ptr<port::IClientRegistry> clients_{};
    std::shared_ptr<port::IMessagePublisher> publisher_{};
    std::shared_ptr<port::IChannelListPublisher> channel_list_publisher_{};
};

}  // namespace bcmd::server::application::usecase
