#include "bcmd/server/adapter/persistence/in_memory_message_repository.hpp"

#include <algorithm>
#include <cstddef>
#include <mutex>
#include <shared_mutex>

namespace bcmd::server::adapter::persistence {

InMemoryMessageRepository::InMemoryMessageRepository(std::uint32_t cap)
    : cap_(cap == 0 ? kDefaultCap : cap) {}

bcmd::VoidResult InMemoryMessageRepository::save(const domain::Message& message) {
    const std::unique_lock lock(mutex_);
    auto& buffer = buffers_[message.channelId().value()];
    buffer.push_back(message);
    while (buffer.size() > cap_) {
        buffer.pop_front();
    }
    return {};
}

std::vector<domain::Message> InMemoryMessageRepository::recent(
    const bcmd::ChannelId& channel_id,
    std::uint32_t count) {
    const std::shared_lock lock(mutex_);
    const auto found = buffers_.find(channel_id.value());
    if (found == buffers_.end() || count == 0) {
        return {};
    }

    const auto& buffer = found->second;
    const auto take = std::min<std::size_t>(count, buffer.size());
    return {buffer.end() - static_cast<std::ptrdiff_t>(take), buffer.end()};
}

}  // namespace bcmd::server::adapter::persistence
