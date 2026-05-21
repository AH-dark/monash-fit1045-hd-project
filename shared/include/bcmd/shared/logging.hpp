#pragma once

#include <spdlog/spdlog.h>

#include <cstdint>
#include <memory>
#include <string_view>

namespace bcmd {

enum class LogLevel : std::uint8_t {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off,
};

struct LoggingConfig {
    LogLevel level{LogLevel::Info};
    bool colorize{true};
    std::string_view pattern{"[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v"};
};

// Initialise the global spdlog default logger. Call once at program startup.
// Re-invocation replaces the previous default logger.
void init_logging(const LoggingConfig& config = {});

// Returns the named logger, creating one with a colored stdout sink the first
// time it is requested. An empty name returns the current default logger.
std::shared_ptr<spdlog::logger> get_logger(std::string_view name = "bcmd");

}  // namespace bcmd
