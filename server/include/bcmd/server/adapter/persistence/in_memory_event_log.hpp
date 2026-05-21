#pragma once

#include "bcmd/server/application/port/i_event_log.hpp"

#include <shared_mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace bcmd::server::adapter::persistence {

class InMemoryEventLog final : public application::port::IEventLog {
public:
    using Entry = std::pair<std::string, std::string>;

    void log(std::string_view event_type, std::string_view details) override;
    std::vector<Entry> entries() const;

private:
    mutable std::shared_mutex mutex_;
    std::vector<Entry> entries_;
};

}  // namespace bcmd::server::adapter::persistence
