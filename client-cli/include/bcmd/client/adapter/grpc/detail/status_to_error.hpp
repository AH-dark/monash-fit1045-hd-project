#pragma once

#include "bcmd/shared/result.hpp"

#include <grpcpp/support/status.h>

namespace bcmd::client::adapter::grpc::detail {

[[nodiscard]] bcmd::Error statusToError(const ::grpc::Status& status);

}  // namespace bcmd::client::adapter::grpc::detail
