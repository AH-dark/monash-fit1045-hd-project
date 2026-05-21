#pragma once

#include "bcmd/server/application/port/i_message_repository.hpp"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace bcmd::server::adapter::persistence {

class PostgresMessageRepository final : public application::port::IMessageRepository {
public:
    bcmd::VoidResult save(const domain::Message& /*message*/) override {
        throw std::runtime_error("not implemented: HD-1a");
    }
    std::vector<domain::Message> recent(const bcmd::ChannelId& /*channel_id*/,
                                        std::uint32_t /*limit*/) override {
        throw std::runtime_error("not implemented: HD-1a");
    }
};

}  // namespace bcmd::server::adapter::persistence
