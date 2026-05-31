#include "bcmd/server/application/usecase/list_messages.hpp"

#include "bcmd/server/application/usecase/join_channel.hpp"
#include "bcmd/server/application/usecase/send_message.hpp"
#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <catch2/catch_test_macros.hpp>

#include "fakes/fake_channel_repository.hpp"
#include "fakes/fake_client_registry.hpp"
#include "fakes/fake_message_publisher.hpp"
#include "fakes/fake_message_repository.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace {

using bcmd::server::application::usecase::JoinChannel;
using bcmd::server::application::usecase::ListMessages;
using bcmd::server::application::usecase::SendMessage;
using bcmd::server::domain::ChannelName;
using bcmd::server::domain::EchoPolicy;
using bcmd::server::domain::Username;
using bcmd::tests::FakeChannelRepository;
using bcmd::tests::FakeClientRegistry;
using bcmd::tests::FakeMessagePublisher;
using bcmd::tests::FakeMessageRepository;

struct Fixture {
    std::shared_ptr<FakeChannelRepository> channels = std::make_shared<FakeChannelRepository>();
    std::shared_ptr<FakeClientRegistry> clients = std::make_shared<FakeClientRegistry>();
    std::shared_ptr<FakeMessageRepository> messages = std::make_shared<FakeMessageRepository>();
    std::shared_ptr<FakeMessagePublisher> publisher = std::make_shared<FakeMessagePublisher>();
    JoinChannel join_use_case{channels, clients};
    SendMessage send_use_case{channels, clients, messages, publisher};
    ListMessages use_case{channels, messages};

    bcmd::ClientId registerClient(const char* name) const {
        auto username = Username::create(name);
        REQUIRE(username.has_value());
        auto session = clients->registerClient(*username);
        REQUIRE(session.has_value());
        return session->id();
    }

    bcmd::ChannelId createChannel(const char* name) const {
        auto channel_name = ChannelName::create(name);
        REQUIRE(channel_name.has_value());
        auto channel = channels->create(*channel_name);
        REQUIRE(channel.has_value());
        return channel->id();
    }

    bcmd::MessageId send(const bcmd::ClientId& sender, const bcmd::ChannelId& channel,
                         const std::string& content) {
        auto id = send_use_case.execute(sender, channel, content, EchoPolicy::IncludeSender);
        REQUIRE(id.has_value());
        return *id;
    }
};

}  // namespace

TEST_CASE("ListMessages returns ChannelNotFound for an unknown channel",
          "[application][use-case][list-messages]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto missing_channel = bcmd::ChannelId::generate();

    const auto result = fixture.use_case.execute(client_id, missing_channel, std::nullopt, 50);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::ChannelNotFound);
}

TEST_CASE("ListMessages returns NotAMember when the caller has not joined",
          "[application][use-case][list-messages]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");

    const auto result = fixture.use_case.execute(client_id, channel_id, std::nullopt, 50);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::NotAMember);
}

TEST_CASE("ListMessages returns the most recent messages when no cursor is provided",
          "[application][use-case][list-messages]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(client_id, channel_id).has_value());
    const auto first = fixture.send(client_id, channel_id, "one");
    const auto second = fixture.send(client_id, channel_id, "two");
    const auto third = fixture.send(client_id, channel_id, "three");

    const auto result = fixture.use_case.execute(client_id, channel_id, std::nullopt, 50);

    REQUIRE(result.has_value());
    REQUIRE(result->messages.size() == 3);
    CHECK(result->messages[0].id() == first);
    CHECK(result->messages[1].id() == second);
    CHECK(result->messages[2].id() == third);
    CHECK_FALSE(result->has_more);
}

TEST_CASE("ListMessages caps the limit at kServerMaxLimit",
          "[application][use-case][list-messages]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(client_id, channel_id).has_value());
    for (std::uint32_t i = 0; i < ListMessages::kServerMaxLimit + 50; ++i) {
        fixture.send(client_id, channel_id, "msg-" + std::to_string(i));
    }

    const auto result = fixture.use_case.execute(client_id, channel_id, std::nullopt,
                                                 ListMessages::kServerMaxLimit + 1000);

    REQUIRE(result.has_value());
    CHECK(result->messages.size() == ListMessages::kServerMaxLimit);
    CHECK(result->has_more);
}

TEST_CASE("ListMessages with a cursor returns messages strictly older than the cursor",
          "[application][use-case][list-messages]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(client_id, channel_id).has_value());
    const auto first = fixture.send(client_id, channel_id, "one");
    const auto second = fixture.send(client_id, channel_id, "two");
    const auto third = fixture.send(client_id, channel_id, "three");
    (void)third;

    const auto result = fixture.use_case.execute(client_id, channel_id, second, 50);

    REQUIRE(result.has_value());
    REQUIRE(result->messages.size() == 1);
    CHECK(result->messages[0].id() == first);
    CHECK_FALSE(result->has_more);
}

TEST_CASE("ListMessages reports has_more when older messages exist beyond the page",
          "[application][use-case][list-messages]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(client_id, channel_id).has_value());
    const auto first = fixture.send(client_id, channel_id, "one");
    const auto second = fixture.send(client_id, channel_id, "two");
    const auto third = fixture.send(client_id, channel_id, "three");
    (void)first;

    const auto result = fixture.use_case.execute(client_id, channel_id, third, 1);

    REQUIRE(result.has_value());
    REQUIRE(result->messages.size() == 1);
    CHECK(result->messages[0].id() == second);
    CHECK(result->has_more);
}

TEST_CASE("ListMessages with an unknown cursor returns empty without error",
          "[application][use-case][list-messages]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(client_id, channel_id).has_value());
    fixture.send(client_id, channel_id, "one");
    const auto unknown_cursor = bcmd::MessageId::generate();

    const auto result = fixture.use_case.execute(client_id, channel_id, unknown_cursor, 10);

    REQUIRE(result.has_value());
    CHECK(result->messages.empty());
    CHECK_FALSE(result->has_more);
}
