#pragma once

#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <vector>

namespace bcmd::server::application::port {

// Persistence port for `Channel` aggregates. Adapters supply implementations;
// the application layer depends only on this interface.
class IChannelRepository {
public:
    virtual ~IChannelRepository() = default;

    // Returns `Error::ChannelNotFound` when no channel matches.
    virtual bcmd::Result<domain::Channel> findById(const bcmd::ChannelId& channel_id) = 0;
    virtual bcmd::Result<domain::Channel> findByName(const domain::ChannelName& name) = 0;

    virtual std::vector<domain::Channel> listAll() = 0;

    virtual bcmd::VoidResult save(const domain::Channel& channel) = 0;

    // Creates a fresh channel; returns `Error::ChannelAlreadyExists` if taken.
    virtual bcmd::Result<domain::Channel> create(domain::ChannelName name) = 0;

protected:
    IChannelRepository() = default;
    IChannelRepository(const IChannelRepository&) = default;
    IChannelRepository& operator=(const IChannelRepository&) = default;
    IChannelRepository(IChannelRepository&&) = default;
    IChannelRepository& operator=(IChannelRepository&&) = default;
};

}  // namespace bcmd::server::application::port
