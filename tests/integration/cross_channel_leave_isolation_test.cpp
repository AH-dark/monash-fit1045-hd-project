#include "bcmd/v1/broadcast.pb.h"

#include <catch2/catch_test_macros.hpp>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>

#include "integration/integration_test_utils.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace {

bool has_any_member_left(const std::vector<bcmd::v1::ChannelEvent>& events) {
    return std::ranges::any_of(events, [](const auto& event) { return event.has_member_left(); });
}

std::size_t count_member_left(const std::vector<bcmd::v1::ChannelEvent>& events,
                              const std::string& channel_id, const std::string& client_id) {
    std::size_t count{0};
    for (const auto& event : events) {
        if (event.has_member_left() && event.member_left().channel_id() == channel_id &&
            event.member_left().client_id() == client_id) {
            ++count;
        }
    }
    return count;
}

}  // namespace

TEST_CASE(
    "cross_channel_leave_isolation: LeaveChannel does not broadcast MemberLeft to "
    "subscribers of other channels",
    "[integration][grpc][leave][member-left][isolation]") {
    using namespace std::chrono_literals;
    namespace integration = bcmd::tests::integration;

    integration::TestServer server;
    auto alice = integration::make_stub(integration::make_tls_channel(server.address()));
    auto bob = integration::make_stub(integration::make_tls_channel(server.address()));
    auto carol = integration::make_stub(integration::make_tls_channel(server.address()));

    const auto alice_id = integration::connect(*alice, "alice");
    const auto bob_id = integration::connect(*bob, "bob");
    const auto carol_id = integration::connect(*carol, "carol");

    const auto chan_a_id = integration::join_channel(*alice, alice_id, "chan-a");
    REQUIRE(integration::join_channel(*bob, bob_id, chan_a_id) == chan_a_id);
    const auto chan_b_id = integration::join_channel(*carol, carol_id, "chan-b");

    integration::Subscription bob_subscription{*bob, bob_id, chan_a_id, 0};
    REQUIRE(bob_subscription.wait_for_events(1, 1s));
    integration::Subscription carol_subscription{*carol, carol_id, chan_b_id, 0};
    REQUIRE(carol_subscription.wait_for_events(1, 1s));

    ::grpc::ClientContext leave_context;
    integration::set_timeout(leave_context, 2s);
    bcmd::v1::LeaveChannelRequest leave_request;
    bcmd::v1::LeaveChannelResponse leave_response;
    leave_request.set_client_id(alice_id);
    leave_request.set_channel_id(chan_a_id);
    REQUIRE(alice->LeaveChannel(&leave_context, leave_request, &leave_response).ok());

    const bool bob_received_member_left = bob_subscription.wait_for(1s, &has_any_member_left);
    REQUIRE(bob_received_member_left);
    CHECK(count_member_left(bob_subscription.events(), chan_a_id, alice_id) == 1);

    // Bounded absence check: give the server an extra window to deliver any
    // cross-channel leak; the predicate must time out for Carol.
    const bool carol_received_member_left =
        carol_subscription.wait_for(500ms, &has_any_member_left);
    REQUIRE_FALSE(carol_received_member_left);
    CHECK(count_member_left(carol_subscription.events(), chan_a_id, alice_id) == 0);
    CHECK_FALSE(has_any_member_left(carol_subscription.events()));

    bob_subscription.stop();
    carol_subscription.stop();
}
