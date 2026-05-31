#include "bcmd/server/application/usecase/list_messages.hpp"

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_message_repository.hpp"
#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <utility>

namespace bcmd::server::application::usecase {

ListMessages::ListMessages(std::shared_ptr<port::IChannelRepository> channels,
                           std::shared_ptr<port::IMessageRepository> messages)
    : channels_(std::move(channels)), messages_(std::move(messages)) {}

bcmd::Result<ListMessages::Result> ListMessages::execute(
    const bcmd::ClientId& client_id, const bcmd::ChannelId& channel_id,
    const std::optional<bcmd::MessageId>& before_message_id, std::uint32_t limit) {
    auto channel = channels_->findById(channel_id);
    if (!channel.has_value()) {
        return std::unexpected(channel.error());
    }
    if (!channel->hasMember(client_id)) {
        return std::unexpected(bcmd::Error::NotAMember);
    }

    const auto capped = std::min(limit, kServerMaxLimit);
    auto page = before_message_id.has_value()
                    ? messages_->listBefore(channel_id, *before_message_id, capped)
                    : messages_->recent(channel_id, capped);
    const bool has_more =
        !page.empty() && !messages_->listBefore(channel_id, page.front().id(), 1).empty();
    return Result{std::move(page), has_more};
}

}  // namespace bcmd::server::application::usecase
