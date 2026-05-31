#include "bcmd/server/adapter/persistence/in_memory_message_repository.hpp"

#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <vector>

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

std::vector<domain::Message> InMemoryMessageRepository::recent(const bcmd::ChannelId& channel_id,
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

std::vector<domain::Message> InMemoryMessageRepository::listBefore(
    const bcmd::ChannelId& channel_id, const bcmd::MessageId& before_message_id,
    std::uint32_t count) {
    const std::shared_lock lock(mutex_);
    const auto found = buffers_.find(channel_id.value());
    if (found == buffers_.end() || count == 0) {
        return {};
    }

    const auto& buffer = found->second;
    const auto cursor = std::ranges::find_if(
        buffer, [&](const domain::Message& message) { return message.id() == before_message_id; });
    if (cursor == buffer.end()) {
        return {};
    }
    const auto distance = static_cast<std::size_t>(std::distance(buffer.begin(), cursor));
    const auto take = std::min<std::size_t>(count, distance);
    return {cursor - static_cast<std::ptrdiff_t>(take), cursor};
}

}  // namespace bcmd::server::adapter::persistence
