#include "bcmd/v1/broadcast.pb.h"

#include <catch2/catch_test_macros.hpp>

#include "integration/integration_test_utils.hpp"
#include <chrono>

TEST_CASE("client can resubscribe after dropping a stream", "[integration][grpc][reconnect]") {
    using namespace std::chrono_literals;
    namespace integration = bcmd::tests::integration;

    integration::TestServer server;
    auto alice = integration::make_stub(integration::make_tls_channel(server.address()));
    auto bob = integration::make_stub(integration::make_tls_channel(server.address()));

    const auto alice_id = integration::connect(*alice, "alice");
    const auto bob_id = integration::connect(*bob, "bob");
    const auto channel_id = integration::join_channel(*alice, alice_id, "reconnect-test");
    REQUIRE(integration::join_channel(*bob, bob_id, channel_id) == channel_id);

    {
        integration::Subscription first_subscription{*alice, alice_id, channel_id};
        integration::send_message(*bob, bob_id, channel_id, "before drop");
        REQUIRE(first_subscription.wait_for_events(1, 2s));
        first_subscription.stop();
    }
    std::this_thread::sleep_for(200ms);

    integration::Subscription second_subscription{*alice, alice_id, channel_id};
    integration::send_message(*bob, bob_id, channel_id, "after resubscribe");
    REQUIRE(second_subscription.wait_for_events(1, 2s));
    CHECK(integration::count_messages(second_subscription.events(), "after resubscribe") == 1);
}
