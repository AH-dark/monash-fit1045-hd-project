#include "bcmd/server/domain/model/message.hpp"

#include "bcmd/server/domain/model/message_content.hpp"
#include "bcmd/shared/ids.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <type_traits>

namespace {

bcmd::server::domain::MessageContent make_content(const char* raw) {
    auto content = bcmd::server::domain::MessageContent::create(raw);
    REQUIRE(content.has_value());
    return content.value();
}

}  // namespace

TEST_CASE("Message preserves the ids and content it was constructed with",
          "[domain][value-object][message]") {
    const auto message_id = bcmd::MessageId::generate();
    const auto sender_id = bcmd::ClientId::generate();
    const auto channel_id = bcmd::ChannelId::generate();
    const auto content = make_content("hello world");

    bcmd::server::domain::Message message{message_id, sender_id, channel_id, content};

    CHECK(message.id() == message_id);
    CHECK(message.senderId() == sender_id);
    CHECK(message.channelId() == channel_id);
    CHECK(message.content() == content);
}

TEST_CASE("Message::sentAt defaults to a time close to now", "[domain][value-object][message]") {
    const auto before = std::chrono::system_clock::now();
    bcmd::server::domain::Message message{
        bcmd::MessageId::generate(),
        bcmd::ClientId::generate(),
        bcmd::ChannelId::generate(),
        make_content("hi"),
    };
    const auto after = std::chrono::system_clock::now();

    CHECK(message.sentAt() >= before);
    CHECK(message.sentAt() <= after);
}

TEST_CASE("Message accepts an explicit sentAt timestamp", "[domain][value-object][message]") {
    const auto fixed = std::chrono::system_clock::time_point{std::chrono::seconds{42}};
    bcmd::server::domain::Message message{
        bcmd::MessageId::generate(),
        bcmd::ClientId::generate(),
        bcmd::ChannelId::generate(),
        make_content("hi"),
        fixed,
    };
    CHECK(message.sentAt() == fixed);
}

TEST_CASE("Message exposes only const accessors (compile-time check)",
          "[domain][value-object][message]") {
    const bcmd::server::domain::Message message{
        bcmd::MessageId::generate(),
        bcmd::ClientId::generate(),
        bcmd::ChannelId::generate(),
        make_content("hi"),
    };
    STATIC_REQUIRE(std::is_same_v<decltype(message.id()), const bcmd::MessageId&>);
    STATIC_REQUIRE(std::is_same_v<decltype(message.senderId()), const bcmd::ClientId&>);
    STATIC_REQUIRE(std::is_same_v<decltype(message.channelId()), const bcmd::ChannelId&>);
}
