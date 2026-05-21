#include "bcmd/server/domain/model/message_content.hpp"

#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace bcmd::server::domain {

namespace {

bool is_whitespace(char character) noexcept {
    const auto byte_value = static_cast<unsigned char>(character);
    return std::isspace(byte_value) != 0;
}

std::string_view trim(std::string_view raw) noexcept {
    while (!raw.empty() && is_whitespace(raw.front())) {
        raw.remove_prefix(1);
    }
    while (!raw.empty() && is_whitespace(raw.back())) {
        raw.remove_suffix(1);
    }
    return raw;
}

}  // namespace

std::optional<MessageContent> MessageContent::create(std::string_view raw) {
    const auto trimmed = trim(raw);
    if (trimmed.size() < MIN_LENGTH || trimmed.size() > MAX_LENGTH) {
        return std::nullopt;
    }
    return MessageContent{std::string(trimmed)};
}

}  // namespace bcmd::server::domain
