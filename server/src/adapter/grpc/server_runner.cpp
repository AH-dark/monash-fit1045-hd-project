#include "bcmd/server/adapter/grpc/server_runner.hpp"

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace bcmd::server::adapter::grpc {

GrpcServerRunner::GrpcServerRunner(std::string bind_address)
    : bind_address_(std::move(bind_address)) {}

void GrpcServerRunner::add_service(::grpc::Service& service) { services_.push_back(&service); }

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
        ::grpc::SslServerCredentialsOptions::PemKeyCertPair keyCertPair{.private_key = key_pem_,
                                                                        .cert_chain = cert_pem_};
        ::grpc::SslServerCredentialsOptions options;
        options.pem_key_cert_pairs.push_back(keyCertPair);
        credentials = ::grpc::SslServerCredentials(options);
    }

    int selectedPort{0};
    builder.AddListeningPort(bind_address_, credentials, &selectedPort);
    for (auto* service : services_) {
        builder.RegisterService(service);
    }

    auto server = builder.BuildAndStart();
    if (!server) {
        spdlog::error("Failed to start gRPC server on {}", bind_address_);
        {
            std::scoped_lock lock(mutex_);
            started_ = false;
        }
        started_cv_.notify_all();
        return 1;
    }

    {
        std::scoped_lock lock(mutex_);
        server_ = std::move(server);
        auto colon = bind_address_.rfind(':');
        auto host = colon == std::string::npos ? bind_address_ : bind_address_.substr(0, colon);
        bound_address_ = std::move(host) + ":" + std::to_string(selectedPort);
        started_ = true;
    }
    started_cv_.notify_all();

    spdlog::info("gRPC server listening on {}", bound_address());
    server_->Wait();
    {
        std::scoped_lock lock(mutex_);
        server_.reset();
        started_ = false;
    }
    return 0;
}

void GrpcServerRunner::shutdown() {
    std::unique_lock lock(mutex_);
    auto* server = server_.get();
    lock.unlock();
    if (server != nullptr) {
        server->Shutdown();
    }
}

std::string GrpcServerRunner::bound_address() const {
    std::scoped_lock lock(mutex_);
    return bound_address_;
}

bool GrpcServerRunner::wait_until_started(std::chrono::milliseconds timeout) {
    std::unique_lock lock(mutex_);
    return started_cv_.wait_for(lock, timeout, [this] { return started_; });
}

}  // namespace bcmd::server::adapter::grpc
