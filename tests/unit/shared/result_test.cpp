#include "bcmd/shared/result.hpp"

#include <catch2/catch_test_macros.hpp>

#include <expected>
#include <string_view>

TEST_CASE("error_message yields a non-empty description for every enum value", "[result][shared]") {
    CHECK_FALSE(bcmd::error_message(bcmd::Error::ChannelNotFound).empty());
    CHECK_FALSE(bcmd::error_message(bcmd::Error::ChannelAlreadyExists).empty());
    CHECK_FALSE(bcmd::error_message(bcmd::Error::ClientNotFound).empty());
    CHECK_FALSE(bcmd::error_message(bcmd::Error::ClientAlreadyExists).empty());
    CHECK_FALSE(bcmd::error_message(bcmd::Error::AlreadyMember).empty());
    CHECK_FALSE(bcmd::error_message(bcmd::Error::NotAMember).empty());
    CHECK_FALSE(bcmd::error_message(bcmd::Error::MessageTooLong).empty());
    CHECK_FALSE(bcmd::error_message(bcmd::Error::MessageEmpty).empty());
    CHECK_FALSE(bcmd::error_message(bcmd::Error::InvalidUsername).empty());
    CHECK_FALSE(bcmd::error_message(bcmd::Error::InvalidChannelName).empty());
    CHECK_FALSE(bcmd::error_message(bcmd::Error::StorageError).empty());
    CHECK_FALSE(bcmd::error_message(bcmd::Error::NetworkError).empty());
    CHECK_FALSE(bcmd::error_message(bcmd::Error::NotImplemented).empty());
}

TEST_CASE("error_message returns stable text for ChannelNotFound", "[result][shared]") {
    CHECK(bcmd::error_message(bcmd::Error::ChannelNotFound) ==
          std::string_view{"channel not found"});
}

TEST_CASE("Result<int> carries a success value", "[result][shared]") {
    bcmd::Result<int> result{42};
    REQUIRE(result.has_value());
    CHECK(result.value() == 42);
}

TEST_CASE("Result<int> propagates an error", "[result][shared]") {
    bcmd::Result<int> result{std::unexpected(bcmd::Error::ChannelNotFound)};
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::ChannelNotFound);
}

TEST_CASE("VoidResult supports both success and failure", "[result][shared]") {
    bcmd::VoidResult ok{};
    CHECK(ok.has_value());

    bcmd::VoidResult fail{std::unexpected(bcmd::Error::NotImplemented)};
    REQUIRE_FALSE(fail.has_value());
    CHECK(fail.error() == bcmd::Error::NotImplemented);
}
