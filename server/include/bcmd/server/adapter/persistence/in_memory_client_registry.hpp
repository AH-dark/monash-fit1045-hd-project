#pragma once

#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/domain/model/client_session.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <chrono>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace bcmd::server::adapter::persistence {

class InMemoryClientRegistry final : public application::port::IClientRegistry {
public:
    bcmd::Result<domain::ClientSession> findById(const bcmd::ClientId& client_id) override;
    bcmd::Result<domain::ClientSession> findByUsername(const domain::Username& name) override;
    bcmd::Result<domain::Username> lookupUsername(const bcmd::ClientId& client_id) override;
    bcmd::Result<domain::ClientSession> registerClient(domain::Username username) override;
    bcmd::VoidResult save(const domain::ClientSession& session) override;
    bcmd::VoidResult remove(const bcmd::ClientId& client_id) override;
    bcmd::VoidResult touchHeartbeat(const bcmd::ClientId& client_id) override;
    std::vector<domain::ClientSession> collectExpired(
        std::chrono::steady_clock::time_point deadline) override;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, domain::ClientSession> clients_by_id_;
    std::unordered_map<std::string, std::string> username_to_id_;
    std::unordered_map<std::string, domain::Username> historical_usernames_;
};

}  // namespace bcmd::server::adapter::persistence
