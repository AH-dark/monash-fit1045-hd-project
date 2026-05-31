#include "bcmd/v1/broadcast.pb.h"

#include <catch2/catch_test_macros.hpp>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>

#include "integration/integration_test_utils.hpp"
#include <chrono>

TEST_CASE("channel lifecycle supports create join broadcast and leave",
          "[integration][grpc][channel-lifecycle]") {
    using namespace std::chrono_literals;
    namespace integration = bcmd::tests::integration;

    integration::TestServer server;
    auto alice = integration::make_stub(integration::make_tls_channel(server.address()));
    auto bob = integration::make_stub(integration::make_tls_channel(server.address()));

    const auto alice_id = integration::connect(*alice, "alice");
    const auto bob_id = integration::connect(*bob, "bob");
    const auto channel_id = integration::join_channel(*alice, alice_id, "lifecycle-test");
    REQUIRE(integration::join_channel(*bob, bob_id, channel_id) == channel_id);

    integration::Subscription bob_subscription{*bob, bob_id, channel_id};
    integration::send_message(*alice, alice_id, channel_id, "before leave");
    REQUIRE(bob_subscription.wait_for_events(1, 2s));
    CHECK(integration::count_messages(bob_subscription.events(), "before leave") == 1);

    ::grpc::ClientContext leave_context;
    integration::set_timeout(leave_context, 2s);
    bcmd::v1::LeaveChannelRequest leave_request;
    bcmd::v1::LeaveChannelResponse leave_response;
    leave_request.set_client_id(alice_id);
    leave_request.set_channel_id(channel_id);
    REQUIRE(alice->LeaveChannel(&leave_context, leave_request, &leave_response).ok());

    ::grpc::ClientContext send_context;
    integration::set_timeout(send_context, 2s);
    bcmd::v1::SendMessageRequest send_request;
    bcmd::v1::SendMessageResponse send_response;
    send_request.set_client_id(alice_id);
    send_request.set_channel_id(channel_id);
    send_request.set_content("after leave");
    const auto status = alice->SendMessage(&send_context, send_request, &send_response);
    CHECK_FALSE(status.ok());
    CHECK(status.error_code() == ::grpc::StatusCode::NOT_FOUND);
}
