#pragma once

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_message_repository.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace bcmd::server::application::usecase {

class ListMessages {
public:
    static constexpr std::uint32_t kServerMaxLimit = 200;

    struct Result {
        std::vector<domain::Message> messages;
        bool has_more = false;
    };

    ListMessages(std::shared_ptr<port::IChannelRepository> channels,
                 std::shared_ptr<port::IMessageRepository> messages);

    // Returns up to `limit` messages strictly older than `before_message_id`
    // (or the most recent `limit` when the cursor is empty), in chronological
    // order. Caller must already be a member of the channel.
    bcmd::Result<Result> execute(const bcmd::ClientId& client_id, const bcmd::ChannelId& channel_id,
                                 const std::optional<bcmd::MessageId>& before_message_id,
                                 std::uint32_t limit);

private:
    std::shared_ptr<port::IChannelRepository> channels_{};
    std::shared_ptr<port::IMessageRepository> messages_{};
};

}  // namespace bcmd::server::application::usecase
