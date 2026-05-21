#pragma once

#include <cstdint>
#include <vector>

#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

namespace bcmd::server::application::port {

// Persistence port for historical messages.
class IMessageRepository {
public:
    virtual ~IMessageRepository() = default;

    virtual bcmd::VoidResult save(const domain::Message& message) = 0;

    // Up to `count` most recent messages for the channel in chronological order
    // (oldest first, newest last). Returns an empty vector when none exist.
    virtual std::vector<domain::Message> recent(const bcmd::ChannelId& channel_id,
                                                std::uint32_t count) = 0;

protected:
    IMessageRepository() = default;
    IMessageRepository(const IMessageRepository&) = default;
    IMessageRepository& operator=(const IMessageRepository&) = default;
    IMessageRepository(IMessageRepository&&) = default;
    IMessageRepository& operator=(IMessageRepository&&) = default;
};

}  // namespace bcmd::server::application::port
