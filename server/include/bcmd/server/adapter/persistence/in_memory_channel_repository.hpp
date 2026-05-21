#pragma once

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "bcmd/server/application/port/i_channel_repository.hpp"

namespace bcmd::server::adapter::persistence {

class InMemoryChannelRepository final : public application::port::IChannelRepository {
public:
    bcmd::Result<domain::Channel> findById(const bcmd::ChannelId& channel_id) override;
    bcmd::Result<domain::Channel> findByName(const domain::ChannelName& name) override;
    std::vector<domain::Channel> listAll() override;
    bcmd::VoidResult save(const domain::Channel& channel) override;
    bcmd::Result<domain::Channel> create(domain::ChannelName name) override;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, domain::Channel> channels_by_id_;
    std::unordered_map<std::string, std::string> name_to_id_;
};

}  // namespace bcmd::server::adapter::persistence
