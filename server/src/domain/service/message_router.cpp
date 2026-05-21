#include "bcmd/server/domain/service/message_router.hpp"

#include "bcmd/server/domain/model/channel.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/shared/ids.hpp"

#include <vector>

namespace bcmd::server::domain {

std::vector<bcmd::ClientId> MessageRouter::recipientsFor(const Channel& channel,
                                                         const Message& message,
                                                         EchoPolicy echo_policy) {
    const auto& members = channel.members();

    std::vector<bcmd::ClientId> recipients;
    recipients.reserve(members.size());

    for (const auto& member_id : members) {
        if (echo_policy == EchoPolicy::ExcludeSender && member_id == message.senderId()) {
            continue;
        }
        recipients.push_back(member_id);
    }

    return recipients;
}

}  // namespace bcmd::server::domain
