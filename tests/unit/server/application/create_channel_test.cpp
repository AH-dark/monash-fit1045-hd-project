#include "bcmd/server/application/usecase/create_channel.hpp"

#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"

#include <catch2/catch_test_macros.hpp>

#include "fakes/fake_channel_list_publisher.hpp"
#include "fakes/fake_channel_repository.hpp"
#include "fakes/fake_client_registry.hpp"
#include <memory>

namespace {

using bcmd::server::application::usecase::CreateChannel;
using bcmd::server::domain::Username;
using bcmd::tests::FakeChannelListPublisher;
using bcmd::tests::FakeChannelRepository;
using bcmd::tests::FakeClientRegistry;

struct Fixture {
    std::shared_ptr<FakeChannelRepository> channels = std::make_shared<FakeChannelRepository>();
    std::shared_ptr<FakeClientRegistry> clients = std::make_shared<FakeClientRegistry>();
    std::shared_ptr<FakeChannelListPublisher> channel_list_publisher =
        std::make_shared<FakeChannelListPublisher>();
    CreateChannel use_case{channels, clients, channel_list_publisher};

    bcmd::ClientId registerClient(const char* name) const {
        auto username = Username::create(name);
        REQUIRE(username.has_value());
        auto session = clients->registerClient(*username);
        REQUIRE(session.has_value());
        return session->id();
    }
};

}  // namespace

TEST_CASE("create_channel successful create publishes ChannelCreatedEvent once",
          "[application][use-case][create-channel]") {
    Fixture fixture;
    const auto client_id = fixture.registerClient("alice");

    const auto result = fixture.use_case.execute(client_id, "general");

    REQUIRE(result.has_value());
    REQUIRE(fixture.channel_list_publisher->publishChannelCreatedCallCount() == 1);
    const auto& record = fixture.channel_list_publisher->createdCalls().front();
    CHECK(record.channel.id() == *result);
    CHECK(record.channel.name().value() == "general");
}
