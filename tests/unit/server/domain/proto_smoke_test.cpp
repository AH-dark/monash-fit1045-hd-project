#include <catch2/catch_test_macros.hpp>

#include <string>

#include "bcmd/v1/broadcast.pb.h"

TEST_CASE("ConnectRequest serializes correctly", "[proto]") {
    bcmd::v1::ConnectRequest req;
    req.set_username("alice");

    std::string serialized;
    REQUIRE(req.SerializeToString(&serialized));
    REQUIRE(serialized.size() > 0);
    CHECK(req.username() == "alice");
}

TEST_CASE("ChannelEvent oneof compiles", "[proto]") {
    bcmd::v1::ChannelEvent evt;
    evt.mutable_message()->set_content("hello");
    REQUIRE(evt.has_message());
    CHECK(evt.message().content() == "hello");
}
