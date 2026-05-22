#include "bcmd/v1/broadcast.pb.h"

#include <catch2/catch_test_macros.hpp>

#include "integration/integration_test_utils.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace {

bool has_replay_complete(const std::vector<bcmd::v1::ChannelEvent>& events) {
    return std::ranges::any_of(events,
                               [](const auto& event) { return event.has_replay_complete(); });
}

}  // namespace

TEST_CASE("subscriber receives requested history then switches to live messages",
          "[integration][grpc][history]") {
    using namespace std::chrono_literals;
    namespace integration = bcmd::tests::integration;

    integration::TestServer server;
    auto alice = integration::make_stub(integration::make_tls_channel(server.address()));
    auto bob = integration::make_stub(integration::make_tls_channel(server.address()));

    const auto alice_id = integration::connect(*alice, "alice");
    const auto channel_id = integration::join_channel(*alice, alice_id, "replay-test");
    for (std::size_t index = 0; index < 12; ++index) {
        integration::send_message(*alice, alice_id, channel_id, "msg-" + std::to_string(index));
    }

    const auto bob_id = integration::connect(*bob, "bob");
    REQUIRE(integration::join_channel(*bob, bob_id, channel_id) == channel_id);
    integration::Subscription bob_subscription{*bob, bob_id, channel_id, 10};
    REQUIRE(bob_subscription.wait_for(2s, has_replay_complete));

    auto events = bob_subscription.events();
    std::size_t replayed_count{0};
    for (const auto& event : events) {
        if (event.has_message() && event.message().from_replay()) {
            ++replayed_count;
        }
    }
    CHECK(replayed_count == 10);

    integration::send_message(*alice, alice_id, channel_id, "live-after-replay");
    REQUIRE(bob_subscription.wait_for_events(events.size() + 1, 2s));
    events = bob_subscription.events();

    bool saw_live_message{false};
    for (const auto& event : events) {
        if (event.has_message() && event.message().content() == "live-after-replay") {
            CHECK_FALSE(event.message().from_replay());
            saw_live_message = true;
        }
    }
    CHECK(saw_live_message);
}
