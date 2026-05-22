#pragma once

#include "bcmd/server/application/port/i_channel_repository.hpp"
#include "bcmd/server/application/port/i_message_publisher.hpp"
#include "bcmd/server/domain/model/client_session.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

namespace bcmd::server::application::usecase::internal {

// Removes `client_id` from `channel_id` in the channel repository,
// broadcasts a MemberLeftEvent to subscribers of `channel_id` via the publisher,
// and saves the updated channel. STRICT error semantics: returns
// `Error::NotAMember` if the client is not in the channel, `Error::ChannelNotFound`
// if the channel does not exist. Does NOT touch the client registry.
//
// Callers that need idempotent behavior (e.g., ExpireInactiveClients racing
// a concurrent Disconnect) MUST swallow `NotAMember`/`ChannelNotFound` at the
// call site. LeaveChannel does NOT swallow.
//
// Lock-order invariant: this function acquires the channel repository's lock
// and the publisher's lock independently; it MUST NOT hold the client registry
// lock when called.
bcmd::VoidResult removeMemberAndBroadcast(port::IChannelRepository& channels,
                                          port::IMessagePublisher& publisher,
                                          const domain::ClientSession& session,
                                          const bcmd::ChannelId& channel_id);

}  // namespace bcmd::server::application::usecase::internal
