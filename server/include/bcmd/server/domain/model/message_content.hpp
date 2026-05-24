#pragma once

#include "bcmd/shared/result.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace bcmd::server::domain {

// Validated value object. Leading and trailing whitespace are trimmed before
// validation. Trimmed length must be 1-4096 chars; the stored value is the
// trimmed string.
class MessageContent {
public:
    static constexpr std::size_t MIN_LENGTH = 1;
    static constexpr std::size_t MAX_LENGTH = 4096;

    static bcmd::Result<MessageContent> create(std::string_view raw);

    [[nodiscard]] const std::string& value() const noexcept { return value_; }

    bool operator==(const MessageContent&) const = default;

private:
    explicit MessageContent(std::string raw) : value_(std::move(raw)) {}

    std::string value_{};
};

}  // namespace bcmd::server::domain

// NOLINTBEGIN(bugprone-std-namespace-modification)
template <>
struct std::hash<bcmd::server::domain::MessageContent> {
    std::size_t operator()(const bcmd::server::domain::MessageContent& content) const noexcept {
        return std::hash<std::string>{}(content.value());
    }
};
// NOLINTEND(bugprone-std-namespace-modification)
