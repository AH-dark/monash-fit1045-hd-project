#include "bcmd/server/domain/model/message_content.hpp"

#include "bcmd/shared/result.hpp"

#include <cctype>
#include <expected>
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

bcmd::Result<MessageContent> MessageContent::create(std::string_view raw) {
    const auto trimmed = trim(raw);
    if (trimmed.empty()) {
        return std::unexpected(bcmd::Error::MessageEmpty);
    }
    if (trimmed.size() > MAX_LENGTH) {
        return std::unexpected(bcmd::Error::MessageTooLong);
    }
    if (trimmed.front() == '/') {
        return std::unexpected(bcmd::Error::MessageInvalidPrefix);
    }
    return MessageContent{std::string(trimmed)};
}

}  // namespace bcmd::server::domain
