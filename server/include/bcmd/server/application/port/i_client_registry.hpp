#pragma once

#include "bcmd/server/domain/model/client_session.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

namespace bcmd::server::application::port {

// Registry for active client sessions.
class IClientRegistry {
public:
    virtual ~IClientRegistry() = default;

    // Returns `Error::ClientNotFound` when the id is unknown.
    virtual bcmd::Result<domain::ClientSession> findById(const bcmd::ClientId& client_id) = 0;
    virtual bcmd::Result<domain::ClientSession> findByUsername(const domain::Username& name) = 0;

    // Returns `Error::ClientAlreadyExists` when the username is taken.
    virtual bcmd::Result<domain::ClientSession> registerClient(domain::Username username) = 0;

    virtual bcmd::VoidResult save(const domain::ClientSession& session) = 0;

    // Returns `Error::ClientNotFound` when the id is unknown.
    virtual bcmd::VoidResult remove(const bcmd::ClientId& client_id) = 0;

protected:
    IClientRegistry() = default;
    IClientRegistry(const IClientRegistry&) = default;
    IClientRegistry& operator=(const IClientRegistry&) = default;
    IClientRegistry(IClientRegistry&&) = default;
    IClientRegistry& operator=(IClientRegistry&&) = default;
};

}  // namespace bcmd::server::application::port
