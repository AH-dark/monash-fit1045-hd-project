#pragma once

#include <cstddef>
#include <vector>

#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/shared/ids.hpp"

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

    void publish(const bcmd::ClientId& recipient_id,
                 const bcmd::server::domain::Message& message,
                 bool from_replay = false) override {
        deliveries.push_back(Delivery{recipient_id, message, from_replay});
    }

    void publishReplayComplete(const bcmd::ClientId& recipient_id,
                               const bcmd::ChannelId& channel_id) override {
        replay_markers.push_back(ReplayMarker{recipient_id, channel_id});
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

    std::vector<Delivery> deliveries;
    std::vector<ReplayMarker> replay_markers;
};

}  // namespace bcmd::tests
