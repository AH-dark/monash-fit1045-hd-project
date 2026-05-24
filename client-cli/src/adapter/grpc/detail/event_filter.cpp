#include "bcmd/client/adapter/grpc/detail/event_filter.hpp"

#include "bcmd/v1/broadcast.pb.h"

#include <string_view>

namespace bcmd::client::adapter::grpc::detail {

bool shouldEmitEventForChannel(const bcmd::v1::ChannelEvent& event,
                               std::string_view subscribed_channel_id) {
    if (event.has_member_left()) {
        return event.member_left().channel_id() == subscribed_channel_id;
    }
    if (event.has_member_joined()) {
        return event.member_joined().channel_id() == subscribed_channel_id;
    }
    return true;
}

}  // namespace bcmd::client::adapter::grpc::detail
