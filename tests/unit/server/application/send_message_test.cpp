#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>

#include "bcmd/server/application/usecase/join_channel.hpp"
#include "bcmd/server/application/usecase/send_message.hpp"
#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/server/domain/model/message_content.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/server/domain/service/message_router.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"
#include "fakes/fake_channel_repository.hpp"
#include "fakes/fake_client_registry.hpp"
#include "fakes/fake_message_publisher.hpp"
#include "fakes/fake_message_repository.hpp"

namespace {

using bcmd::server::application::usecase::JoinChannel;
using bcmd::server::application::usecase::SendMessage;
using bcmd::server::domain::ChannelName;
using bcmd::server::domain::EchoPolicy;
using bcmd::server::domain::MessageContent;
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
    SendMessage use_case{channels, clients, messages, publisher};

    bcmd::ClientId registerClient(const char* name) {
        auto username = Username::create(name);
        REQUIRE(username.has_value());
        auto session = clients->registerClient(*username);
        REQUIRE(session.has_value());
        return session->id();
    }

    bcmd::ChannelId createChannel(const char* name) {
        auto channel_name = ChannelName::create(name);
        REQUIRE(channel_name.has_value());
        auto channel = channels->create(*channel_name);
        REQUIRE(channel.has_value());
        return channel->id();
    }
};

}  // namespace

TEST_CASE("SendMessage returns MessageEmpty for whitespace-only content",
          "[application][use-case][send-message]") {
    Fixture fixture;
    const auto sender = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(sender, channel_id).has_value());

    const auto result = fixture.use_case.execute(sender, channel_id, "   ");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::MessageEmpty);
    CHECK(fixture.publisher->deliveries.empty());
}

TEST_CASE("SendMessage returns MessageTooLong for oversize content",
          "[application][use-case][send-message]") {
    Fixture fixture;
    const auto sender = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(sender, channel_id).has_value());

    const std::string too_long(MessageContent::MAX_LENGTH + 1, 'x');
    const auto result = fixture.use_case.execute(sender, channel_id, too_long);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::MessageTooLong);
}

TEST_CASE("SendMessage rejects a sender that is not a channel member",
          "[application][use-case][send-message]") {
    Fixture fixture;
    const auto sender = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");

    const auto result = fixture.use_case.execute(sender, channel_id, "hi");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::NotAMember);
}

TEST_CASE("SendMessage publishes once per recipient (excluding sender by default)",
          "[application][use-case][send-message]") {
    Fixture fixture;
    const auto alice = fixture.registerClient("alice");
    const auto bob = fixture.registerClient("bob");
    const auto carol = fixture.registerClient("carol");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(alice, channel_id).has_value());
    REQUIRE(fixture.join_use_case.execute(bob, channel_id).has_value());
    REQUIRE(fixture.join_use_case.execute(carol, channel_id).has_value());

    const auto result = fixture.use_case.execute(alice, channel_id, "hello everyone");

    REQUIRE(result.has_value());
    CHECK(fixture.publisher->deliveries.size() == 2);
    CHECK(fixture.publisher->countFor(alice) == 0);
    CHECK(fixture.publisher->countFor(bob) == 1);
    CHECK(fixture.publisher->countFor(carol) == 1);
    CHECK(fixture.messages->totalFor(channel_id) == 1);
}

TEST_CASE("SendMessage echoes the sender when policy is IncludeSender",
          "[application][use-case][send-message]") {
    Fixture fixture;
    const auto alice = fixture.registerClient("alice");
    const auto bob = fixture.registerClient("bob");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(alice, channel_id).has_value());
    REQUIRE(fixture.join_use_case.execute(bob, channel_id).has_value());

    const auto result = fixture.use_case.execute(alice, channel_id, "echo",
                                                  EchoPolicy::IncludeSender);

    REQUIRE(result.has_value());
    CHECK(fixture.publisher->deliveries.size() == 2);
    CHECK(fixture.publisher->countFor(alice) == 1);
    CHECK(fixture.publisher->countFor(bob) == 1);
}

TEST_CASE("SendMessage persists the message via the repository",
          "[application][use-case][send-message]") {
    Fixture fixture;
    const auto sender = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(sender, channel_id).has_value());

    const auto result = fixture.use_case.execute(sender, channel_id, "  greetings  ");

    REQUIRE(result.has_value());
    const auto stored = fixture.messages->recent(channel_id, 10);
    REQUIRE(stored.size() == 1);
    CHECK(stored.front().id() == *result);
    CHECK(stored.front().content().value() == "greetings");
    CHECK(stored.front().senderId() == sender);
    CHECK(stored.front().channelId() == channel_id);
}

TEST_CASE("SendMessage emits live (non-replay) deliveries",
          "[application][use-case][send-message]") {
    Fixture fixture;
    const auto alice = fixture.registerClient("alice");
    const auto bob = fixture.registerClient("bob");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(alice, channel_id).has_value());
    REQUIRE(fixture.join_use_case.execute(bob, channel_id).has_value());

    REQUIRE(fixture.use_case.execute(alice, channel_id, "ping").has_value());

    REQUIRE(fixture.publisher->deliveries.size() == 1);
    CHECK_FALSE(fixture.publisher->deliveries.front().from_replay);
}
