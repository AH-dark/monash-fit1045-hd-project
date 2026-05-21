#include <catch2/catch_test_macros.hpp>

#include <chrono>

#include "integration/integration_test_utils.hpp"

TEST_CASE("TLS gRPC server broadcasts a live message between two clients",
          "[integration][grpc][round-trip]") {
    using namespace std::chrono_literals;
    namespace integration = bcmd::tests::integration;

    integration::TestServer server;
    auto alice = integration::make_stub(integration::make_tls_channel(server.address()));
    auto bob = integration::make_stub(integration::make_tls_channel(server.address()));

    const auto alice_id = integration::connect(*alice, "alice");
    const auto bob_id = integration::connect(*bob, "bob");
    const auto channel_id = integration::join_channel(*alice, alice_id, "test-channel");
    REQUIRE(integration::join_channel(*bob, bob_id, channel_id) == channel_id);

    integration::Subscription alice_subscription{*alice, alice_id, channel_id, 0};
    integration::Subscription bob_subscription{*bob, bob_id, channel_id, 0};
    REQUIRE(alice_subscription.wait_for_events(1, 1s));
    REQUIRE(bob_subscription.wait_for_events(1, 1s));

    integration::send_message(*alice, alice_id, channel_id, "hello bob");

    REQUIRE(bob_subscription.wait_for_events(2, 2s));
    const auto events = bob_subscription.events();
    CHECK(integration::count_messages(events, "hello bob") == 1);
}
