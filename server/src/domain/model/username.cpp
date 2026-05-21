#include "bcmd/server/domain/model/username.hpp"

#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace bcmd::server::domain {

namespace {

bool is_allowed_char(char character) noexcept {
    const auto byte_value = static_cast<unsigned char>(character);
    return std::isalnum(byte_value) != 0 || character == '_' || character == '-';
}

}  // namespace

std::optional<Username> Username::create(std::string_view raw) {
    if (raw.size() < MIN_LENGTH || raw.size() > MAX_LENGTH) {
        return std::nullopt;
    }
    for (const char character : raw) {
        if (!is_allowed_char(character)) {
            return std::nullopt;
        }
    }
    return Username{std::string(raw)};
}

}  // namespace bcmd::server::domain
