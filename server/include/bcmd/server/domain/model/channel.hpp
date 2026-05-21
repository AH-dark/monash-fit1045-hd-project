#pragma once

#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <chrono>
#include <cstddef>
#include <unordered_set>

namespace bcmd::server::domain {

// Aggregate root for a chat channel. Owns its membership set.
// Channel-name uniqueness is enforced at the repository level, not here.
class Channel {
public:
    Channel(bcmd::ChannelId id, ChannelName name);

    [[nodiscard]] const bcmd::ChannelId& id() const noexcept { return id_; }
    [[nodiscard]] const ChannelName& name() const noexcept { return name_; }
    [[nodiscard]] std::size_t memberCount() const noexcept { return members_.size(); }
    [[nodiscard]] const std::unordered_set<bcmd::ClientId>& members() const noexcept {
        return members_;
    }
    [[nodiscard]] std::chrono::system_clock::time_point createdAt() const noexcept {
        return created_at_;
    }

    // Idempotent on the caller's side: returns `Error::AlreadyMember` when the
    // client is already in the channel so the caller can choose to ignore.
    bcmd::VoidResult addMember(const bcmd::ClientId& client_id);
    bcmd::VoidResult removeMember(const bcmd::ClientId& client_id);
    [[nodiscard]] bool hasMember(const bcmd::ClientId& client_id) const noexcept;

private:
    bcmd::ChannelId id_;
    ChannelName name_;
    std::unordered_set<bcmd::ClientId> members_{};
    std::chrono::system_clock::time_point created_at_{};
};

}  // namespace bcmd::server::domain
