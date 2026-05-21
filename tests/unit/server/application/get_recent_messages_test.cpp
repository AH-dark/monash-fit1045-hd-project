#include "bcmd/server/application/usecase/get_recent_messages.hpp"

#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/server/domain/model/message_content.hpp"
#include "bcmd/shared/ids.hpp"

#include <catch2/catch_test_macros.hpp>

#include "fakes/fake_message_repository.hpp"
#include <cstddef>
#include <memory>
#include <string>

namespace {

using bcmd::server::application::usecase::GetRecentMessages;
using bcmd::server::domain::Message;
using bcmd::server::domain::MessageContent;
using bcmd::tests::FakeMessageRepository;

MessageContent make_content(const std::string& raw) {
    auto content = MessageContent::create(raw);
    REQUIRE(content.has_value());
    return *content;
}

void seed_messages(FakeMessageRepository& repo, const bcmd::ChannelId& channel_id,
                   const bcmd::ClientId& sender_id, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        const Message message{bcmd::MessageId::generate(), sender_id, channel_id,
                              make_content("msg-" + std::to_string(i))};
        REQUIRE(repo.save(message).has_value());
    }
}

}  // namespace

TEST_CASE("GetRecentMessages returns the requested count when enough exist",
          "[application][use-case][get-recent-messages]") {
    auto repo = std::make_shared<FakeMessageRepository>();
    const auto channel_id = bcmd::ChannelId::generate();
    const auto sender_id = bcmd::ClientId::generate();
    seed_messages(*repo, channel_id, sender_id, 10);
    GetRecentMessages use_case{repo};

    const auto recent = use_case.execute(channel_id, 5);

    CHECK(recent.size() == 5);
    CHECK(recent.front().content().value() == "msg-5");
    CHECK(recent.back().content().value() == "msg-9");
}

TEST_CASE("GetRecentMessages caps at the server maximum",
          "[application][use-case][get-recent-messages]") {
    auto repo = std::make_shared<FakeMessageRepository>();
    const auto channel_id = bcmd::ChannelId::generate();
    const auto sender_id = bcmd::ClientId::generate();
    seed_messages(*repo, channel_id, sender_id, GetRecentMessages::kServerMaxMessages + 50);
    GetRecentMessages use_case{repo};

    const auto recent = use_case.execute(channel_id, GetRecentMessages::kServerMaxMessages + 1000);

    CHECK(recent.size() == GetRecentMessages::kServerMaxMessages);
}

TEST_CASE("GetRecentMessages returns all available when fewer than requested",
          "[application][use-case][get-recent-messages]") {
    auto repo = std::make_shared<FakeMessageRepository>();
    const auto channel_id = bcmd::ChannelId::generate();
    const auto sender_id = bcmd::ClientId::generate();
    seed_messages(*repo, channel_id, sender_id, 3);
    GetRecentMessages use_case{repo};

    const auto recent = use_case.execute(channel_id, 5);

    CHECK(recent.size() == 3);
}

TEST_CASE("GetRecentMessages returns empty for unknown channel",
          "[application][use-case][get-recent-messages]") {
    auto repo = std::make_shared<FakeMessageRepository>();
    GetRecentMessages use_case{repo};

    CHECK(use_case.execute(bcmd::ChannelId::generate(), 10).empty());
}
