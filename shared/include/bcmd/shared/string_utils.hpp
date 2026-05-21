#pragma once

#include <string>
#include <string_view>

namespace bcmd {

[[nodiscard]] std::string_view trim(std::string_view input) noexcept;

[[nodiscard]] std::string trim_copy(std::string_view input);

}  // namespace bcmd
