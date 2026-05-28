#pragma once

#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"

#include <cstddef>
#include <string>
#include <unordered_set>
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
        std::unordered_set<bcmd::ClientId> recipients;
        bcmd::ClientId client_id;
        std::string username;
    };

    using MemberJoinedRecord = MemberLeftRecord;

    void publish(const bcmd::ClientId& recipient_id, const bcmd::server::domain::Message& message,
                 bool from_replay = false) override {
        deliveries.push_back(
            Delivery{.recipient = recipient_id, .message = message, .from_replay = from_replay});
    }

    void publishReplayComplete(const bcmd::ClientId& recipient_id,
                               const bcmd::ChannelId& channel_id) override {
        replay_markers.push_back(ReplayMarker{.recipient = recipient_id, .channel = channel_id});
    }

    void broadcastMemberJoined(const bcmd::ChannelId& channel_id,
                               const std::unordered_set<bcmd::ClientId>& recipients,
                               const bcmd::ClientId& client_id,
                               const bcmd::server::domain::Username& username) override {
        joined_broadcasts_.push_back(MemberJoinedRecord{
            .channel_id = channel_id,
            .recipients = recipients,
            .client_id = client_id,
            .username = username.value(),
        });
    }

    void broadcastMemberLeft(const bcmd::ChannelId& channel_id,
                             const std::unordered_set<bcmd::ClientId>& recipients,
                             const bcmd::ClientId& client_id,
                             const bcmd::server::domain::Username& username) override {
        broadcasts_.push_back(MemberLeftRecord{
            .channel_id = channel_id,
            .recipients = recipients,
            .client_id = client_id,
            .username = username.value(),
        });
    }

    void unregisterSubscriber(const bcmd::ClientId& client_id) override {
        unregister_calls_.push_back(client_id);
    }

    [[nodiscard]] std::size_t countFor(const bcmd::ClientId& recipient_id) const {
        std::size_t total = 0;
        for (const auto& delivery : deliveries) {
            if (delivery.recipient == recipient_id) {
                ++total;
            }
        }
        return total;
    }

    [[nodiscard]] const std::vector<MemberLeftRecord>& broadcasts() const { return broadcasts_; }

    [[nodiscard]] const std::vector<MemberJoinedRecord>& joinedBroadcasts() const {
        return joined_broadcasts_;
    }

    [[nodiscard]] const std::vector<bcmd::ClientId>& unregisterCalls() const {
        return unregister_calls_;
    }

    std::vector<Delivery>
        deliveries;  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::vector<ReplayMarker>
        replay_markers;  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    std::vector<MemberJoinedRecord> joined_broadcasts_;
    std::vector<MemberLeftRecord> broadcasts_;
    std::vector<bcmd::ClientId> unregister_calls_;
};

}  // namespace bcmd::tests
