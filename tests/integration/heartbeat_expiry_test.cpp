#include "bcmd/v1/broadcast.pb.h"

#include <catch2/catch_test_macros.hpp>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>

#include "integration/integration_test_utils.hpp"
#include <__chrono/duration.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

namespace {

bool has_member_left(const std::vector<bcmd::v1::ChannelEvent>& events,
                     const std::string& client_id) {
    return std::ranges::any_of(events, [&](const auto& event) {
        return event.has_member_left() && event.member_left().client_id() == client_id;
    });
}

}  // namespace

TEST_CASE("a stale client is reaped and the survivor receives MemberLeftEvent",
          "[integration][grpc][heartbeat][expiry]") {
    using namespace std::chrono_literals;
    namespace integration = bcmd::tests::integration;

    // Aggressive timeouts so the test completes well under the 10s SLO.
    integration::TestServer server{/*insecure=*/false,
                                   /*heartbeat_timeout=*/std::chrono::seconds{2},
                                   /*heartbeat_sweep_interval=*/std::chrono::seconds{1}};

    auto alice = integration::make_stub(integration::make_tls_channel(server.address()));
    auto bob = integration::make_stub(integration::make_tls_channel(server.address()));

    const auto alice_id = integration::connect(*alice, "alice");
    const auto bob_id = integration::connect(*bob, "bob");
    const auto channel_id = integration::join_channel(*alice, alice_id, "qa");
    REQUIRE(integration::join_channel(*bob, bob_id, channel_id) == channel_id);

    integration::Subscription bob_subscription{*bob, bob_id, channel_id, 0};

    std::atomic<bool> heartbeating{true};
    std::thread bob_heartbeater([&] {
        while (heartbeating.load(std::memory_order_relaxed)) {
            (void)integration::send_heartbeat(*bob, bob_id);
            std::this_thread::sleep_for(500ms);
        }
    });

    const bool received_member_left = bob_subscription.wait_for(
        5s, +[](const std::vector<bcmd::v1::ChannelEvent>& events) {
            return std::ranges::any_of(events,
                                       [](const auto& event) { return event.has_member_left(); });
        });

    heartbeating.store(false, std::memory_order_relaxed);
    if (bob_heartbeater.joinable()) {
        bob_heartbeater.join();
    }

    REQUIRE(received_member_left);
    const auto events = bob_subscription.events();
    CHECK(has_member_left(events, alice_id));

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
