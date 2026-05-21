#pragma once

#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"

#include <chrono>
#include <unordered_set>

namespace bcmd::server::domain {

class ClientSession {
public:
    ClientSession(bcmd::ClientId id, Username username);

    const bcmd::ClientId& id() const noexcept { return id_; }
    const Username& username() const noexcept { return username_; }
    const std::unordered_set<bcmd::ChannelId>& joinedChannels() const noexcept {
        return joined_channels_;
    }
    std::chrono::system_clock::time_point connectedSince() const noexcept {
        return connected_since_;
    }

    void joinChannel(const bcmd::ChannelId& channel_id);
    void leaveChannel(const bcmd::ChannelId& channel_id);
    bool isInChannel(const bcmd::ChannelId& channel_id) const noexcept;

private:
    bcmd::ClientId id_;
    Username username_;
    std::unordered_set<bcmd::ChannelId> joined_channels_;
    std::chrono::system_clock::time_point connected_since_;
};

}  // namespace bcmd::server::domain
