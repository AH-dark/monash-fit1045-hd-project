#pragma once

#include "bcmd/server/application/port/i_channel_repository.hpp"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace bcmd::server::adapter::persistence {

class MongoChannelRepository final : public application::port::IChannelRepository {
public:
    bcmd::Result<domain::Channel> findById(const bcmd::ChannelId& /*id*/) override {
        throw std::runtime_error("not implemented: HD-1a");
    }
    bcmd::Result<domain::Channel> findByName(const domain::ChannelName& /*name*/) override {
        throw std::runtime_error("not implemented: HD-1a");
    }
    std::vector<domain::Channel> listAll() override {
        throw std::runtime_error("not implemented: HD-1a");
    }
    bcmd::VoidResult save(const domain::Channel& /*channel*/) override {
        throw std::runtime_error("not implemented: HD-1a");
    }
    bcmd::Result<domain::Channel> create(domain::ChannelName /*name*/) override {
        throw std::runtime_error("not implemented: HD-1a");
    }
};

}  // namespace bcmd::server::adapter::persistence
