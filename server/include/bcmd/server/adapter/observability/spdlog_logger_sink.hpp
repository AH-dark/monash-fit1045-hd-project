#pragma once

#include "bcmd/server/application/port/i_event_log.hpp"

#include <spdlog/spdlog.h>

#include <string_view>

namespace bcmd::server::adapter::observability {

class SpdlogEventLogSink final : public application::port::IEventLog {
public:
    void log(std::string_view event_type, std::string_view details) override {
        spdlog::info("[event] type={} details={}", event_type, details);
    }
};

}  // namespace bcmd::server::adapter::observability
