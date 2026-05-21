#include "bcmd/client/application/usecase/send_message_command.hpp"

#include "bcmd/client/application/usecase/connect_to_server.hpp"
#include "bcmd/shared/result.hpp"

#include <catch2/catch_test_macros.hpp>

#include "fakes/fake_server_gateway.hpp"
#include <expected>
#include <memory>
#include <string>

namespace {

using bcmd::client::application::usecase::ConnectToServer;
using bcmd::client::application::usecase::SendMessageCommand;
using bcmd::tests::FakeServerGateway;

}  // namespace

// NOLINTBEGIN(misc-use-anonymous-namespace, cppcoreguidelines-avoid-do-while,
// readability-identifier-naming)

TEST_CASE("SendMessageCommand sends message via gateway and returns message id",
          "[client-application]") {
    auto gateway = std::make_shared<FakeServerGateway>();
    gateway->send_message_result = std::string{"msg-42"};
    SendMessageCommand use_case{gateway};

    const auto result = use_case.execute("client-1", "channel-1", "hello");

    REQUIRE(result.has_value());
    CHECK(*result == "msg-42");
    CHECK(gateway->send_message_calls == 1);
    CHECK(gateway->last_client_id == "client-1");
    CHECK(gateway->last_channel_id == "channel-1");
    CHECK(gateway->last_sent_content == "hello");
}

TEST_CASE("SendMessageCommand propagates gateway errors", "[client-application]") {
    auto gateway = std::make_shared<FakeServerGateway>();
    gateway->send_message_result = std::unexpected(bcmd::Error::NetworkError);
    SendMessageCommand use_case{gateway};

    const auto result = use_case.execute("client-1", "channel-1", "hello");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::NetworkError);
    CHECK(gateway->send_message_calls == 1);
}

TEST_CASE("ConnectToServer returns client id", "[client-application]") {
    auto gateway = std::make_shared<FakeServerGateway>();
    gateway->connect_result = std::string{"client-99"};
    ConnectToServer use_case{gateway};

    const auto result = use_case.execute("alice");

    REQUIRE(result.has_value());
    CHECK(*result == "client-99");
    CHECK(gateway->connect_calls == 1);
    CHECK(gateway->last_username == "alice");
}

TEST_CASE("ConnectToServer propagates gateway errors", "[client-application]") {
    auto gateway = std::make_shared<FakeServerGateway>();
    gateway->connect_result = std::unexpected(bcmd::Error::InvalidUsername);
    ConnectToServer use_case{gateway};

    const auto result = use_case.execute("");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::InvalidUsername);
    CHECK(gateway->connect_calls == 1);
}

// NOLINTEND(misc-use-anonymous-namespace, cppcoreguidelines-avoid-do-while,
// readability-identifier-naming)
