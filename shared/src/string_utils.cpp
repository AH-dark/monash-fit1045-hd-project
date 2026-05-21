#include "bcmd/shared/string_utils.hpp"

#include <cctype>
#include <string>
#include <string_view>

namespace bcmd {

namespace {

bool is_ascii_space(char character) noexcept {
    return std::isspace(static_cast<unsigned char>(character)) != 0;
}

}  // namespace

std::string_view trim(std::string_view input) noexcept {
    while (!input.empty() && is_ascii_space(input.front())) {
        input.remove_prefix(1);
    }
    while (!input.empty() && is_ascii_space(input.back())) {
        input.remove_suffix(1);
    }
    return input;
}

std::string trim_copy(std::string_view input) { return std::string{trim(input)}; }

}  // namespace bcmd
