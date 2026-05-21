#pragma once

#include "bcmd/shared/result.hpp"

#include <string>

namespace bcmd::server::adapter::grpc {

bcmd::Result<std::string> read_pem_file(const std::string& path);

}  // namespace bcmd::server::adapter::grpc
