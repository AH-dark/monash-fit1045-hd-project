#include "bcmd/server/application/usecase/subscribe_to_channel_list.hpp"

#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <catch2/catch_test_macros.hpp>

#include "fakes/fake_channel_list_publisher.hpp"
#include "fakes/fake_client_registry.hpp"
#include <memory>

namespace {

using bcmd::server::application::usecase::SubscribeToChannelList;
using bcmd::server::domain::Username;
using bcmd::tests::FakeChannelListPublisher;
using bcmd::tests::FakeClientRegistry;

struct Fixture {
    std::shared_ptr<FakeClientRegistry> clients = std::make_shared<FakeClientRegistry>();
    std::shared_ptr<FakeChannelListPublisher> publisher =
        std::make_shared<FakeChannelListPublisher>();
    SubscribeToChannelList use_case{clients, publisher};

    bcmd::ClientId registerClient(const char* name) const {
        auto username = Username::create(name);
        REQUIRE(username.has_value());
        auto session = clients->registerClient(*username);
        REQUIRE(session.has_value());
        return session->id();
    }
};

}  // namespace

TEST_CASE("subscribe_to_channel_list execute returns ClientNotFound when client_id is unknown",
          "[application][use-case][subscribe-to-channel-list]") {
    Fixture fixture;

    const auto result = fixture.use_case.execute(bcmd::ClientId::generate(), nullptr);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::ClientNotFound);
    CHECK(fixture.publisher->registerSubscriberWithSnapshotCallCount() == 0);
}

TEST_CASE(
    "subscribe_to_channel_list execute calls publisher registerSubscriberWithSnapshot on valid "
    "client_id",
    "[application][use-case][subscribe-to-channel-list]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");

    const auto result = fixture.use_case.execute(client_id, nullptr);

    REQUIRE(result.has_value());
    REQUIRE(fixture.publisher->registerSubscriberWithSnapshotCallCount() == 1);
    CHECK(fixture.publisher->registerCalls().front().subscriber_id == client_id);
    CHECK(fixture.publisher->registerCalls().front().writer == nullptr);
}
