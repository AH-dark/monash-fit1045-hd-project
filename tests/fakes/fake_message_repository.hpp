#pragma once

#include "bcmd/server/application/port/i_message_repository.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace bcmd::tests {

class FakeMessageRepository final : public bcmd::server::application::port::IMessageRepository {
public:
    bcmd::VoidResult save(const bcmd::server::domain::Message& message) override {
        by_channel_[message.channelId()].push_back(message);
        return {};
    }

    std::vector<bcmd::server::domain::Message> recent(const bcmd::ChannelId& channel_id,
                                                      std::uint32_t count) override {
        const auto iter = by_channel_.find(channel_id);
        if (iter == by_channel_.end() || count == 0) {
            return {};
        }
        const auto& stream = iter->second;
        const auto take = std::min<std::size_t>(count, stream.size());
        return {stream.end() - static_cast<std::ptrdiff_t>(take), stream.end()};
    }

    [[nodiscard]] std::size_t totalFor(const bcmd::ChannelId& channel_id) const {
        const auto iter = by_channel_.find(channel_id);
        return iter == by_channel_.end() ? 0 : iter->second.size();
    }

private:
    std::unordered_map<bcmd::ChannelId, std::vector<bcmd::server::domain::Message>> by_channel_;
};

}  // namespace bcmd::tests
