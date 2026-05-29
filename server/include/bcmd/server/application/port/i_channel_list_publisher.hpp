#pragma once

#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/shared/ids.hpp"

#include <cstdint>

namespace grpc {
template <class W>
class ServerWriterInterface;
}  // namespace grpc

namespace bcmd::v1 {
class ChannelListEvent;
}  // namespace bcmd::v1

namespace bcmd::server::application::port {

class IChannelListPublisher {
public:
    virtual ~IChannelListPublisher() = default;

    virtual void registerSubscriberWithSnapshot(
        const bcmd::ClientId& subscriber_id,
        ::grpc::ServerWriterInterface<bcmd::v1::ChannelListEvent>* writer) = 0;

    virtual void unregisterSubscriber(const bcmd::ClientId& subscriber_id) = 0;

    virtual void publishChannelCreated(const bcmd::server::domain::Channel& channel) = 0;

    virtual void publishMemberCountChanged(const bcmd::ChannelId& channel_id,
                                           std::int32_t member_count) = 0;

protected:
    IChannelListPublisher() = default;
    IChannelListPublisher(const IChannelListPublisher&) = default;
    IChannelListPublisher& operator=(const IChannelListPublisher&) = default;
    IChannelListPublisher(IChannelListPublisher&&) = default;
    IChannelListPublisher& operator=(IChannelListPublisher&&) = default;
};

}  // namespace bcmd::server::application::port
