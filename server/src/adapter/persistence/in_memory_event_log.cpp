#include "bcmd/server/adapter/persistence/in_memory_event_log.hpp"

#include <mutex>
#include <shared_mutex>
#include <string>

namespace bcmd::server::adapter::persistence {

void InMemoryEventLog::log(std::string_view event_type, std::string_view details) {
    const std::unique_lock lock(mutex_);
    entries_.emplace_back(std::string(event_type), std::string(details));
}

std::vector<InMemoryEventLog::Entry> InMemoryEventLog::entries() const {
    const std::shared_lock lock(mutex_);
    return entries_;
}

}  // namespace bcmd::server::adapter::persistence
