#pragma once

#include "bcmd/server/domain/model/client_session.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <chrono>
#include <vector>

namespace bcmd::server::application::port {

// Registry for active client sessions.
class IClientRegistry {
public:
    virtual ~IClientRegistry() = default;

    // Returns `Error::ClientNotFound` when the id is unknown.
    virtual bcmd::Result<domain::ClientSession> findById(const bcmd::ClientId& client_id) = 0;
    virtual bcmd::Result<domain::ClientSession> findByUsername(const domain::Username& name) = 0;

    // Looks up the username that was registered for `client_id`, even after the
    // session has been removed or reaped. Returns `Error::ClientNotFound` only
    // when the id was never registered.
    virtual bcmd::Result<domain::Username> lookupUsername(const bcmd::ClientId& client_id) = 0;

    // Returns `Error::ClientAlreadyExists` when the username is taken.
    virtual bcmd::Result<domain::ClientSession> registerClient(domain::Username username) = 0;

    virtual bcmd::VoidResult save(const domain::ClientSession& session) = 0;

    // Returns `Error::ClientNotFound` when the id is unknown.
    virtual bcmd::VoidResult remove(const bcmd::ClientId& client_id) = 0;

    // Updates the last-seen timestamp for the given client. Returns
    // `Error::ClientNotFound` when the id is unknown.
    virtual bcmd::VoidResult touchHeartbeat(const bcmd::ClientId& client_id) = 0;

    // Atomically adds `channel_id` to the session's joined-channel set. Use this
    // instead of read-modify-`save` to avoid clobbering the last-seen timestamp
    // (which `touchHeartbeat` mutates concurrently). Returns
    // `Error::ClientNotFound` when the id is unknown. Idempotent for a channel
    // the client is already in.
    virtual bcmd::VoidResult joinChannelAtomic(const bcmd::ClientId& client_id,
                                               const bcmd::ChannelId& channel_id) = 0;

    // Atomically removes `channel_id` from the session's joined-channel set.
    // Returns `Error::ClientNotFound` when the id is unknown. Idempotent for a
    // channel the client is not in.
    virtual bcmd::VoidResult leaveChannelAtomic(const bcmd::ClientId& client_id,
                                                const bcmd::ChannelId& channel_id) = 0;

    // Returns copies of every session whose last-seen timestamp is older
    // than `deadline`. Callers decide whether to remove them.
    virtual std::vector<domain::ClientSession> collectExpired(
        std::chrono::steady_clock::time_point deadline) = 0;

protected:
    IClientRegistry() = default;
    IClientRegistry(const IClientRegistry&) = default;
    IClientRegistry& operator=(const IClientRegistry&) = default;
    IClientRegistry(IClientRegistry&&) = default;
    IClientRegistry& operator=(IClientRegistry&&) = default;
};

}  // namespace bcmd::server::application::port
