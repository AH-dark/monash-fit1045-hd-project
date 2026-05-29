#pragma once

#include "bcmd/server/application/port/i_channel_list_publisher.hpp"
#include "bcmd/server/application/port/i_client_registry.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <memory>

namespace bcmd::server::application::usecase {

class SubscribeToChannelList {
public:
    SubscribeToChannelList(std::shared_ptr<port::IClientRegistry> clients,
                           std::shared_ptr<port::IChannelListPublisher> publisher);

    // Validates the subscriber and registers the writer with the publisher.
    // The publisher synchronously delivers an initial snapshot before returning.
    // The caller (gRPC handler) is responsible for the polling loop and for
    // invoking `unregisterSubscriber` on disconnect.
    bcmd::VoidResult execute(const bcmd::ClientId& subscriber_id,
                             ::grpc::ServerWriterInterface<bcmd::v1::ChannelListEvent>* writer);

private:
    std::shared_ptr<port::IClientRegistry> clients_{};
    std::shared_ptr<port::IChannelListPublisher> publisher_{};
};

}  // namespace bcmd::server::application::usecase
