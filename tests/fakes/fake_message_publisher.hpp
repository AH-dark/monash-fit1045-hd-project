#pragma once

#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace bcmd::tests {

class FakeMessagePublisher final : public bcmd::server::application::port::IMessagePublisher {
public:
    struct Delivery {
        bcmd::ClientId recipient;
        bcmd::server::domain::Message message;
        bool from_replay;
    };

    struct ReplayMarker {
        bcmd::ClientId recipient;
        bcmd::ChannelId channel;
    };

    struct MemberLeftRecord {
        bcmd::ChannelId channel_id;
        bcmd::ClientId client_id;
        std::string username;
    };

    void publish(const bcmd::ClientId& recipient_id, const bcmd::server::domain::Message& message,
                 bool from_replay = false) override {
        deliveries.push_back(
            Delivery{.recipient = recipient_id, .message = message, .from_replay = from_replay});
    }

    void publishReplayComplete(const bcmd::ClientId& recipient_id,
                               const bcmd::ChannelId& channel_id) override {
        replay_markers.push_back(ReplayMarker{recipient_id, channel_id});
    }

    void broadcastMemberLeft(const bcmd::ChannelId& channel_id, const bcmd::ClientId& client_id,
                             const bcmd::server::domain::Username& username) override {
        broadcasts_.push_back(MemberLeftRecord{
            .channel_id = channel_id,
            .client_id = client_id,
            .username = username.value(),
        });
    }

    void unregisterSubscriber(const bcmd::ClientId& client_id) override {
        unregister_calls_.push_back(client_id);
    }

    std::size_t countFor(const bcmd::ClientId& recipient_id) const {
        std::size_t total = 0;
        for (const auto& delivery : deliveries) {
            if (delivery.recipient == recipient_id) {
                ++total;
            }
        }
        return total;
    }

    const std::vector<MemberLeftRecord>& broadcasts() const { return broadcasts_; }

    const std::vector<bcmd::ClientId>& unregisterCalls() const { return unregister_calls_; }

    std::vector<Delivery> deliveries;
    std::vector<ReplayMarker> replay_markers;

private:
    std::vector<MemberLeftRecord> broadcasts_;
    std::vector<bcmd::ClientId> unregister_calls_;
};

}  // namespace bcmd::tests
