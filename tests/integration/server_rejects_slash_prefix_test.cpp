#include "bcmd/v1/broadcast.pb.h"

#include <catch2/catch_test_macros.hpp>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>

#include "integration/integration_test_utils.hpp"
#include <chrono>
#include <string>
#include <string_view>

namespace {

::grpc::Status send_raw_content(bcmd::tests::integration::BroadcastStub& stub,
                                std::string_view client_id, std::string_view channel_id,
                                std::string_view content) {
    using namespace std::chrono_literals;
    namespace integration = bcmd::tests::integration;

    ::grpc::ClientContext context;
    integration::set_timeout(context, 2s);
    bcmd::v1::SendMessageRequest request;
    bcmd::v1::SendMessageResponse response;
    request.set_client_id(std::string{client_id});
    request.set_channel_id(std::string{channel_id});
    request.set_content(std::string{content});
    return stub.SendMessage(&context, request, &response);
}

}  // namespace

TEST_CASE(
    "server_rejects_slash_prefix: SendMessage rejects slash-prefixed content with "
    "INVALID_ARGUMENT",
    "[integration][grpc][send-message][slash-prefix]") {
    namespace integration = bcmd::tests::integration;

    integration::TestServer server;
    auto alice = integration::make_stub(integration::make_tls_channel(server.address()));

    const auto alice_id = integration::connect(*alice, "alice");
    const auto channel_id = integration::join_channel(*alice, alice_id, "chan-x");

    const std::string_view expected_message_fragment = "must not start with '/'";

    SECTION("content '/echo' is rejected") {
        const auto status = send_raw_content(*alice, alice_id, channel_id, "/echo");
        CHECK_FALSE(status.ok());
        CHECK(status.error_code() == ::grpc::StatusCode::INVALID_ARGUMENT);
        CHECK(std::string_view{status.error_message()}.contains(expected_message_fragment));
    }

    SECTION("content '  /foo' (leading whitespace before slash) is rejected") {
        const auto status = send_raw_content(*alice, alice_id, channel_id, "  /foo");
        CHECK_FALSE(status.ok());
        CHECK(status.error_code() == ::grpc::StatusCode::INVALID_ARGUMENT);
        CHECK(std::string_view{status.error_message()}.contains(expected_message_fragment));
    }

    SECTION("content '/' alone is rejected") {
        const auto status = send_raw_content(*alice, alice_id, channel_id, "/");
        CHECK_FALSE(status.ok());
        CHECK(status.error_code() == ::grpc::StatusCode::INVALID_ARGUMENT);
        CHECK(std::string_view{status.error_message()}.contains(expected_message_fragment));
    }

    SECTION("content 'hello /world' with slash in the middle is accepted") {
        const auto status = send_raw_content(*alice, alice_id, channel_id, "hello /world");
        CHECK(status.ok());
    }

    SECTION("plain content 'hello' is accepted") {
        const auto status = send_raw_content(*alice, alice_id, channel_id, "hello");
        CHECK(status.ok());
    }
}
