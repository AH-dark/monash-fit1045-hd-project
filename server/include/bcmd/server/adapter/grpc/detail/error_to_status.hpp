#pragma once

#include "bcmd/shared/result.hpp"

#include <grpcpp/support/status.h>

namespace bcmd::server::adapter::grpc::detail {

[[nodiscard]] ::grpc::Status errorToStatus(bcmd::Error error);

}  // namespace bcmd::server::adapter::grpc::detail
