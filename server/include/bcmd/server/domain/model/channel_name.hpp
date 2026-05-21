#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace bcmd::server::domain {

// Validated value object. Allowed: 1-64 chars from [a-zA-Z0-9-] (no underscore).
class ChannelName {
public:
    static constexpr std::size_t MIN_LENGTH = 1;
    static constexpr std::size_t MAX_LENGTH = 64;

    static std::optional<ChannelName> create(std::string_view raw);

    const std::string& value() const noexcept { return value_; }

    bool operator==(const ChannelName&) const = default;

private:
    explicit ChannelName(std::string raw) : value_(std::move(raw)) {}

    std::string value_{};
};

}  // namespace bcmd::server::domain

template <>
struct std::hash<bcmd::server::domain::ChannelName> {
    std::size_t operator()(const bcmd::server::domain::ChannelName& name) const noexcept {
        return std::hash<std::string>{}(name.value());
    }
};
