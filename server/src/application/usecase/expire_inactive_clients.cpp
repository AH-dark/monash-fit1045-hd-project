#include "bcmd/server/application/usecase/expire_inactive_clients.hpp"

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/application/usecase/internal/remove_member_broadcast.hpp"
#include "bcmd/server/domain/model/client_session.hpp"
#include "bcmd/shared/result.hpp"

#include <spdlog/spdlog.h>

#include <chrono>
#include <cstddef>
#include <memory>
#include <utility>

namespace bcmd::server::application::usecase {

ExpireInactiveClients::ExpireInactiveClients(std::shared_ptr<port::IClientRegistry> clients,
                                             std::shared_ptr<port::IChannelRepository> channels,
                                             std::shared_ptr<port::IMessagePublisher> publisher)
    : clients_(std::move(clients)),
      channels_(std::move(channels)),
      publisher_(std::move(publisher)) {}

std::size_t ExpireInactiveClients::run(std::chrono::steady_clock::time_point deadline) {
    auto expired = clients_->collectExpired(deadline);
    std::size_t count = 0;
    for (const auto& session : expired) {
        for (const auto& channel_id : session.joinedChannels()) {
            if (auto r = internal::removeMemberAndBroadcast(*channels_, *publisher_, session,
                                                            channel_id);
                !r.has_value()) {
                if (r.error() != bcmd::Error::NotAMember &&
                    r.error() != bcmd::Error::ChannelNotFound) {
                    spdlog::debug("expire: removeMember unexpected error for {} in {}: {}",
                                  session.id().value(), channel_id.value(),
                                  bcmd::error_message(r.error()));
                }
            }
        }
        if (auto r = clients_->remove(session.id()); !r.has_value()) {
            if (r.error() != bcmd::Error::ClientNotFound) {
                spdlog::debug("expire: remove client {} unexpected error: {}", session.id().value(),
                              bcmd::error_message(r.error()));
            }
        }
        publisher_->unregisterSubscriber(session.id());
        ++count;
        spdlog::info("expired client {} (username={})", session.id().value(),
                     session.username().value());
    }
    return count;
}

}  // namespace bcmd::server::application::usecase
