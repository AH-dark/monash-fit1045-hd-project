#pragma once

#include <string>
#include <vector>

#include <grpcpp/grpcpp.h>

namespace bcmd::server::adapter::grpc {

class GrpcServerRunner {
public:
    explicit GrpcServerRunner(std::string bind_address);

    void add_service(::grpc::Service& service);
    void set_ssl_credentials(const std::string& cert_pem, const std::string& key_pem);
    void set_insecure();
    int run_and_block();

private:
    std::string bind_address_;
    std::vector<::grpc::Service*> services_;
    bool insecure_{false};
    std::string cert_pem_;
    std::string key_pem_;
};

}  // namespace bcmd::server::adapter::grpc
