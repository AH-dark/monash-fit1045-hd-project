#include "bcmd/shared/logging.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("init_logging completes without throwing", "[logging][shared]") {
    REQUIRE_NOTHROW(bcmd::init_logging());
}

TEST_CASE("init_logging honours an explicit non-default level", "[logging][shared]") {
    bcmd::LoggingConfig config;
    config.level = bcmd::LogLevel::Warn;
    REQUIRE_NOTHROW(bcmd::init_logging(config));
}

TEST_CASE("get_logger returns the default logger for an empty name", "[logging][shared]") {
    bcmd::init_logging();
    auto logger = bcmd::get_logger("");
    REQUIRE(logger != nullptr);
}

TEST_CASE("get_logger returns the same instance on repeated lookup", "[logging][shared]") {
    bcmd::init_logging();
    auto first = bcmd::get_logger("shared_test_logger");
    auto second = bcmd::get_logger("shared_test_logger");
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    CHECK(first.get() == second.get());
}
