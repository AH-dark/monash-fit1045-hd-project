#pragma once

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
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
    void shutdown();
    std::string bound_address() const;
    bool wait_until_started(std::chrono::milliseconds timeout);

private:
    std::string bind_address_;
    std::vector<::grpc::Service*> services_;
    bool insecure_{false};
    std::string cert_pem_;
    std::string key_pem_;
    mutable std::mutex mutex_;
    std::condition_variable started_cv_;
    std::unique_ptr<::grpc::Server> server_;
    std::string bound_address_;
    bool started_{false};
};

}  // namespace bcmd::server::adapter::grpc
