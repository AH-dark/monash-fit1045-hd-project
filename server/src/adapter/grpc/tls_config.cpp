#include "bcmd/server/adapter/grpc/tls_config.hpp"

#include "bcmd/shared/result.hpp"

#include <expected>
#include <fstream>
#include <sstream>
#include <string>

namespace bcmd::server::adapter::grpc {

bcmd::Result<std::string> read_pem_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return std::unexpected(bcmd::Error::StorageError);
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

}  // namespace bcmd::server::adapter::grpc
