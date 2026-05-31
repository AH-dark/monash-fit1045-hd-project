#include "bcmd/server/application/usecase/join_channel.hpp"

#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <catch2/catch_test_macros.hpp>

#include "fakes/fake_channel_list_publisher.hpp"
#include "fakes/fake_channel_repository.hpp"
#include "fakes/fake_client_registry.hpp"
#include "fakes/fake_message_publisher.hpp"
#include <memory>

namespace {

using bcmd::server::application::usecase::JoinChannel;
using bcmd::server::domain::Channel;
using bcmd::server::domain::ChannelName;
using bcmd::server::domain::Username;
using bcmd::tests::FakeChannelListPublisher;
using bcmd::tests::FakeChannelRepository;
using bcmd::tests::FakeClientRegistry;
using bcmd::tests::FakeMessagePublisher;

struct Fixture {
    std::shared_ptr<FakeChannelRepository> channels = std::make_shared<FakeChannelRepository>();
    std::shared_ptr<FakeClientRegistry> clients = std::make_shared<FakeClientRegistry>();
    std::shared_ptr<FakeMessagePublisher> message_publisher =
        std::make_shared<FakeMessagePublisher>();
    std::shared_ptr<FakeChannelListPublisher> channel_list_publisher =
        std::make_shared<FakeChannelListPublisher>();
    JoinChannel use_case{channels, clients, message_publisher, channel_list_publisher};

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
};

}  // namespace

TEST_CASE("JoinChannel returns ChannelNotFound when channel is missing",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto missing_channel = bcmd::ChannelId::generate();

    const auto result = fixture.use_case.execute(client_id, missing_channel);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::ChannelNotFound);
}

TEST_CASE("JoinChannel returns ClientNotFound when client is unknown",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto channel_id = fixture.createChannel("general");
    const auto unknown_client = bcmd::ClientId::generate();

    const auto result = fixture.use_case.execute(unknown_client, channel_id);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::ClientNotFound);
}

TEST_CASE("JoinChannel adds the client to the channel on success",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");

    const auto result = fixture.use_case.execute(client_id, channel_id);

    REQUIRE(result.has_value());
    auto stored = fixture.channels->findById(channel_id);
    REQUIRE(stored.has_value());
    CHECK(stored->memberCount() == 1);
    CHECK(stored->hasMember(client_id));

    auto session = fixture.clients->findById(client_id);
    REQUIRE(session.has_value());
    CHECK(session->isInChannel(channel_id));
}

TEST_CASE("JoinChannel is idempotent when re-joining the same channel",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");

    REQUIRE(fixture.use_case.execute(client_id, channel_id).has_value());
    const auto second = fixture.use_case.execute(client_id, channel_id);

    REQUIRE(second.has_value());

    auto stored = fixture.channels->findById(channel_id);
    REQUIRE(stored.has_value());
    CHECK(stored->memberCount() == 1);
    CHECK(stored->hasMember(client_id));
    CHECK(fixture.message_publisher->joinedBroadcasts().size() == 1);
    CHECK(fixture.channel_list_publisher->publishMemberCountChangedCallCount() == 1);
}

TEST_CASE("JoinChannel leaves the previous channel when joining a different one",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto general_id = fixture.createChannel("general");
    const auto random_id = fixture.createChannel("random");

    REQUIRE(fixture.use_case.execute(client_id, general_id).has_value());
    REQUIRE(fixture.use_case.execute(client_id, random_id).has_value());

    auto general = fixture.channels->findById(general_id);
    REQUIRE(general.has_value());
    CHECK(general->memberCount() == 0);
    CHECK_FALSE(general->hasMember(client_id));

    auto random = fixture.channels->findById(random_id);
    REQUIRE(random.has_value());
    CHECK(random->memberCount() == 1);
    CHECK(random->hasMember(client_id));

    auto session = fixture.clients->findById(client_id);
    REQUIRE(session.has_value());
    CHECK(session->joinedChannels().size() == 1);
    CHECK(session->isInChannel(random_id));
    CHECK_FALSE(session->isInChannel(general_id));
}

TEST_CASE("JoinChannel broadcasts memberLeft when leaving the previous channel",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto alice = fixture.registerClient("alice");
    const auto general_id = fixture.createChannel("general");
    const auto random_id = fixture.createChannel("random");

    REQUIRE(fixture.use_case.execute(alice, general_id).has_value());
    REQUIRE(fixture.use_case.execute(alice, random_id).has_value());

    const auto& left = fixture.message_publisher->broadcasts();
    REQUIRE(left.size() == 1);
    CHECK(left.front().channel_id == general_id);
    CHECK(left.front().client_id == alice);
    CHECK(left.front().username == "alice");
}

