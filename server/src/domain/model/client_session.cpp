#include "bcmd/server/domain/model/client_session.hpp"

#include <chrono>
#include <utility>

namespace bcmd::server::domain {

ClientSession::ClientSession(bcmd::ClientId id, Username username)
    : id_(std::move(id)),
      username_(std::move(username)),
      connected_since_(std::chrono::system_clock::now()) {}

void ClientSession::joinChannel(const bcmd::ChannelId& channel_id) {
    joined_channels_.insert(channel_id);
}

void ClientSession::leaveChannel(const bcmd::ChannelId& channel_id) {
    joined_channels_.erase(channel_id);
}

bool ClientSession::isInChannel(const bcmd::ChannelId& channel_id) const noexcept {
    return joined_channels_.contains(channel_id);
}

}  // namespace bcmd::server::domain
