#pragma once

#include <string>

#include "bcmd/shared/result.hpp"

namespace bcmd::server::adapter::grpc {

bcmd::Result<std::string> read_pem_file(const std::string& path);

}  // namespace bcmd::server::adapter::grpc
