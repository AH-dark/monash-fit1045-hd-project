#include "bcmd/v1/broadcast.pb.h"

#include <catch2/catch_test_macros.hpp>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>

#include "integration/integration_test_utils.hpp"
#include <__chrono/duration.h>
#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

namespace {

bool has_member_left(const std::vector<bcmd::v1::ChannelEvent>& events,
                     const std::string& client_id) {
    return std::ranges::any_of(events, [&](const auto& event) {
        return event.has_member_left() && event.member_left().client_id() == client_id;
    });
}

}  // namespace

TEST_CASE("Disconnect broadcasts MemberLeftEvent to every joined channel",
          "[integration][grpc][disconnect][member-left]") {
    using namespace std::chrono_literals;
    namespace integration = bcmd::tests::integration;

    integration::TestServer server;
    auto alice = integration::make_stub(integration::make_tls_channel(server.address()));
    auto bob = integration::make_stub(integration::make_tls_channel(server.address()));

    const auto alice_id = integration::connect(*alice, "alice");
    const auto bob_id = integration::connect(*bob, "bob");
    const auto channel_id = integration::join_channel(*alice, alice_id, "qa");
    REQUIRE(integration::join_channel(*bob, bob_id, channel_id) == channel_id);

    integration::Subscription bob_subscription{*bob, bob_id, channel_id, 0};
    REQUIRE(bob_subscription.wait_for_events(1, 1s));

    const auto status = integration::disconnect(*alice, alice_id);
    REQUIRE(status.ok());

    const bool received_member_left = bob_subscription.wait_for(
        1s, +[](const std::vector<bcmd::v1::ChannelEvent>& events) {
            return std::ranges::any_of(events,
                                       [](const auto& event) { return event.has_member_left(); });
        });
    REQUIRE(received_member_left);
    CHECK(has_member_left(bob_subscription.events(), alice_id));

    ::grpc::ClientContext list_context;
    integration::set_timeout(list_context, 2s);
    bcmd::v1::ListChannelsRequest list_request;
    bcmd::v1::ListChannelsResponse list_response;
    REQUIRE(bob->ListChannels(&list_context, list_request, &list_response).ok());
    bool found_channel = false;
    for (const auto& summary : list_response.channels()) {
        if (summary.id() == channel_id) {
            found_channel = true;
            CHECK(summary.member_count() == 1);
        }
    }
    CHECK(found_channel);

    const auto stale_status = integration::send_heartbeat(*alice, alice_id);
    CHECK_FALSE(stale_status.ok());
    CHECK(stale_status.error_code() == ::grpc::StatusCode::NOT_FOUND);
}
