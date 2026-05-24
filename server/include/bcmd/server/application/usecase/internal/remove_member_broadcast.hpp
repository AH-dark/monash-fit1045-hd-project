#pragma once

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/domain/model/client_session.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

namespace bcmd::server::application::usecase::internal {

// Removes `client_id` from `channel_id` in the channel repository, saves the
// updated channel, snapshots remaining members, and broadcasts a MemberLeftEvent
// only to subscribers in that snapshot. STRICT error semantics: returns
// `Error::NotAMember` if the client is not in the channel, `Error::ChannelNotFound`
// if the channel does not exist. Does NOT touch the client registry.
//
// Callers that need idempotent behavior (e.g., ExpireInactiveClients racing
// a concurrent Disconnect) MUST swallow `NotAMember`/`ChannelNotFound` at the
// call site. LeaveChannel does NOT swallow.
//
// Lock-order invariant: this function uses snapshot-passing so the publisher
// never queries the channel repository; channel repository and publisher locks
// remain independent, and callers MUST NOT hold the client registry lock.
bcmd::VoidResult removeMemberAndBroadcast(port::IChannelRepository& channels,
                                          port::IMessagePublisher& publisher,
                                          const domain::ClientSession& session,
                                          const bcmd::ChannelId& channel_id);

}  // namespace bcmd::server::application::usecase::internal
