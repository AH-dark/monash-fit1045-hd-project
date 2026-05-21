#pragma once

#include "bcmd/shared/string_utils.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace bcmd::client::adapter::cli {

// NOLINTBEGIN(readability-identifier-naming)
enum class CommandType : std::uint8_t { Join, Create, Leave, List, Quit, None };

struct ParsedCommand {
    CommandType type{CommandType::None};
    std::string arg{};
};

inline ParsedCommand parseCommand(std::string_view input) {
    constexpr std::string_view join_prefix{"/join "};
    constexpr std::string_view create_prefix{"/create "};

    const auto trimmed = bcmd::trim(input);

    if (trimmed.starts_with(join_prefix)) {
        return ParsedCommand{
            .type = CommandType::Join,
            .arg = bcmd::trim_copy(trimmed.substr(join_prefix.size())),
        };
    }
    if (trimmed.starts_with(create_prefix)) {
        return ParsedCommand{
            .type = CommandType::Create,
            .arg = bcmd::trim_copy(trimmed.substr(create_prefix.size())),
        };
    }
    if (trimmed == "/leave") {
        return ParsedCommand{.type = CommandType::Leave, .arg = {}};
    }
    if (trimmed == "/list") {
        return ParsedCommand{.type = CommandType::List, .arg = {}};
    }
    if (trimmed == "/quit") {
        return ParsedCommand{.type = CommandType::Quit, .arg = {}};
    }
    return ParsedCommand{};
}

// NOLINTEND(readability-identifier-naming)

}  // namespace bcmd::client::adapter::cli
