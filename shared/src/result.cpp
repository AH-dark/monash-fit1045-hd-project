#include "bcmd/shared/result.hpp"

namespace bcmd {

std::string_view error_message(Error error) noexcept {
    switch (error) {
        case Error::ChannelNotFound:      return "channel not found";
        case Error::ChannelAlreadyExists: return "channel already exists";
        case Error::ClientNotFound:       return "client not found";
        case Error::ClientAlreadyExists:  return "client already exists";
        case Error::AlreadyMember:        return "already a member";
        case Error::NotAMember:           return "not a member";
        case Error::MessageTooLong:       return "message too long";
        case Error::MessageEmpty:         return "message is empty";
        case Error::InvalidUsername:      return "invalid username";
        case Error::InvalidChannelName:   return "invalid channel name";
        case Error::StorageError:         return "storage error";
        case Error::NetworkError:         return "network error";
        case Error::NotImplemented:       return "not implemented";
    }
    return "unknown error";
}

}  // namespace bcmd
