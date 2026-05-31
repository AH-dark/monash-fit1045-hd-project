#include "bcmd/shared/result.hpp"

#include <string_view>

namespace bcmd {

std::string_view error_message(Error error) noexcept {
    switch (error) {
        case Error::ChannelNotFound:
            return "channel not found";
        case Error::ChannelAlreadyExists:
            return "channel already exists";
        case Error::ClientNotFound:
            return "client not found";
        case Error::ClientAlreadyExists:
            return "client already exists";
        case Error::AlreadyMember:
            return "already a member";
        case Error::NotAMember:
            return "not a member";
        case Error::MessageEmpty:
            return "message is empty";
        case Error::MessageInvalidPrefix:
            return "message must not start with '/'";
        case Error::MessageTooLong:
            return "message too long";
        case Error::InvalidUsername:
            return "invalid username";
        case Error::InvalidChannelName:
            return "invalid channel name (allowed: 1-64 chars of [a-zA-Z0-9-])";
        case Error::InvalidArgument:
            return "invalid argument";
        case Error::StorageError:
            return "storage error";
        case Error::NetworkError:
            return "network error";
        case Error::NotImplemented:
            return "not implemented";
    }
    return "unknown error";
}

}  // namespace bcmd
