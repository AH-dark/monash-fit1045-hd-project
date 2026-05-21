#include "bcmd/server/domain/model/message_content.hpp"

#include <cctype>
#include <string>
#include <string_view>

namespace bcmd::server::domain {

namespace {

bool is_whitespace(char character) noexcept {
    const auto byte_value = static_cast<unsigned char>(character);
    return std::isspace(byte_value) != 0;
}

std::string_view trim(std::string_view raw) noexcept {
    std::size_t start = 0;
    while (start < raw.size() && is_whitespace(raw[start])) {
        ++start;
    }
    std::size_t end = raw.size();
    while (end > start && is_whitespace(raw[end - 1])) {
        --end;
    }
    return raw.substr(start, end - start);
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
