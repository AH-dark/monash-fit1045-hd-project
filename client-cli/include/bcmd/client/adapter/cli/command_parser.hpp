#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace bcmd::client::adapter::cli {

// NOLINTBEGIN(readability-identifier-naming)
enum class CommandType : std::uint8_t { Join, Leave, List, Quit, None };

struct ParsedCommand {
    CommandType type{CommandType::None};
    std::string arg;
};

inline ParsedCommand parseCommand(std::string_view input) {
    constexpr std::string_view join_prefix{"/join "};

    if (input.starts_with(join_prefix)) {
        return ParsedCommand{.type = CommandType::Join,
                             .arg = std::string{input.substr(join_prefix.size())}};
    }
    if (input == "/leave") {
        return ParsedCommand{.type = CommandType::Leave};
    }
    if (input == "/list") {
        return ParsedCommand{.type = CommandType::List};
    }
    if (input == "/quit") {
        return ParsedCommand{.type = CommandType::Quit};
    }
    return ParsedCommand{};
}

// NOLINTEND(readability-identifier-naming)

}  // namespace bcmd::client::adapter::cli
