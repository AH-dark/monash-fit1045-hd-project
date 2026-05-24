#include "bcmd/client/adapter/cli/command_parser.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string_view>

namespace {

using bcmd::client::adapter::cli::CommandType;
using bcmd::client::adapter::cli::parseCommand;

void expect_command(std::string_view input, CommandType type, std::string_view arg) {
    const auto parsed = parseCommand(input);

    CHECK(parsed.type == type);
    CHECK(parsed.arg == arg);
}

}  // namespace

// NOLINTBEGIN(misc-use-anonymous-namespace, readability-identifier-naming)

TEST_CASE("parseCommand parses /join lobby", "[client-adapter]") {
    expect_command("/join lobby", CommandType::Join, "lobby");
}

TEST_CASE("parseCommand parses /create dev", "[client-adapter]") {
    expect_command("/create dev", CommandType::Create, "dev");
}

TEST_CASE("parseCommand parses /leave", "[client-adapter]") {
    expect_command("/leave", CommandType::Leave, "");
}

TEST_CASE("parseCommand parses /list", "[client-adapter]") {
    expect_command("/list", CommandType::List, "");
}

TEST_CASE("parseCommand parses /quit", "[client-adapter]") {
    expect_command("/quit", CommandType::Quit, "");
}

TEST_CASE("parseCommand returns None for ordinary text", "[client-adapter]") {
    expect_command("hello", CommandType::None, "");
}

TEST_CASE("parseCommand returns Unknown for unrecognized slash input", "[client-adapter]") {
    expect_command("/foo", CommandType::Unknown, "/foo");
}

TEST_CASE("parseCommand returns Unknown for bare slash", "[client-adapter]") {
    expect_command("/", CommandType::Unknown, "/");
}

TEST_CASE("parseCommand is case sensitive", "[client-adapter]") {
    expect_command("/JOIN room", CommandType::Unknown, "/JOIN room");
}

TEST_CASE("parseCommand trims leading and trailing whitespace", "[client-adapter]") {
    expect_command("  /foo  ", CommandType::Unknown, "/foo");
}

TEST_CASE("parseCommand treats /join without argument as unknown", "[client-adapter]") {
    expect_command("/join", CommandType::Unknown, "/join");
}

TEST_CASE("parseCommand ignores non-leading slash text", "[client-adapter]") {
    expect_command("hello /world", CommandType::None, "");
}

TEST_CASE("parseCommand returns None for empty input", "[client-adapter]") {
    expect_command("", CommandType::None, "");
}

TEST_CASE("parseCommand returns None for whitespace-only input", "[client-adapter]") {
    expect_command("   ", CommandType::None, "");
}

// NOLINTEND(misc-use-anonymous-namespace, readability-identifier-naming)
