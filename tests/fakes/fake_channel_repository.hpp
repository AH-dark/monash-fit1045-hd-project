#pragma once

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/server/domain/model/channel_name.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <expected>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bcmd::tests {

class FakeChannelRepository final : public bcmd::server::application::port::IChannelRepository {
public:
    bcmd::Result<bcmd::server::domain::Channel> findById(
        const bcmd::ChannelId& channel_id) override {
        const auto iter = by_id_.find(channel_id);
        if (iter == by_id_.end()) {
            return std::unexpected(bcmd::Error::ChannelNotFound);
        }
        return iter->second;
    }

    bcmd::Result<bcmd::server::domain::Channel> findByName(
        const bcmd::server::domain::ChannelName& name) override {
        const auto iter = name_to_id_.find(name);
        if (iter == name_to_id_.end()) {
            return std::unexpected(bcmd::Error::ChannelNotFound);
        }
        return by_id_.at(iter->second);
    }

    std::vector<bcmd::server::domain::Channel> listAll() override {
        std::vector<bcmd::server::domain::Channel> snapshot;
        snapshot.reserve(by_id_.size());
        for (const auto& [_, channel] : by_id_) {
            snapshot.push_back(channel);
        }
        return snapshot;
    }

    bcmd::VoidResult save(const bcmd::server::domain::Channel& channel) override {
        by_id_.insert_or_assign(channel.id(), channel);
        name_to_id_.insert_or_assign(channel.name(), channel.id());
        return {};
    }

    bcmd::Result<bcmd::server::domain::Channel> create(
        bcmd::server::domain::ChannelName name) override {
        if (name_to_id_.contains(name)) {
            return std::unexpected(bcmd::Error::ChannelAlreadyExists);
        }
        bcmd::server::domain::Channel channel{bcmd::ChannelId::generate(), name};
        name_to_id_.emplace(std::move(name), channel.id());
        by_id_.emplace(channel.id(), channel);
        return channel;
    }

private:
    std::unordered_map<bcmd::ChannelId, bcmd::server::domain::Channel> by_id_;
    std::unordered_map<bcmd::server::domain::ChannelName, bcmd::ChannelId> name_to_id_;
};

}  // namespace bcmd::tests
