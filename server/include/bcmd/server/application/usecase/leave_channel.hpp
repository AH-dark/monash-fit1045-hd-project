#pragma once

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <memory>
#include <unordered_set>
#include <utility>

namespace bcmd::server::application::usecase {

class LeaveChannel {
public:
    LeaveChannel(std::shared_ptr<port::IChannelRepository> channels,
                 std::shared_ptr<port::IClientRegistry> clients)
        : LeaveChannel(std::move(channels), std::move(clients), makeNullPublisher()) {}

    LeaveChannel(std::shared_ptr<port::IChannelRepository> channels,
                 std::shared_ptr<port::IClientRegistry> clients,
                 std::shared_ptr<port::IMessagePublisher> publisher);

    bcmd::VoidResult execute(const bcmd::ClientId& client_id, const bcmd::ChannelId& channel_id);

private:
    class NullMessagePublisher final : public port::IMessagePublisher {
    public:
        void publish(const bcmd::ClientId&, const domain::Message&, bool) override {}

        void publishReplayComplete(const bcmd::ClientId&, const bcmd::ChannelId&) override {}

        void broadcastMemberLeft(const bcmd::ChannelId&, const std::unordered_set<bcmd::ClientId>&,
                                 const bcmd::ClientId&, const domain::Username&) override {}

        void unregisterSubscriber(const bcmd::ClientId&) override {}
    };

    static std::shared_ptr<port::IMessagePublisher> makeNullPublisher() {
        static const auto publisher = std::make_shared<NullMessagePublisher>();
        return publisher;
    }

    std::shared_ptr<port::IChannelRepository> channels_{};
    std::shared_ptr<port::IClientRegistry> clients_{};
    std::shared_ptr<port::IMessagePublisher> publisher_{};
};

}  // namespace bcmd::server::application::usecase
