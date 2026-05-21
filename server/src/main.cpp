#include <CLI/CLI.hpp>

#include <cstdint>
#include <memory>
#include <string>

#include <spdlog/spdlog.h>

#include "bcmd/server/adapter/grpc/broadcast_service_impl.hpp"
#include "bcmd/server/adapter/grpc/client_publisher.hpp"
#include "bcmd/server/adapter/grpc/server_runner.hpp"
#include "bcmd/server/adapter/grpc/tls_config.hpp"
#include "bcmd/server/adapter/persistence/in_memory_channel_repository.hpp"
#include "bcmd/server/adapter/persistence/in_memory_client_registry.hpp"
#include "bcmd/server/adapter/persistence/in_memory_event_log.hpp"
#include "bcmd/server/adapter/persistence/in_memory_message_repository.hpp"
#include "bcmd/server/application/usecase/get_recent_messages.hpp"
#include "bcmd/server/application/usecase/join_channel.hpp"
#include "bcmd/server/application/usecase/leave_channel.hpp"
#include "bcmd/server/application/usecase/list_channels.hpp"
#include "bcmd/server/application/usecase/send_message.hpp"
#include "bcmd/server/application/usecase/subscribe_to_channel.hpp"
#include "bcmd/shared/logging.hpp"

int main(int argc, char** argv) {
    CLI::App app{"bcmd - broadcast messaging server"};

    std::string bind_address{"0.0.0.0:50051"};
    std::string cert_path;
    std::string key_path;
    bool insecure{false};
    std::uint32_t history_cap{200};
    bool verbose{false};
    bool quiet{false};

    app.add_option("--bind,-b", bind_address, "Bind address")->default_val("0.0.0.0:50051");
    app.add_option("--cert", cert_path, "TLS certificate PEM");
    app.add_option("--key", key_path, "TLS private key PEM");
    app.add_flag("--insecure", insecure, "Disable TLS");
    app.add_option("--history-cap", history_cap, "Max messages per channel")->default_val(200);
    app.add_flag("--verbose,-v", verbose, "Enable debug logging");
    app.add_flag("--quiet,-q", quiet, "Raise log level to warn");

    CLI11_PARSE(app, argc, argv);

    const auto LOG_LEVEL = [&] {
        if (quiet) {
            return bcmd::LogLevel::Warn;
        }
        if (verbose) {
            return bcmd::LogLevel::Debug;
        }
        return bcmd::LogLevel::Info;
    }();
    bcmd::init_logging({.level = LOG_LEVEL});

    if (!insecure && (cert_path.empty() || key_path.empty())) {
        spdlog::error("--cert and --key required unless --insecure");
        return 1;
    }

    namespace persistence = bcmd::server::adapter::persistence;
    namespace grpc_adapter = bcmd::server::adapter::grpc;
    namespace usecase = bcmd::server::application::usecase;

    auto channel_repo = std::make_shared<persistence::InMemoryChannelRepository>();
    auto client_registry = std::make_shared<persistence::InMemoryClientRegistry>();
    auto message_repo = std::make_shared<persistence::InMemoryMessageRepository>(history_cap);
    auto event_log = std::make_shared<persistence::InMemoryEventLog>();
    auto publisher = std::make_shared<grpc_adapter::GrpcClientPublisher>();
    (void)event_log;

    auto join_channel = std::make_shared<usecase::JoinChannel>(channel_repo, client_registry);
    auto leave_channel = std::make_shared<usecase::LeaveChannel>(channel_repo, client_registry);
    auto send_message =
        std::make_shared<usecase::SendMessage>(channel_repo, client_registry, message_repo, publisher);
    auto list_channels = std::make_shared<usecase::ListChannels>(channel_repo);
    auto get_recent = std::make_shared<usecase::GetRecentMessages>(message_repo);
    auto subscribe =
        std::make_shared<usecase::SubscribeToChannel>(channel_repo, message_repo, publisher);

    grpc_adapter::BroadcastServiceImpl service{join_channel, leave_channel, send_message,
                                               list_channels, get_recent, subscribe,
                                               publisher, client_registry};

    grpc_adapter::GrpcServerRunner runner{bind_address};
    runner.add_service(service);

    if (insecure) {
        spdlog::warn("TLS disabled (--insecure). NOT for production.");
        runner.set_insecure();
    } else {
        auto cert_pem = grpc_adapter::read_pem_file(cert_path);
        auto key_pem = grpc_adapter::read_pem_file(key_path);
        if (!cert_pem.has_value() || !key_pem.has_value()) {
            spdlog::error("Failed to read TLS certificate or key");
            return 1;
        }
        runner.set_ssl_credentials(*cert_pem, *key_pem);
    }

    spdlog::info("Starting bcmd on {}", bind_address);
    return runner.run_and_block();
}
