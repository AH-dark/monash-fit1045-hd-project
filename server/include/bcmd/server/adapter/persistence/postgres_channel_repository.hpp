#pragma once

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <stdexcept>
#include <vector>

namespace bcmd::server::adapter::persistence {

class PostgresChannelRepository final : public application::port::IChannelRepository {
public:
    bcmd::Result<domain::Channel> findById(const bcmd::ChannelId& /*id*/) override {
        throw std::runtime_error("not implemented: postgres");
    }
    bcmd::Result<domain::Channel> findByName(const domain::ChannelName& /*name*/) override {
        throw std::runtime_error("not implemented: postgres");
    }
    std::vector<domain::Channel> listAll() override {
        throw std::runtime_error("not implemented: postgres");
    }
    bcmd::VoidResult save(const domain::Channel& /*channel*/) override {
        throw std::runtime_error("not implemented: postgres");
    }
    bcmd::Result<domain::Channel> create(domain::ChannelName /*name*/) override {
        throw std::runtime_error("not implemented: postgres");
    }
};

}  // namespace bcmd::server::adapter::persistence
