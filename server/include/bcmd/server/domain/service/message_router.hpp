#pragma once

#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/shared/ids.hpp"

#include <cstdint>
#include <vector>

namespace bcmd::server::domain {

enum class EchoPolicy : std::uint8_t {
    ExcludeSender,
    IncludeSender,
};

// Pure stateless function-object: given a channel, the message being
// broadcast, and an echo policy, returns the list of recipient client IDs.
// Stateless by design so it is trivially thread-safe.
class MessageRouter {
public:
    static std::vector<bcmd::ClientId> recipientsFor(
        const Channel& channel, const Message& message,
        EchoPolicy echo_policy = EchoPolicy::ExcludeSender);
};

}  // namespace bcmd::server::domain
