#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>

#include "bcmd/server/application/usecase/list_channels.hpp"
#include "bcmd/server/domain/model/channel_name.hpp"
#include "fakes/fake_channel_repository.hpp"

namespace {

using bcmd::server::application::usecase::ListChannels;
using bcmd::server::domain::ChannelName;
using bcmd::tests::FakeChannelRepository;

ChannelName make_name(const char* raw) {
    auto name = ChannelName::create(raw);
    REQUIRE(name.has_value());
    return *name;
}

}  // namespace

TEST_CASE("ListChannels returns empty when repository is empty",
          "[application][use-case][list-channels]") {
    auto channels = std::make_shared<FakeChannelRepository>();
    ListChannels use_case{channels};

    CHECK(use_case.execute().empty());
}

TEST_CASE("ListChannels returns every channel held by the repository",
          "[application][use-case][list-channels]") {
    auto channels = std::make_shared<FakeChannelRepository>();
    REQUIRE(channels->create(make_name("general")).has_value());
    REQUIRE(channels->create(make_name("random")).has_value());
    REQUIRE(channels->create(make_name("dev-team")).has_value());
    ListChannels use_case{channels};

    const auto listed = use_case.execute();

    CHECK(listed.size() == 3);
    const auto has_name = [&](const char* needle) {
        return std::ranges::any_of(listed, [&](const auto& channel) {
            return channel.name().value() == needle;
        });
    };
    CHECK(has_name("general"));
    CHECK(has_name("random"));
    CHECK(has_name("dev-team"));
}
