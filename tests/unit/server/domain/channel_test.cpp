#include "bcmd/server/domain/model/channel.hpp"

#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

bcmd::server::domain::ChannelName make_name(const char* raw) {
    auto name = bcmd::server::domain::ChannelName::create(raw);
    REQUIRE(name.has_value());
    return *name;
}

}  // namespace

TEST_CASE("New channel has no members", "[domain][entity][channel]") {
    bcmd::server::domain::Channel channel{bcmd::ChannelId::generate(), make_name("general")};
    CHECK(channel.memberCount() == 0);
    CHECK(channel.members().empty());
}

TEST_CASE("Channel exposes the id and name it was constructed with", "[domain][entity][channel]") {
    const auto channel_id = bcmd::ChannelId::generate();
    const auto channel_name = make_name("dev-team");
    bcmd::server::domain::Channel channel{channel_id, channel_name};
    CHECK(channel.id() == channel_id);
    CHECK(channel.name() == channel_name);
}

TEST_CASE("Channel::addMember inserts a brand-new client", "[domain][entity][channel]") {
    bcmd::server::domain::Channel channel{bcmd::ChannelId::generate(), make_name("general")};
    const auto client_id = bcmd::ClientId::generate();
    const auto result = channel.addMember(client_id);
    REQUIRE(result.has_value());
    CHECK(channel.memberCount() == 1);
    CHECK(channel.hasMember(client_id));
}

TEST_CASE("Channel::addMember returns AlreadyMember for a duplicate insert",
          "[domain][entity][channel]") {
    bcmd::server::domain::Channel channel{bcmd::ChannelId::generate(), make_name("general")};
    const auto client_id = bcmd::ClientId::generate();
    REQUIRE(channel.addMember(client_id).has_value());

    const auto second = channel.addMember(client_id);
    REQUIRE_FALSE(second.has_value());
    CHECK(second.error() == bcmd::Error::AlreadyMember);
    CHECK(channel.memberCount() == 1);
}

TEST_CASE("Channel::removeMember returns NotAMember when the client is absent",
          "[domain][entity][channel]") {
    bcmd::server::domain::Channel channel{bcmd::ChannelId::generate(), make_name("general")};
    const auto result = channel.removeMember(bcmd::ClientId::generate());
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::NotAMember);
}

TEST_CASE("Channel::removeMember removes an existing client", "[domain][entity][channel]") {
    bcmd::server::domain::Channel channel{bcmd::ChannelId::generate(), make_name("general")};
    const auto client_id = bcmd::ClientId::generate();
    REQUIRE(channel.addMember(client_id).has_value());

    const auto removed = channel.removeMember(client_id);
    REQUIRE(removed.has_value());
    CHECK(channel.memberCount() == 0);
    CHECK_FALSE(channel.hasMember(client_id));
}

TEST_CASE("Channel tracks multiple distinct members", "[domain][entity][channel]") {
    bcmd::server::domain::Channel channel{bcmd::ChannelId::generate(), make_name("general")};
    const auto first = bcmd::ClientId::generate();
    const auto second = bcmd::ClientId::generate();
    const auto third = bcmd::ClientId::generate();
    REQUIRE(channel.addMember(first).has_value());
    REQUIRE(channel.addMember(second).has_value());
    REQUIRE(channel.addMember(third).has_value());
    CHECK(channel.memberCount() == 3);
    CHECK(channel.hasMember(first));
    CHECK(channel.hasMember(second));
    CHECK(channel.hasMember(third));
}

TEST_CASE("Channel::hasMember returns false for unknown clients", "[domain][entity][channel]") {
    bcmd::server::domain::Channel channel{bcmd::ChannelId::generate(), make_name("general")};
    CHECK_FALSE(channel.hasMember(bcmd::ClientId::generate()));
}
