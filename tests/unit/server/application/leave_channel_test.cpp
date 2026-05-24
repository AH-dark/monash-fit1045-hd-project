#include "bcmd/server/application/usecase/leave_channel.hpp"

#include "bcmd/server/application/usecase/join_channel.hpp"
#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <catch2/catch_test_macros.hpp>

#include "fakes/fake_channel_repository.hpp"
#include "fakes/fake_client_registry.hpp"
#include "fakes/fake_message_publisher.hpp"
#include <memory>

namespace {

using bcmd::server::application::usecase::JoinChannel;
using bcmd::server::application::usecase::LeaveChannel;
using bcmd::server::domain::ChannelName;
using bcmd::server::domain::Username;
using bcmd::tests::FakeChannelRepository;
using bcmd::tests::FakeClientRegistry;
using bcmd::tests::FakeMessagePublisher;

struct Fixture {
    std::shared_ptr<FakeChannelRepository> channels = std::make_shared<FakeChannelRepository>();
    std::shared_ptr<FakeClientRegistry> clients = std::make_shared<FakeClientRegistry>();
    std::shared_ptr<FakeMessagePublisher> publisher = std::make_shared<FakeMessagePublisher>();
    JoinChannel join_use_case{channels, clients};
    LeaveChannel use_case{channels, clients, publisher};

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

TEST_CASE("LeaveChannel returns ChannelNotFound when channel is missing",
          "[application][use-case][leave-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto missing_channel = bcmd::ChannelId::generate();

    const auto result = fixture.use_case.execute(client_id, missing_channel);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::ChannelNotFound);
    CHECK(fixture.publisher->broadcasts().empty());
}

TEST_CASE("LeaveChannel returns NotAMember when client never joined",
          "[application][use-case][leave-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");

    const auto result = fixture.use_case.execute(client_id, channel_id);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::NotAMember);
}

TEST_CASE("LeaveChannel does NOT broadcast when client is not a member",
          "[application][use-case][leave-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");

    const auto result = fixture.use_case.execute(client_id, channel_id);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::NotAMember);
    CHECK(fixture.publisher->broadcasts().empty());
}

TEST_CASE("LeaveChannel removes the client from the channel on success",
          "[application][use-case][leave-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(client_id, channel_id).has_value());

    const auto result = fixture.use_case.execute(client_id, channel_id);

    REQUIRE(result.has_value());
    auto stored = fixture.channels->findById(channel_id);
    REQUIRE(stored.has_value());
    CHECK(stored->memberCount() == 0);
    CHECK_FALSE(stored->hasMember(client_id));

    auto session = fixture.clients->findById(client_id);
    REQUIRE(session.has_value());
    CHECK_FALSE(session->isInChannel(channel_id));
}

TEST_CASE("LeaveChannel broadcasts MemberLeftEvent on successful leave",
          "[application][use-case][leave-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(client_id, channel_id).has_value());

    const auto result = fixture.use_case.execute(client_id, channel_id);

    REQUIRE(result.has_value());
    REQUIRE(fixture.publisher->broadcasts().size() == 1);
    const auto& record = fixture.publisher->broadcasts().front();
    CHECK(record.channel_id == channel_id);
    CHECK(record.client_id == client_id);
    CHECK(record.username == "alice");
    CHECK(record.recipients.empty());
}

TEST_CASE("LeaveChannel broadcasts MemberLeft only to remaining channel members",
          "[application][use-case][leave-channel]") {
    Fixture fixture;
    const auto leaver_id = fixture.registerClient("alice");
    const auto remaining_id = fixture.registerClient("bob");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(leaver_id, channel_id).has_value());
    REQUIRE(fixture.join_use_case.execute(remaining_id, channel_id).has_value());

    const auto result = fixture.use_case.execute(leaver_id, channel_id);

    REQUIRE(result.has_value());
    REQUIRE(fixture.publisher->broadcasts().size() == 1);
    const auto& record = fixture.publisher->broadcasts().front();
    CHECK(record.channel_id == channel_id);
    CHECK(record.client_id == leaver_id);
    CHECK(record.recipients.size() == 1);
    CHECK(record.recipients.contains(remaining_id));
    CHECK_FALSE(record.recipients.contains(leaver_id));
}

TEST_CASE("LeaveChannel a second time returns NotAMember",
          "[application][use-case][leave-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(client_id, channel_id).has_value());
    REQUIRE(fixture.use_case.execute(client_id, channel_id).has_value());

    const auto second = fixture.use_case.execute(client_id, channel_id);

    REQUIRE_FALSE(second.has_value());
    CHECK(second.error() == bcmd::Error::NotAMember);
    CHECK(fixture.publisher->broadcasts().size() == 1);
}
