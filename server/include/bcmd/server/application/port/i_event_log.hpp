#pragma once

#include <string_view>

namespace bcmd::server::application::port {

// HD audit-log port. Minimal first-cut interface; richer event types are
// expected once persistent storage is wired in.
class IEventLog {
public:
    virtual ~IEventLog() = default;

    virtual void log(std::string_view event_type, std::string_view details) = 0;

protected:
    IEventLog() = default;
    IEventLog(const IEventLog&) = default;
    IEventLog& operator=(const IEventLog&) = default;
    IEventLog(IEventLog&&) = default;
    IEventLog& operator=(IEventLog&&) = default;
};

}  // namespace bcmd::server::application::port
