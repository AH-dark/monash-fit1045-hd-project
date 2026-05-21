#pragma once

#include "bcmd/server/application/port/i_message_repository.hpp"

#include <cstdint>
#include <deque>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace bcmd::server::adapter::persistence {

class InMemoryMessageRepository final : public application::port::IMessageRepository {
public:
    static constexpr std::uint32_t kDefaultCap = 200;

    explicit InMemoryMessageRepository(std::uint32_t cap = kDefaultCap);

    bcmd::VoidResult save(const domain::Message& message) override;
    std::vector<domain::Message> recent(const bcmd::ChannelId& channel_id,
                                        std::uint32_t count) override;

private:
    mutable std::shared_mutex mutex_{};
    std::uint32_t cap_{kDefaultCap};
    std::unordered_map<std::string, std::deque<domain::Message>> buffers_{};
};

}  // namespace bcmd::server::adapter::persistence
