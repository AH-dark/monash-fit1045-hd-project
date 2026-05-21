#include "bcmd/shared/ids.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <unordered_set>

TEST_CASE("ChannelId::generate produces distinct values", "[ids][shared]") {
    const auto first = bcmd::ChannelId::generate();
    const auto second = bcmd::ChannelId::generate();
    REQUIRE(first != second);
    CHECK_FALSE(first.value().empty());
    CHECK_FALSE(second.value().empty());
}

TEST_CASE("ChannelId roundtrips through to_string / parse", "[ids][shared]") {
    const auto original = bcmd::ChannelId::generate();
    const auto parsed = bcmd::ChannelId::parse(original.to_string());
    REQUIRE(parsed.has_value());
    CHECK(*parsed == original);
}

TEST_CASE("ChannelId::parse rejects malformed input", "[ids][shared]") {
    CHECK_FALSE(bcmd::ChannelId::parse("not-a-uuid").has_value());
    CHECK_FALSE(bcmd::ChannelId::parse("").has_value());
    CHECK_FALSE(bcmd::ChannelId::parse("00000000-0000-0000-0000-000000000000").has_value());
}

TEST_CASE("MessageId is usable as an unordered_set key", "[ids][shared]") {
    std::unordered_set<bcmd::MessageId> seen;
    for (int i = 0; i < 32; ++i) {
        seen.insert(bcmd::MessageId::generate());
    }
    CHECK(seen.size() == 32);
}

TEST_CASE("ClientId and ChannelId values do not collide trivially", "[ids][shared]") {
    const auto channel = bcmd::ChannelId::generate();
    const auto client = bcmd::ClientId::generate();
    CHECK(channel.to_string() != client.to_string());
}
