#include "bcmd/server/application/usecase/subscribe_to_channel_list.hpp"

#include "bcmd/server/application/port/i_channel_list_publisher.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <expected>
#include <memory>
#include <utility>

namespace bcmd::server::application::usecase {

SubscribeToChannelList::SubscribeToChannelList(
    std::shared_ptr<port::IClientRegistry> clients,
    std::shared_ptr<port::IChannelListPublisher> publisher)
    : clients_(std::move(clients)), publisher_(std::move(publisher)) {}

bcmd::VoidResult SubscribeToChannelList::execute(
    const bcmd::ClientId& subscriber_id,
    ::grpc::ServerWriterInterface<bcmd::v1::ChannelListEvent>* writer) {
    auto client = clients_->findById(subscriber_id);
    if (!client.has_value()) {
        return std::unexpected(client.error());
    }
    publisher_->registerSubscriberWithSnapshot(subscriber_id, writer);
    return {};
}

}  // namespace bcmd::server::application::usecase
