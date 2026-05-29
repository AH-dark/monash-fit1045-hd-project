#pragma once

#include "bcmd/server/application/port/i_channel_list_publisher.hpp"
#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>

namespace bcmd::server::application::usecase {

class CreateChannel {
public:
    CreateChannel(std::shared_ptr<port::IChannelRepository> channels,
                  std::shared_ptr<port::IClientRegistry> clients)
        : CreateChannel(std::move(channels), std::move(clients), makeNullChannelListPublisher()) {}

    CreateChannel(std::shared_ptr<port::IChannelRepository> channels,
                  std::shared_ptr<port::IClientRegistry> clients,
                  std::shared_ptr<port::IChannelListPublisher> channel_list_publisher);

    bcmd::Result<bcmd::ChannelId> execute(const bcmd::ClientId& client_id,
                                          std::string_view channel_name);

private:
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
    std::shared_ptr<port::IChannelListPublisher> channel_list_publisher_{};
};

}  // namespace bcmd::server::application::usecase
