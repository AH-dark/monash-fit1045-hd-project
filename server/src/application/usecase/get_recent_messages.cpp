#include "bcmd/server/application/usecase/get_recent_messages.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "bcmd/server/application/port/i_message_repository.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/shared/ids.hpp"

namespace bcmd::server::application::usecase {

GetRecentMessages::GetRecentMessages(std::shared_ptr<port::IMessageRepository> messages)
    : messages_(std::move(messages)) {}

std::vector<domain::Message> GetRecentMessages::execute(const bcmd::ChannelId& channel_id,
                                                         std::uint32_t count) {
    const auto capped = std::min(count, kServerMaxMessages);
    return messages_->recent(channel_id, capped);
}

}  // namespace bcmd::server::application::usecase
