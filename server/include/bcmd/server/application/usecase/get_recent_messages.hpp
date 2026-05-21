#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "bcmd/server/application/port/i_message_repository.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/shared/ids.hpp"

namespace bcmd::server::application::usecase {

class GetRecentMessages {
public:
    // Server-side hard cap. Even when a caller asks for more, the use case
    // refuses to fetch beyond this to bound work + bandwidth.
    static constexpr std::uint32_t kServerMaxMessages = 200;

    explicit GetRecentMessages(std::shared_ptr<port::IMessageRepository> messages);

    std::vector<domain::Message> execute(const bcmd::ChannelId& channel_id,
                                         std::uint32_t count);

private:
    std::shared_ptr<port::IMessageRepository> messages_;
};

}  // namespace bcmd::server::application::usecase
