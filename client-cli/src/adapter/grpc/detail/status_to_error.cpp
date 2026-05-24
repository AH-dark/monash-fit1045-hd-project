#include "bcmd/client/adapter/grpc/detail/status_to_error.hpp"

#include "bcmd/shared/result.hpp"

#include <grpcpp/support/status.h>

#include <string>

namespace bcmd::client::adapter::grpc::detail {

bcmd::Error statusToError(const ::grpc::Status& status) {
    const std::string message = status.error_message();

    switch (status.error_code()) {
        case ::grpc::StatusCode::NOT_FOUND:
            if (message.starts_with("client not found")) {
                return bcmd::Error::ClientNotFound;
            }
            if (message.starts_with("not a member")) {
                return bcmd::Error::NotAMember;
            }
            return bcmd::Error::ChannelNotFound;
        case ::grpc::StatusCode::ALREADY_EXISTS:
            if (message.starts_with("channel already exists")) {
                return bcmd::Error::ChannelAlreadyExists;
            }
            if (message.starts_with("client already exists")) {
                return bcmd::Error::ClientAlreadyExists;
            }
            return bcmd::Error::AlreadyMember;
        case ::grpc::StatusCode::INVALID_ARGUMENT:
            if (message.starts_with("invalid username")) {
                return bcmd::Error::InvalidUsername;
            }
            if (message.starts_with("message too long")) {
                return bcmd::Error::MessageTooLong;
            }
            if (message.starts_with("message is empty")) {
                return bcmd::Error::MessageEmpty;
            }
            if (message.starts_with("message must not start with '/'")) {
                return bcmd::Error::MessageInvalidPrefix;
            }
            return bcmd::Error::InvalidChannelName;
        case ::grpc::StatusCode::UNAVAILABLE:
        case ::grpc::StatusCode::UNKNOWN:
        case ::grpc::StatusCode::INTERNAL:
        default:
            return bcmd::Error::NetworkError;
    }
}

}  // namespace bcmd::client::adapter::grpc::detail
