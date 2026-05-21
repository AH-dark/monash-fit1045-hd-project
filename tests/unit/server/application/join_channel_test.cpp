#include "bcmd/server/application/usecase/join_channel.hpp"

#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <catch2/catch_test_macros.hpp>

#include "fakes/fake_channel_repository.hpp"
#include "fakes/fake_client_registry.hpp"
#include <memory>

namespace {

using bcmd::server::application::usecase::JoinChannel;
using bcmd::server::domain::Channel;
using bcmd::server::domain::ChannelName;
using bcmd::server::domain::Username;
using bcmd::tests::FakeChannelRepository;
using bcmd::tests::FakeClientRegistry;

struct Fixture {
    std::shared_ptr<FakeChannelRepository> channels = std::make_shared<FakeChannelRepository>();
    std::shared_ptr<FakeClientRegistry> clients = std::make_shared<FakeClientRegistry>();
    JoinChannel use_case{channels, clients};

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

TEST_CASE("JoinChannel returns AlreadyMember when re-joining",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");

    REQUIRE(fixture.use_case.execute(client_id, channel_id).has_value());
    const auto second = fixture.use_case.execute(client_id, channel_id);

    REQUIRE_FALSE(second.has_value());
    CHECK(second.error() == bcmd::Error::AlreadyMember);
}

TEST_CASE("JoinChannel::executeByName creates the channel when absent",
          "[application][use-case][join-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");

    const auto created = fixture.use_case.executeByName(client_id, "general");

    REQUIRE(created.has_value());
    auto stored = fixture.channels->findById(*created);
    REQUIRE(stored.has_value());
    CHECK(stored->hasMember(client_id));
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
