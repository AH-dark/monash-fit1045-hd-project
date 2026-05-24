#include "bcmd/server/adapter/grpc/detail/error_to_status.hpp"

#include "bcmd/shared/result.hpp"

#include <catch2/catch_test_macros.hpp>
#include <grpcpp/support/status.h>

#include <string>

namespace {

void expectStatusCode(bcmd::Error error, ::grpc::StatusCode expected_code) {
    const auto status = bcmd::server::adapter::grpc::detail::errorToStatus(error);
    CHECK(status.error_code() == expected_code);
    CHECK(status.error_message() == std::string(bcmd::error_message(error)));
}

}  // namespace

TEST_CASE("errorToStatus maps invalid argument errors to INVALID_ARGUMENT",
          "[adapter][grpc][error-to-status]") {
    expectStatusCode(bcmd::Error::MessageInvalidPrefix, ::grpc::StatusCode::INVALID_ARGUMENT);
    expectStatusCode(bcmd::Error::MessageEmpty, ::grpc::StatusCode::INVALID_ARGUMENT);
    expectStatusCode(bcmd::Error::MessageTooLong, ::grpc::StatusCode::INVALID_ARGUMENT);
    expectStatusCode(bcmd::Error::InvalidUsername, ::grpc::StatusCode::INVALID_ARGUMENT);
    expectStatusCode(bcmd::Error::InvalidChannelName, ::grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_CASE("errorToStatus maps not found errors to NOT_FOUND", "[adapter][grpc][error-to-status]") {
    expectStatusCode(bcmd::Error::ChannelNotFound, ::grpc::StatusCode::NOT_FOUND);
    expectStatusCode(bcmd::Error::ClientNotFound, ::grpc::StatusCode::NOT_FOUND);
    expectStatusCode(bcmd::Error::NotAMember, ::grpc::StatusCode::NOT_FOUND);
}

TEST_CASE("errorToStatus maps already exists errors to ALREADY_EXISTS",
          "[adapter][grpc][error-to-status]") {
    expectStatusCode(bcmd::Error::AlreadyMember, ::grpc::StatusCode::ALREADY_EXISTS);
    expectStatusCode(bcmd::Error::ChannelAlreadyExists, ::grpc::StatusCode::ALREADY_EXISTS);
    expectStatusCode(bcmd::Error::ClientAlreadyExists, ::grpc::StatusCode::ALREADY_EXISTS);
}

TEST_CASE("errorToStatus maps internal errors to INTERNAL", "[adapter][grpc][error-to-status]") {
    expectStatusCode(bcmd::Error::StorageError, ::grpc::StatusCode::INTERNAL);
    expectStatusCode(bcmd::Error::NetworkError, ::grpc::StatusCode::INTERNAL);
    expectStatusCode(bcmd::Error::NotImplemented, ::grpc::StatusCode::INTERNAL);
}
