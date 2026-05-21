#pragma once

#include "bcmd/server/application/port/i_message_repository.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace bcmd::server::adapter::persistence {

class MongoMessageRepository final : public application::port::IMessageRepository {
public:
    bcmd::VoidResult save(const domain::Message& /*message*/) override {
        throw std::runtime_error("not implemented: mongo");
    }
    std::vector<domain::Message> recent(const bcmd::ChannelId& /*channel_id*/,
                                        std::uint32_t /*limit*/) override {
        throw std::runtime_error("not implemented: mongo");
    }
};

}  // namespace bcmd::server::adapter::persistence
