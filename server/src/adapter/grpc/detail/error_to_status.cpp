#include "bcmd/server/adapter/grpc/detail/error_to_status.hpp"

#include "bcmd/shared/result.hpp"

#include <grpcpp/support/status.h>

#include <string>

namespace bcmd::server::adapter::grpc::detail {

::grpc::Status errorToStatus(bcmd::Error error) {
    switch (error) {
        case bcmd::Error::ChannelNotFound:
        case bcmd::Error::ClientNotFound:
        case bcmd::Error::NotAMember:
            return {::grpc::StatusCode::NOT_FOUND, std::string(bcmd::error_message(error))};
        case bcmd::Error::AlreadyMember:
        case bcmd::Error::ChannelAlreadyExists:
        case bcmd::Error::ClientAlreadyExists:
            return {::grpc::StatusCode::ALREADY_EXISTS, std::string(bcmd::error_message(error))};
        case bcmd::Error::InvalidUsername:
        case bcmd::Error::InvalidChannelName:
        case bcmd::Error::MessageEmpty:
        case bcmd::Error::MessageInvalidPrefix:
        case bcmd::Error::MessageTooLong:
            return {::grpc::StatusCode::INVALID_ARGUMENT, std::string(bcmd::error_message(error))};
        case bcmd::Error::StorageError:
        case bcmd::Error::NetworkError:
        case bcmd::Error::NotImplemented:
            return {::grpc::StatusCode::INTERNAL, std::string(bcmd::error_message(error))};
    }

    return {::grpc::StatusCode::INTERNAL, std::string(bcmd::error_message(error))};
}

}  // namespace bcmd::server::adapter::grpc::detail
