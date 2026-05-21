#include "bcmd/server/adapter/persistence/in_memory_channel_repository.hpp"

#include <expected>
#include <mutex>
#include <shared_mutex>
#include <utility>

#include "bcmd/shared/result.hpp"

namespace bcmd::server::adapter::persistence {

bcmd::Result<domain::Channel> InMemoryChannelRepository::findById(
    const bcmd::ChannelId& channel_id) {
    const std::shared_lock lock(mutex_);
    const auto found = channels_by_id_.find(channel_id.value());
    if (found == channels_by_id_.end()) {
        return std::unexpected(bcmd::Error::ChannelNotFound);
    }
    return found->second;
}

bcmd::Result<domain::Channel> InMemoryChannelRepository::findByName(
    const domain::ChannelName& name) {
    const std::shared_lock lock(mutex_);
    const auto id_found = name_to_id_.find(name.value());
    if (id_found == name_to_id_.end()) {
        return std::unexpected(bcmd::Error::ChannelNotFound);
    }
    const auto channel_found = channels_by_id_.find(id_found->second);
    if (channel_found == channels_by_id_.end()) {
        return std::unexpected(bcmd::Error::ChannelNotFound);
    }
    return channel_found->second;
}

std::vector<domain::Channel> InMemoryChannelRepository::listAll() {
    const std::shared_lock lock(mutex_);
    std::vector<domain::Channel> channels;
    channels.reserve(channels_by_id_.size());
    for (const auto& [_, channel] : channels_by_id_) {
        channels.push_back(channel);
    }
    return channels;
}

bcmd::VoidResult InMemoryChannelRepository::save(const domain::Channel& channel) {
    const std::unique_lock lock(mutex_);
    channels_by_id_.insert_or_assign(channel.id().value(), channel);
    name_to_id_.insert_or_assign(channel.name().value(), channel.id().value());
    return {};
}

bcmd::Result<domain::Channel> InMemoryChannelRepository::create(domain::ChannelName name) {
    const std::unique_lock lock(mutex_);
    if (name_to_id_.contains(name.value())) {
        return std::unexpected(bcmd::Error::ChannelAlreadyExists);
    }

    domain::Channel channel{bcmd::ChannelId::generate(), std::move(name)};
    const auto id = channel.id().value();
    const auto channel_name = channel.name().value();
    channels_by_id_.emplace(id, channel);
    name_to_id_.emplace(channel_name, id);
    return channel;
}

}  // namespace bcmd::server::adapter::persistence