TEST_CASE("JoinChannel publishes MemberCountChanged for both the left and joined channels",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto general_id = fixture.createChannel("general");
    const auto random_id = fixture.createChannel("random");

    REQUIRE(fixture.use_case.execute(client_id, general_id).has_value());
    REQUIRE(fixture.use_case.execute(client_id, random_id).has_value());

    const auto& calls = fixture.channel_list_publisher->memberCountCalls();
    REQUIRE(calls.size() == 3);
    CHECK(calls[0].channel_id == general_id);
    CHECK(calls[0].member_count == 1);
    CHECK(calls[1].channel_id == general_id);
    CHECK(calls[1].member_count == 0);
    CHECK(calls[2].channel_id == random_id);
    CHECK(calls[2].member_count == 1);
}

TEST_CASE("JoinChannel::executeByName is idempotent when already a member of the same channel",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");

    REQUIRE(fixture.use_case.executeByName(client_id, "general").has_value());
    const auto second = fixture.use_case.executeByName(client_id, "general");

    REQUIRE(second.has_value());
    CHECK(*second == channel_id);

    auto stored = fixture.channels->findById(channel_id);
    REQUIRE(stored.has_value());
    CHECK(stored->memberCount() == 1);
    CHECK(fixture.message_publisher->joinedBroadcasts().size() == 1);
}

TEST_CASE("JoinChannel::executeByName also leaves the previous channel",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto general_id = fixture.createChannel("general");
    fixture.createChannel("random");

    REQUIRE(fixture.use_case.executeByName(client_id, "general").has_value());
    REQUIRE(fixture.use_case.executeByName(client_id, "random").has_value());

    auto general = fixture.channels->findById(general_id);
    REQUIRE(general.has_value());
    CHECK(general->memberCount() == 0);

    auto session = fixture.clients->findById(client_id);
    REQUIRE(session.has_value());
    CHECK(session->joinedChannels().size() == 1);
    CHECK_FALSE(session->isInChannel(general_id));
}

TEST_CASE("JoinChannel::executeByName returns ChannelNotFound when absent",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");

    const auto result = fixture.use_case.executeByName(client_id, "general");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::ChannelNotFound);
}

TEST_CASE("JoinChannel::executeByName reuses an existing channel",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto alice = fixture.registerClient("alice");
    const auto bob = fixture.registerClient("bob");
    const auto existing = fixture.createChannel("general");

    const auto resolved = fixture.use_case.executeByName(alice, "general");

    REQUIRE(resolved.has_value());
    CHECK(*resolved == existing);

    REQUIRE(fixture.use_case.executeByName(bob, "general").has_value());
    auto stored = fixture.channels->findById(existing);
    REQUIRE(stored.has_value());
    CHECK(stored->memberCount() == 2);
}

TEST_CASE("JoinChannel::executeByName rejects an invalid name",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");

    const auto result = fixture.use_case.executeByName(client_id, "");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::InvalidChannelName);
}

TEST_CASE(
    "join_channel successful join via execute(channel_id) calls broadcastMemberJoined exactly once",
    "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");

    const auto result = fixture.use_case.execute(client_id, channel_id);

    REQUIRE(result.has_value());
    REQUIRE(fixture.message_publisher->joinedBroadcasts().size() == 1);
    const auto& record = fixture.message_publisher->joinedBroadcasts().front();
    CHECK(record.channel_id == channel_id);
    CHECK(record.client_id == client_id);
    CHECK(record.username == "alice");
}

TEST_CASE("join_channel successful join via executeByName also calls broadcastMemberJoined",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");

    const auto result = fixture.use_case.executeByName(client_id, "general");

    REQUIRE(result.has_value());
    CHECK(*result == channel_id);
    REQUIRE(fixture.message_publisher->joinedBroadcasts().size() == 1);
    const auto& record = fixture.message_publisher->joinedBroadcasts().front();
    CHECK(record.channel_id == channel_id);
    CHECK(record.client_id == client_id);
}

TEST_CASE(
    "join_channel successful join publishes MemberCountChangedEvent on channel-list publisher",
    "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");

    const auto result = fixture.use_case.execute(client_id, channel_id);

    REQUIRE(result.has_value());
    REQUIRE(fixture.channel_list_publisher->publishMemberCountChangedCallCount() == 1);
    const auto& record = fixture.channel_list_publisher->memberCountCalls().front();
    CHECK(record.channel_id == channel_id);
    CHECK(record.member_count == 1);
}

TEST_CASE("join_channel failed join does NOT publish any event",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto missing_channel = bcmd::ChannelId::generate();

    const auto result = fixture.use_case.execute(client_id, missing_channel);

    REQUIRE_FALSE(result.has_value());
    CHECK(fixture.message_publisher->joinedBroadcasts().empty());
    CHECK(fixture.channel_list_publisher->publishMemberCountChangedCallCount() == 0);
    CHECK(fixture.channel_list_publisher->publishChannelCreatedCallCount() == 0);
}
