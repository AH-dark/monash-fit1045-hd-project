// AI Generated.
#pragma once

#include <format>
#include <string>
#include <utility>

// Thin wrapper around std::print / std::println so callers compile on
// libstdc++ versions that ship <format> but not yet <print>.
#if defined(__cpp_lib_print) && __cpp_lib_print >= 202207L

#include <print>

namespace bcmd {
using std::print;
using std::println;
}  // namespace bcmd

#else

#include <cstdio>

namespace bcmd {

template <class... Args>
void print(std::format_string<Args...> fmt, Args&&... args) {
    std::fputs(std::format(fmt, std::forward<Args>(args)...).c_str(), stdout);
}

template <class... Args>
void println(std::format_string<Args...> fmt, Args&&... args) {
    auto formatted = std::format(fmt, std::forward<Args>(args)...);
    formatted.push_back('\n');
    std::fputs(formatted.c_str(), stdout);
}

inline void println() { std::putchar('\n'); }

}  // namespace bcmd

#endif
