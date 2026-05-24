#include "bcmd/client/adapter/grpc/detail/status_to_error.hpp"

#include "bcmd/shared/result.hpp"

#include <catch2/catch_test_macros.hpp>
#include <grpcpp/support/status.h>

#include <string>

namespace {

bcmd::Error mapStatus(::grpc::StatusCode code, const std::string& message) {
    return bcmd::client::adapter::grpc::detail::statusToError(::grpc::Status{code, message});
}

bcmd::Error mapServerError(bcmd::Error server_error, ::grpc::StatusCode code) {
    return mapStatus(code, std::string(bcmd::error_message(server_error)));
}

}  // namespace

TEST_CASE("statusToError maps NOT_FOUND status codes to bcmd::Error",
          "[adapter][grpc][status-to-error]") {
    CHECK(mapServerError(bcmd::Error::ClientNotFound, ::grpc::StatusCode::NOT_FOUND) ==
          bcmd::Error::ClientNotFound);
    CHECK(mapServerError(bcmd::Error::NotAMember, ::grpc::StatusCode::NOT_FOUND) ==
          bcmd::Error::NotAMember);
    CHECK(mapServerError(bcmd::Error::ChannelNotFound, ::grpc::StatusCode::NOT_FOUND) ==
          bcmd::Error::ChannelNotFound);

    SECTION("unknown NOT_FOUND payload defaults to ChannelNotFound") {
        CHECK(mapStatus(::grpc::StatusCode::NOT_FOUND, "something else") ==
              bcmd::Error::ChannelNotFound);
    }
}

TEST_CASE("statusToError maps ALREADY_EXISTS status codes to bcmd::Error",
          "[adapter][grpc][status-to-error]") {
    CHECK(mapServerError(bcmd::Error::ChannelAlreadyExists, ::grpc::StatusCode::ALREADY_EXISTS) ==
          bcmd::Error::ChannelAlreadyExists);
    CHECK(mapServerError(bcmd::Error::ClientAlreadyExists, ::grpc::StatusCode::ALREADY_EXISTS) ==
          bcmd::Error::ClientAlreadyExists);
    CHECK(mapServerError(bcmd::Error::AlreadyMember, ::grpc::StatusCode::ALREADY_EXISTS) ==
          bcmd::Error::AlreadyMember);

    SECTION("unknown ALREADY_EXISTS payload defaults to AlreadyMember") {
        CHECK(mapStatus(::grpc::StatusCode::ALREADY_EXISTS, "something else") ==
              bcmd::Error::AlreadyMember);
    }
}

TEST_CASE("statusToError maps INVALID_ARGUMENT status codes to bcmd::Error",
          "[adapter][grpc][status-to-error]") {
    CHECK(mapServerError(bcmd::Error::InvalidUsername, ::grpc::StatusCode::INVALID_ARGUMENT) ==
          bcmd::Error::InvalidUsername);
    CHECK(mapServerError(bcmd::Error::MessageTooLong, ::grpc::StatusCode::INVALID_ARGUMENT) ==
          bcmd::Error::MessageTooLong);
    CHECK(mapServerError(bcmd::Error::MessageEmpty, ::grpc::StatusCode::INVALID_ARGUMENT) ==
          bcmd::Error::MessageEmpty);
    CHECK(mapServerError(bcmd::Error::MessageInvalidPrefix, ::grpc::StatusCode::INVALID_ARGUMENT) ==
          bcmd::Error::MessageInvalidPrefix);
    CHECK(mapServerError(bcmd::Error::InvalidChannelName, ::grpc::StatusCode::INVALID_ARGUMENT) ==
          bcmd::Error::InvalidChannelName);

    SECTION("unknown INVALID_ARGUMENT payload defaults to InvalidChannelName") {
        CHECK(mapStatus(::grpc::StatusCode::INVALID_ARGUMENT, "something else") ==
              bcmd::Error::InvalidChannelName);
    }
}

TEST_CASE("statusToError maps transport-level failures to NetworkError",
          "[adapter][grpc][status-to-error]") {
    CHECK(mapStatus(::grpc::StatusCode::UNAVAILABLE, "server down") == bcmd::Error::NetworkError);
    CHECK(mapStatus(::grpc::StatusCode::UNKNOWN, "weird") == bcmd::Error::NetworkError);
    CHECK(mapStatus(::grpc::StatusCode::INTERNAL, "boom") == bcmd::Error::NetworkError);
    CHECK(mapStatus(::grpc::StatusCode::DEADLINE_EXCEEDED, "slow") == bcmd::Error::NetworkError);
    CHECK(mapStatus(::grpc::StatusCode::UNAUTHENTICATED, "no creds") == bcmd::Error::NetworkError);
}
