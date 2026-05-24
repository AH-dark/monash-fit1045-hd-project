#pragma once

#include "bcmd/v1/broadcast.pb.h"

#include <string_view>

namespace bcmd::client::adapter::grpc::detail {

[[nodiscard]] bool shouldEmitEventForChannel(const bcmd::v1::ChannelEvent& event,
                                             std::string_view subscribed_channel_id);

}  // namespace bcmd::client::adapter::grpc::detail
