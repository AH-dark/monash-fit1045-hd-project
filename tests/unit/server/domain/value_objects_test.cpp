#include <string>

#include <catch2/catch_test_macros.hpp>

#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/server/domain/model/message_content.hpp"
#include "bcmd/server/domain/model/username.hpp"

using bcmd::server::domain::ChannelName;
using bcmd::server::domain::MessageContent;
using bcmd::server::domain::Username;

TEST_CASE("Username::create accepts a simple lowercase name", "[domain][value-object][username]") {
    const auto name = Username::create("alice");
    REQUIRE(name.has_value());
    CHECK(name->value() == "alice");
}

TEST_CASE("Username::create accepts a name with digits, underscore and hyphen",
          "[domain][value-object][username]") {
    const auto name = Username::create("user-123_ok");
    REQUIRE(name.has_value());
    CHECK(name->value() == "user-123_ok");
}

TEST_CASE("Username::create rejects an empty string", "[domain][value-object][username]") {
    CHECK_FALSE(Username::create("").has_value());
}

TEST_CASE("Username::create rejects input longer than 32 characters",
          "[domain][value-object][username]") {
    const std::string too_long(33, 'a');
    CHECK_FALSE(Username::create(too_long).has_value());
}

TEST_CASE("Username::create accepts the boundary length of exactly 32 characters",
          "[domain][value-object][username]") {
    const std::string boundary(32, 'a');
    const auto name = Username::create(boundary);
    REQUIRE(name.has_value());
    CHECK(name->value().size() == 32);
}

TEST_CASE("Username::create rejects disallowed punctuation",
          "[domain][value-object][username]") {
    CHECK_FALSE(Username::create("user!name").has_value());
    CHECK_FALSE(Username::create("user name").has_value());
    CHECK_FALSE(Username::create("user@name").has_value());
    CHECK_FALSE(Username::create("user.name").has_value());
}

TEST_CASE("Username equality compares underlying values",
          "[domain][value-object][username]") {
    const auto first = Username::create("alice");
    const auto second = Username::create("alice");
    const auto other = Username::create("bob");
    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    REQUIRE(other.has_value());
    CHECK(*first == *second);
    CHECK_FALSE(*first == *other);
}

TEST_CASE("Username::create accepts a single character",
          "[domain][value-object][username]") {
    const auto name = Username::create("a");
    REQUIRE(name.has_value());
    CHECK(name->value() == "a");
}

TEST_CASE("ChannelName::create accepts a valid name", "[domain][value-object][channel-name]") {
    const auto name = ChannelName::create("general");
    REQUIRE(name.has_value());
    CHECK(name->value() == "general");
}

TEST_CASE("ChannelName::create rejects underscores", "[domain][value-object][channel-name]") {
    CHECK_FALSE(ChannelName::create("chan_with_underscore").has_value());
}

TEST_CASE("ChannelName::create rejects an empty string", "[domain][value-object][channel-name]") {
    CHECK_FALSE(ChannelName::create("").has_value());
}

TEST_CASE("ChannelName::create rejects input longer than 64 characters",
          "[domain][value-object][channel-name]") {
    const std::string too_long(65, 'x');
    CHECK_FALSE(ChannelName::create(too_long).has_value());
}

TEST_CASE("ChannelName::create accepts the boundary length of exactly 64 characters",
          "[domain][value-object][channel-name]") {
    const std::string boundary(64, 'x');
    const auto name = ChannelName::create(boundary);
    REQUIRE(name.has_value());
    CHECK(name->value().size() == 64);
}

TEST_CASE("ChannelName::create accepts hyphens and digits",
          "[domain][value-object][channel-name]") {
    const auto name = ChannelName::create("dev-ops-2026");
    REQUIRE(name.has_value());
    CHECK(name->value() == "dev-ops-2026");
}

TEST_CASE("MessageContent::create accepts a normal message",
          "[domain][value-object][message-content]") {
    const auto content = MessageContent::create("hello");
    REQUIRE(content.has_value());
    CHECK(content->value() == "hello");
}

TEST_CASE("MessageContent::create trims surrounding whitespace before validating",
          "[domain][value-object][message-content]") {
    const auto content = MessageContent::create("  hello  ");
    REQUIRE(content.has_value());
    CHECK(content->value() == "hello");
}

TEST_CASE("MessageContent::create rejects whitespace-only input",
          "[domain][value-object][message-content]") {
    CHECK_FALSE(MessageContent::create("   ").has_value());
    CHECK_FALSE(MessageContent::create("\t\n").has_value());
}

TEST_CASE("MessageContent::create accepts the boundary length of exactly 4096 characters",
          "[domain][value-object][message-content]") {
    const std::string boundary(4096, 'm');
    const auto content = MessageContent::create(boundary);
    REQUIRE(content.has_value());
    CHECK(content->value().size() == 4096);
}

TEST_CASE("MessageContent::create rejects input longer than 4096 characters",
          "[domain][value-object][message-content]") {
    const std::string too_long(4097, 'm');
    CHECK_FALSE(MessageContent::create(too_long).has_value());
}

TEST_CASE("MessageContent::create rejects an empty string",
          "[domain][value-object][message-content]") {
    CHECK_FALSE(MessageContent::create("").has_value());
}
