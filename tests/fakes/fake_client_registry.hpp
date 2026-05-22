#pragma once

#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/domain/model/client_session.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <chrono>
#include <expected>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bcmd::tests {

class FakeClientRegistry final : public bcmd::server::application::port::IClientRegistry {
public:
    bcmd::Result<bcmd::server::domain::ClientSession> findById(
        const bcmd::ClientId& client_id) override {
        const auto iter = by_id_.find(client_id);
        if (iter == by_id_.end()) {
            return std::unexpected(bcmd::Error::ClientNotFound);
        }
        return iter->second;
    }

    bcmd::Result<bcmd::server::domain::ClientSession> findByUsername(
        const bcmd::server::domain::Username& name) override {
        const auto iter = name_to_id_.find(name);
        if (iter == name_to_id_.end()) {
            return std::unexpected(bcmd::Error::ClientNotFound);
        }
        return by_id_.at(iter->second);
    }

    bcmd::Result<bcmd::server::domain::ClientSession> registerClient(
        bcmd::server::domain::Username username) override {
        if (name_to_id_.contains(username)) {
            return std::unexpected(bcmd::Error::ClientAlreadyExists);
        }
        bcmd::server::domain::ClientSession session{bcmd::ClientId::generate(), username};
        name_to_id_.emplace(std::move(username), session.id());
        by_id_.emplace(session.id(), session);
        return session;
    }

    bcmd::VoidResult save(const bcmd::server::domain::ClientSession& session) override {
        by_id_.insert_or_assign(session.id(), session);
        name_to_id_.insert_or_assign(session.username(), session.id());
        return {};
    }

    bcmd::VoidResult remove(const bcmd::ClientId& client_id) override {
        const auto iter = by_id_.find(client_id);
        if (iter == by_id_.end()) {
            return std::unexpected(bcmd::Error::ClientNotFound);
        }
        name_to_id_.erase(iter->second.username());
        by_id_.erase(iter);
        return {};
    }

    bcmd::VoidResult touchHeartbeat(const bcmd::ClientId& client_id) override {
        const auto iter = by_id_.find(client_id);
        if (iter == by_id_.end()) {
            return std::unexpected(bcmd::Error::ClientNotFound);
        }
        iter->second.touch();
        return {};
    }

    std::vector<bcmd::server::domain::ClientSession> collectExpired(
        std::chrono::steady_clock::time_point deadline) override {
        std::vector<bcmd::server::domain::ClientSession> expired;
        for (const auto& [_, session] : by_id_) {
            if (session.isExpired(deadline)) {
                expired.push_back(session);
            }
        }
        return expired;
    }

private:
    std::unordered_map<bcmd::ClientId, bcmd::server::domain::ClientSession> by_id_;
    std::unordered_map<bcmd::server::domain::Username, bcmd::ClientId> name_to_id_;
};

}  // namespace bcmd::tests
