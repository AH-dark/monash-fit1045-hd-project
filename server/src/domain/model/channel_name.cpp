#include "bcmd/server/domain/model/channel_name.hpp"

#include <cctype>
#include <string>
#include <string_view>

namespace bcmd::server::domain {

namespace {

bool is_allowed_char(char character) noexcept {
    const auto byte_value = static_cast<unsigned char>(character);
    return std::isalnum(byte_value) != 0 || character == '-';
}

}  // namespace

std::optional<ChannelName> ChannelName::create(std::string_view raw) {
    if (raw.size() < MIN_LENGTH || raw.size() > MAX_LENGTH) {
        return std::nullopt;
    }
    for (const char character : raw) {
        if (!is_allowed_char(character)) {
            return std::nullopt;
        }
    }
    return ChannelName{std::string(raw)};
}

}  // namespace bcmd::server::domain
