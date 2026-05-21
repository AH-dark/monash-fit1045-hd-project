#include "bcmd/server/adapter/persistence/in_memory_client_registry.hpp"

#include "bcmd/server/domain/model/client_session.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <expected>
#include <mutex>
#include <shared_mutex>
#include <utility>

namespace bcmd::server::adapter::persistence {

bcmd::Result<domain::ClientSession> InMemoryClientRegistry::findById(
    const bcmd::ClientId& client_id) {
    const std::shared_lock lock(mutex_);
    const auto found = clients_by_id_.find(client_id.value());
    if (found == clients_by_id_.end()) {
        return std::unexpected(bcmd::Error::ClientNotFound);
    }
    return found->second;
}

bcmd::Result<domain::ClientSession> InMemoryClientRegistry::findByUsername(
    const domain::Username& name) {
    const std::shared_lock lock(mutex_);
    const auto id_found = username_to_id_.find(name.value());
    if (id_found == username_to_id_.end()) {
        return std::unexpected(bcmd::Error::ClientNotFound);
    }
    const auto client_found = clients_by_id_.find(id_found->second);
    if (client_found == clients_by_id_.end()) {
        return std::unexpected(bcmd::Error::ClientNotFound);
    }
    return client_found->second;
}

bcmd::Result<domain::ClientSession> InMemoryClientRegistry::registerClient(
    domain::Username username) {
    const std::unique_lock lock(mutex_);
    if (username_to_id_.contains(username.value())) {
        return std::unexpected(bcmd::Error::ClientAlreadyExists);
    }

    domain::ClientSession session{bcmd::ClientId::generate(), std::move(username)};
    const auto id = session.id().value();
    const auto username_value = session.username().value();
    clients_by_id_.emplace(id, session);
    username_to_id_.emplace(username_value, id);
    return session;
}

bcmd::VoidResult InMemoryClientRegistry::save(const domain::ClientSession& session) {
    const std::unique_lock lock(mutex_);
    clients_by_id_.insert_or_assign(session.id().value(), session);
    username_to_id_.insert_or_assign(session.username().value(), session.id().value());
    return {};
}

bcmd::VoidResult InMemoryClientRegistry::remove(const bcmd::ClientId& client_id) {
    const std::unique_lock lock(mutex_);
    const auto found = clients_by_id_.find(client_id.value());
    if (found == clients_by_id_.end()) {
        return std::unexpected(bcmd::Error::ClientNotFound);
    }
    username_to_id_.erase(found->second.username().value());
    clients_by_id_.erase(found);
    return {};
}

}  // namespace bcmd::server::adapter::persistence
