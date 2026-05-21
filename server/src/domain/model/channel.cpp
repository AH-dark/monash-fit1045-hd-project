#include "bcmd/server/domain/model/channel.hpp"

#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <chrono>
#include <expected>
#include <utility>

namespace bcmd::server::domain {

Channel::Channel(bcmd::ChannelId id, ChannelName name)
    : id_(std::move(id)), name_(std::move(name)), created_at_(std::chrono::system_clock::now()) {}

bcmd::VoidResult Channel::addMember(const bcmd::ClientId& client_id) {
    const auto [_, inserted] = members_.insert(client_id);
    if (!inserted) {
        return std::unexpected(bcmd::Error::AlreadyMember);
    }
    return {};
}

bcmd::VoidResult Channel::removeMember(const bcmd::ClientId& client_id) {
    const auto removed = members_.erase(client_id);
    if (removed == 0) {
        return std::unexpected(bcmd::Error::NotAMember);
    }
    return {};
}

bool Channel::hasMember(const bcmd::ClientId& client_id) const noexcept {
    return members_.contains(client_id);
}

}  // namespace bcmd::server::domain
