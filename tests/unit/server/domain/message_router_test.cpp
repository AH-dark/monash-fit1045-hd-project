#include "bcmd/server/domain/service/message_router.hpp"

#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/server/domain/model/message_content.hpp"
#include "bcmd/shared/ids.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <vector>

namespace {

using bcmd::server::domain::Channel;
using bcmd::server::domain::ChannelName;
using bcmd::server::domain::EchoPolicy;
using bcmd::server::domain::Message;
using bcmd::server::domain::MessageContent;
using bcmd::server::domain::MessageRouter;

ChannelName make_name(const char* raw) {
    auto name = ChannelName::create(raw);
    REQUIRE(name.has_value());
    return *name;
}

MessageContent make_content(const char* raw) {
    auto content = MessageContent::create(raw);
    REQUIRE(content.has_value());
    return content.value();
}

bool contains(const std::vector<bcmd::ClientId>& haystack, const bcmd::ClientId& needle) {
    return std::ranges::find(haystack, needle) != haystack.end();
}

}  // namespace

TEST_CASE("MessageRouter with ExcludeSender returns all members except the sender",
          "[domain][service][router]") {
    Channel channel{bcmd::ChannelId::generate(), make_name("general")};
    const auto sender_id = bcmd::ClientId::generate();
    const auto other_a = bcmd::ClientId::generate();
    const auto other_b = bcmd::ClientId::generate();
    REQUIRE(channel.addMember(sender_id).has_value());
    REQUIRE(channel.addMember(other_a).has_value());
    REQUIRE(channel.addMember(other_b).has_value());

    const Message message{
        bcmd::MessageId::generate(),
        sender_id,
        channel.id(),
        make_content("hello"),
    };

    const auto recipients =
        MessageRouter::recipientsFor(channel, message, EchoPolicy::ExcludeSender);

    CHECK(recipients.size() == 2);
    CHECK(contains(recipients, other_a));
    CHECK(contains(recipients, other_b));
    CHECK_FALSE(contains(recipients, sender_id));
}

TEST_CASE("MessageRouter with IncludeSender returns every member", "[domain][service][router]") {
    Channel channel{bcmd::ChannelId::generate(), make_name("general")};
    const auto sender_id = bcmd::ClientId::generate();
    const auto other_a = bcmd::ClientId::generate();
    const auto other_b = bcmd::ClientId::generate();
    REQUIRE(channel.addMember(sender_id).has_value());
    REQUIRE(channel.addMember(other_a).has_value());
    REQUIRE(channel.addMember(other_b).has_value());

    const Message message{
        bcmd::MessageId::generate(),
        sender_id,
        channel.id(),
        make_content("hello"),
    };

    const auto recipients =
        MessageRouter::recipientsFor(channel, message, EchoPolicy::IncludeSender);

    CHECK(recipients.size() == 3);
    CHECK(contains(recipients, sender_id));
}

TEST_CASE("MessageRouter returns all members when the sender is not in the channel",
          "[domain][service][router]") {
    Channel channel{bcmd::ChannelId::generate(), make_name("general")};
    const auto member_a = bcmd::ClientId::generate();
    const auto member_b = bcmd::ClientId::generate();
    const auto member_c = bcmd::ClientId::generate();
    REQUIRE(channel.addMember(member_a).has_value());
    REQUIRE(channel.addMember(member_b).has_value());
    REQUIRE(channel.addMember(member_c).has_value());

    const auto outsider_id = bcmd::ClientId::generate();
    const Message message{
        bcmd::MessageId::generate(),
        outsider_id,
        channel.id(),
        make_content("hello"),
    };

    const auto recipients =
        MessageRouter::recipientsFor(channel, message, EchoPolicy::ExcludeSender);

    CHECK(recipients.size() == 3);
}

TEST_CASE(
    "MessageRouter on a single-member channel where sender is the only member returns no "
    "recipients",
    "[domain][service][router]") {
    Channel channel{bcmd::ChannelId::generate(), make_name("general")};
    const auto sender_id = bcmd::ClientId::generate();
    REQUIRE(channel.addMember(sender_id).has_value());

    const Message message{
        bcmd::MessageId::generate(),
        sender_id,
        channel.id(),
        make_content("hello"),
    };

    const auto recipients =
        MessageRouter::recipientsFor(channel, message, EchoPolicy::ExcludeSender);

    CHECK(recipients.empty());
}

TEST_CASE("MessageRouter on an empty channel returns no recipients", "[domain][service][router]") {
    Channel channel{bcmd::ChannelId::generate(), make_name("general")};

    const Message message{
        bcmd::MessageId::generate(),
        bcmd::ClientId::generate(),
        channel.id(),
        make_content("hello"),
    };

    const auto excluded = MessageRouter::recipientsFor(channel, message, EchoPolicy::ExcludeSender);
    const auto included = MessageRouter::recipientsFor(channel, message, EchoPolicy::IncludeSender);

    CHECK(excluded.empty());
    CHECK(included.empty());
}

TEST_CASE("MessageRouter defaults to ExcludeSender", "[domain][service][router]") {
    Channel channel{bcmd::ChannelId::generate(), make_name("general")};
    const auto sender_id = bcmd::ClientId::generate();
    const auto other_id = bcmd::ClientId::generate();
    REQUIRE(channel.addMember(sender_id).has_value());
    REQUIRE(channel.addMember(other_id).has_value());

    const Message message{
        bcmd::MessageId::generate(),
        sender_id,
        channel.id(),
        make_content("hello"),
    };

    const auto recipients = MessageRouter::recipientsFor(channel, message);
    CHECK(recipients.size() == 1);
    CHECK(recipients.front() == other_id);
}
