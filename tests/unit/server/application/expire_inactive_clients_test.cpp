#include "bcmd/server/application/usecase/expire_inactive_clients.hpp"

#include "bcmd/server/application/usecase/join_channel.hpp"
#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"

#include <catch2/catch_test_macros.hpp>

#include "fakes/fake_channel_repository.hpp"
#include "fakes/fake_client_registry.hpp"
#include "fakes/fake_message_publisher.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <unordered_set>

namespace {

using bcmd::server::application::usecase::ExpireInactiveClients;
using bcmd::server::application::usecase::JoinChannel;
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
    ExpireInactiveClients use_case{clients, channels, publisher};

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

std::chrono::steady_clock::time_point deadlineExpiringEverySession() {
    return std::chrono::steady_clock::now() + std::chrono::hours(1);
}

std::chrono::steady_clock::time_point deadlineExpiringNoSession() {
    return std::chrono::steady_clock::now() - std::chrono::hours(1);
}

}  // namespace

TEST_CASE("ExpireInactiveClients returns 0 and produces no side effects when no clients expired",
          "[application][use-case][expire-inactive-clients]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(client_id, channel_id).has_value());

    const auto expired_count = fixture.use_case.run(deadlineExpiringNoSession());

    CHECK(expired_count == 0);
    CHECK(fixture.publisher->broadcasts().empty());
    CHECK(fixture.publisher->unregisterCalls().empty());
    CHECK(fixture.clients->findById(client_id).has_value());
}

TEST_CASE(
    "ExpireInactiveClients broadcasts MemberLeft for every joined channel and removes the "
    "session",
    "[application][use-case][expire-inactive-clients]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto general_id = fixture.createChannel("general");
    const auto random_id = fixture.createChannel("random");
    REQUIRE(fixture.join_use_case.execute(client_id, general_id).has_value());
    REQUIRE(fixture.join_use_case.execute(client_id, random_id).has_value());

    const auto expired_count = fixture.use_case.run(deadlineExpiringEverySession());

    CHECK(expired_count == 1);
    REQUIRE(fixture.publisher->broadcasts().size() == 2);
    const auto& broadcasts = fixture.publisher->broadcasts();
    const bool has_general =
        std::any_of(broadcasts.begin(), broadcasts.end(),
                    [&](const auto& record) { return record.channel_id == general_id; });
    const bool has_random =
        std::any_of(broadcasts.begin(), broadcasts.end(),
                    [&](const auto& record) { return record.channel_id == random_id; });
    CHECK(has_general);
    CHECK(has_random);
    for (const auto& record : broadcasts) {
        CHECK(record.client_id == client_id);
        CHECK(record.username == "alice");
    }

    REQUIRE(fixture.publisher->unregisterCalls().size() == 1);
    CHECK(fixture.publisher->unregisterCalls().front() == client_id);

    auto lookup = fixture.clients->findById(client_id);
    REQUIRE_FALSE(lookup.has_value());
    CHECK(lookup.error() == bcmd::Error::ClientNotFound);
}

TEST_CASE("ExpireInactiveClients tolerates a concurrently-removed client (swallow ClientNotFound)",
          "[application][use-case][expire-inactive-clients]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(client_id, channel_id).has_value());

    REQUIRE(fixture.clients->remove(client_id).has_value());

    const auto expired_count = fixture.use_case.run(deadlineExpiringEverySession());

    CHECK(expired_count == 0);
    CHECK(fixture.publisher->broadcasts().empty());
    CHECK(fixture.publisher->unregisterCalls().empty());
}

TEST_CASE("ExpireInactiveClients only removes sessions older than the deadline",
          "[application][use-case][expire-inactive-clients]") {
    Fixture fixture;
    const auto stale_a = fixture.registerClient("alice");
    const auto stale_b = fixture.registerClient("bob");
    const auto fresh = fixture.registerClient("carol");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(stale_a, channel_id).has_value());
    REQUIRE(fixture.join_use_case.execute(stale_b, channel_id).has_value());
    REQUIRE(fixture.join_use_case.execute(fresh, channel_id).has_value());

    REQUIRE(fixture.clients->touchHeartbeat(fresh).has_value());
    auto fresh_session = fixture.clients->findById(fresh);
    REQUIRE(fresh_session.has_value());
    const auto deadline = fresh_session->lastHeartbeatAt();

    const auto expired_count = fixture.use_case.run(deadline);

    CHECK(expired_count == 2);
    REQUIRE(fixture.publisher->unregisterCalls().size() == 2);
    const auto& unregistered = fixture.publisher->unregisterCalls();
    const bool unregistered_alice =
        std::find(unregistered.begin(), unregistered.end(), stale_a) != unregistered.end();
    const bool unregistered_bob =
        std::find(unregistered.begin(), unregistered.end(), stale_b) != unregistered.end();
    CHECK(unregistered_alice);
    CHECK(unregistered_bob);

    CHECK_FALSE(fixture.clients->findById(stale_a).has_value());
    CHECK_FALSE(fixture.clients->findById(stale_b).has_value());
    CHECK(fixture.clients->findById(fresh).has_value());
}

TEST_CASE("ExpireInactiveClients broadcasts post-removal recipient snapshots",
          "[application][use-case][expire-inactive-clients]") {
    Fixture fixture;
    const auto alice = fixture.registerClient("alice");
    const auto bob = fixture.registerClient("bob");
    const auto carol = fixture.registerClient("carol");
    const auto channel_id = fixture.createChannel("general");
    REQUIRE(fixture.join_use_case.execute(alice, channel_id).has_value());
    REQUIRE(fixture.join_use_case.execute(bob, channel_id).has_value());
    REQUIRE(fixture.join_use_case.execute(carol, channel_id).has_value());

    const auto expired_count = fixture.use_case.run(deadlineExpiringEverySession());

    CHECK(expired_count == 3);
    REQUIRE(fixture.publisher->broadcasts().size() == 3);
    std::unordered_set<bcmd::ClientId> removed;
    for (const auto& record : fixture.publisher->broadcasts()) {
        CHECK(record.channel_id == channel_id);
        CHECK_FALSE(record.recipients.contains(record.client_id));
        for (const auto& previously_removed : removed) {
            CHECK_FALSE(record.recipients.contains(previously_removed));
        }
        removed.insert(record.client_id);
    }
    CHECK(fixture.publisher->broadcasts().at(0).recipients.size() == 2);
    CHECK(fixture.publisher->broadcasts().at(1).recipients.size() == 1);
    CHECK(fixture.publisher->broadcasts().at(2).recipients.empty());
}
