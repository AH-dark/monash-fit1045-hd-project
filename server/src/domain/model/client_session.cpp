#include "bcmd/server/domain/model/client_session.hpp"

#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"

#include <chrono>
#include <utility>

namespace bcmd::server::domain {

ClientSession::ClientSession(bcmd::ClientId id, Username username)
    : id_(std::move(id)),
      username_(std::move(username)),
      connected_since_(std::chrono::system_clock::now()),
      last_heartbeat_at_(std::chrono::steady_clock::now()) {}

void ClientSession::touch() noexcept { last_heartbeat_at_ = std::chrono::steady_clock::now(); }

bool ClientSession::isExpired(std::chrono::steady_clock::time_point deadline) const noexcept {
    return last_heartbeat_at_ < deadline;
}

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
