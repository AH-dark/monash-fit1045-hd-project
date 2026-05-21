#include <catch2/catch_test_macros.hpp>

#include <chrono>

#include "integration/integration_test_utils.hpp"

namespace {

::grpc::Status try_connect(bcmd::tests::integration::BroadcastStub& stub,
                           std::string_view username) {
    ::grpc::ClientContext context;
    bcmd::tests::integration::set_timeout(context, std::chrono::seconds{2});
    bcmd::v1::ConnectRequest request;
    bcmd::v1::ConnectResponse response;
    request.set_username(std::string{username});
    return stub.Connect(&context, request, &response);
}

}  // namespace

TEST_CASE("TLS server rejects insecure clients and accepts trusted TLS clients",
          "[integration][grpc][tls]") {
    namespace integration = bcmd::tests::integration;

    integration::TestServer tls_server;
    auto insecure_stub = integration::make_stub(integration::make_insecure_channel(tls_server.address()));
    auto tls_stub = integration::make_stub(integration::make_tls_channel(tls_server.address()));

    CHECK_FALSE(try_connect(*insecure_stub, "alice").ok());
    CHECK(try_connect(*tls_stub, "alice").ok());
}

TEST_CASE("insecure server mode allows insecure clients", "[integration][grpc][tls]") {
    namespace integration = bcmd::tests::integration;

    integration::TestServer insecure_server{/*insecure=*/true};
    auto stub = integration::make_stub(integration::make_insecure_channel(insecure_server.address()));

    CHECK(try_connect(*stub, "local").ok());
}
