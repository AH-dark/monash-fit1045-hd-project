#pragma once

#include "bcmd/server/application/port/i_client_registry.hpp"

#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace bcmd::server::adapter::persistence {

class InMemoryClientRegistry final : public application::port::IClientRegistry {
public:
    bcmd::Result<domain::ClientSession> findById(const bcmd::ClientId& client_id) override;
    bcmd::Result<domain::ClientSession> findByUsername(const domain::Username& name) override;
    bcmd::Result<domain::ClientSession> registerClient(domain::Username username) override;
    bcmd::VoidResult save(const domain::ClientSession& session) override;
    bcmd::VoidResult remove(const bcmd::ClientId& client_id) override;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, domain::ClientSession> clients_by_id_;
    std::unordered_map<std::string, std::string> username_to_id_;
};

}  // namespace bcmd::server::adapter::persistence
