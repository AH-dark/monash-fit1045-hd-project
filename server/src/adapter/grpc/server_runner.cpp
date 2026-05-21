#include "bcmd/server/adapter/grpc/server_runner.hpp"

#include <memory>
#include <string>
#include <utility>

#include <spdlog/spdlog.h>

namespace bcmd::server::adapter::grpc {

GrpcServerRunner::GrpcServerRunner(std::string bind_address)
    : bind_address_(std::move(bind_address)) {}

void GrpcServerRunner::add_service(::grpc::Service& service) {
    services_.push_back(&service);
}

void GrpcServerRunner::set_ssl_credentials(const std::string& cert_pem,
                                           const std::string& key_pem) {
    insecure_ = false;
    cert_pem_ = cert_pem;
    key_pem_ = key_pem;
}

void GrpcServerRunner::set_insecure() {
    insecure_ = true;
    cert_pem_.clear();
    key_pem_.clear();
}

int GrpcServerRunner::run_and_block() {
    ::grpc::ServerBuilder builder;

    std::shared_ptr<::grpc::ServerCredentials> credentials;
    if (insecure_) {
        spdlog::warn("Starting gRPC server with insecure credentials");
        credentials = ::grpc::InsecureServerCredentials();
    } else {
        ::grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair{key_pem_, cert_pem_};
        ::grpc::SslServerCredentialsOptions options;
        options.pem_key_cert_pairs.push_back(key_cert_pair);
        credentials = ::grpc::SslServerCredentials(options);
    }

    builder.AddListeningPort(bind_address_, credentials);
    for (auto* service : services_) {
        builder.RegisterService(service);
    }

    auto server = builder.BuildAndStart();
    if (!server) {
        spdlog::error("Failed to start gRPC server on {}", bind_address_);
        return 1;
    }

    spdlog::info("gRPC server listening on {}", bind_address_);
    server->Wait();
    return 0;
}

}  // namespace bcmd::server::adapter::grpc
