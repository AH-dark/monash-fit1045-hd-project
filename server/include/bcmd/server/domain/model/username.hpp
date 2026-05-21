#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace bcmd::server::domain {

// Validated value object. Allowed: 1-32 chars from [a-zA-Z0-9_-].
// Construction is private; use the `create` factory for safe validation.
class Username {
public:
    static constexpr std::size_t MIN_LENGTH = 1;
    static constexpr std::size_t MAX_LENGTH = 32;

    static std::optional<Username> create(std::string_view raw);

    [[nodiscard]] const std::string& value() const noexcept { return value_; }

    bool operator==(const Username&) const = default;

private:
    explicit Username(std::string raw) : value_(std::move(raw)) {}

    std::string value_{};
};

}  // namespace bcmd::server::domain

template <>
struct std::hash<bcmd::server::domain::Username> {  // NOLINT(bugprone-std-namespace-modification)
    std::size_t operator()(const bcmd::server::domain::Username& name) const noexcept {
        return std::hash<std::string>{}(name.value());
    }
};
