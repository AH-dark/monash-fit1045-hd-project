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
#include <string_view>
#include <unordered_set>
#include <utility>

namespace bcmd::server::application::usecase {

class JoinChannel {
public:
    JoinChannel(std::shared_ptr<port::IChannelRepository> channels,
                std::shared_ptr<port::IClientRegistry> clients)
        : JoinChannel(std::move(channels), std::move(clients), makeNullMessagePublisher(),
                      makeNullChannelListPublisher()) {}

    JoinChannel(std::shared_ptr<port::IChannelRepository> channels,
                std::shared_ptr<port::IClientRegistry> clients,
                std::shared_ptr<port::IMessagePublisher> message_publisher,
                std::shared_ptr<port::IChannelListPublisher> channel_list_publisher);

    bcmd::VoidResult execute(const bcmd::ClientId& client_id, const bcmd::ChannelId& channel_id);

    // Joins an existing channel by name. Returns Error::ChannelNotFound when the
    // channel does not exist; use CreateChannel to create one first.
    bcmd::Result<bcmd::ChannelId> executeByName(const bcmd::ClientId& client_id,
                                                std::string_view channel_name);

private:
    // Removes `session` from every channel except `target_channel_id`, broadcasting
    // a member-left event and emitting a member-count change for each. Tolerates
    // `NotAMember`/`ChannelNotFound` from racing leaves/deletions. Uses
    // `leaveChannelAtomic` to mutate the session in place; no longer requires the
    // caller to `save` the session afterwards.
    bcmd::VoidResult leaveOtherChannels(const domain::ClientSession& session,
                                        const bcmd::ChannelId& target_channel_id);

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

    static std::shared_ptr<port::IMessagePublisher> makeNullMessagePublisher() {
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
    std::shared_ptr<port::IMessagePublisher> message_publisher_{};
    std::shared_ptr<port::IChannelListPublisher> channel_list_publisher_{};
};

}  // namespace bcmd::server::application::usecase
