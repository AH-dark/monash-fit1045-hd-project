#include "bcmd/shared/logging.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <string_view>

namespace bcmd {

namespace {

spdlog::level::level_enum to_spdlog_level(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace:
            return spdlog::level::trace;
        case LogLevel::Debug:
            return spdlog::level::debug;
        case LogLevel::Info:
            return spdlog::level::info;
        case LogLevel::Warn:
            return spdlog::level::warn;
        case LogLevel::Error:
            return spdlog::level::err;
        case LogLevel::Critical:
            return spdlog::level::critical;
        case LogLevel::Off:
            return spdlog::level::off;
    }
    return spdlog::level::info;
}

std::shared_ptr<spdlog::logger> make_stdout_logger(std::string name, bool colorize) {
    if (colorize) {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        return std::make_shared<spdlog::logger>(std::move(name), sink);
    }
    auto sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
    return std::make_shared<spdlog::logger>(std::move(name), sink);
}

}  // namespace

void init_logging(const LoggingConfig& config) {
    auto logger = make_stdout_logger("bcmd", config.colorize);
    logger->set_level(to_spdlog_level(config.level));
    logger->set_pattern(std::string(config.pattern));
    spdlog::set_default_logger(std::move(logger));
}

std::shared_ptr<spdlog::logger> get_logger(std::string_view name) {
    if (name.empty()) {
        return spdlog::default_logger();
    }
    const std::string NAME_STR(name);
    if (auto existing = spdlog::get(NAME_STR)) {
        return existing;
    }
    auto logger = make_stdout_logger(NAME_STR, true);
    spdlog::register_logger(logger);
    return logger;
}

}  // namespace bcmd
