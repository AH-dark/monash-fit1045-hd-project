#include "bcmd/client/adapter/grpc/grpc_server_gateway.hpp"
#include "bcmd/client/adapter/tui/ftxui_presenter.hpp"
#include "bcmd/client/adapter/tui/inbox_queue.hpp"
#include "bcmd/client/application/usecase/connect_to_server.hpp"
#include "bcmd/client/application/usecase/join_channel_command.hpp"
#include "bcmd/client/application/usecase/send_message_command.hpp"
#include "bcmd/client/application/usecase/subscribe_command.hpp"
#include "bcmd/shared/logging.hpp"
#include "bcmd/shared/result.hpp"

#include <CLI/CLI.hpp>
#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

std::string read_file(const std::string& path) {
    std::ifstream file{path};
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bcmd::LogLevel select_log_level(bool quiet, bool verbose) noexcept {
    if (quiet) {
        return bcmd::LogLevel::Warn;
    }
    if (verbose) {
        return bcmd::LogLevel::Debug;
    }
    return bcmd::LogLevel::Info;
}

}  // namespace

int main(int argc, char** argv) {
    CLI::App app{"bcli - broadcast messaging client"};

    std::string server_address{"localhost:50051"};
    std::string username;
    std::string ca_cert_path;
    bool insecure{false};
    std::uint32_t replay_count{10};
    bool verbose{false};
    bool quiet{false};

    app.add_option("--server,-s", server_address, "Server address")->default_val("localhost:50051");
    app.add_option("--username,-u", username, "Username")->required();
    app.add_option("--ca", ca_cert_path, "Path to CA certificate (PEM)");
    app.add_flag("--insecure", insecure, "Disable TLS (for local testing only)");
    app.add_option("--replay,-r", replay_count, "Number of history messages to replay")
        ->default_val(10);
    app.add_flag("--verbose,-v", verbose, "Enable debug logging");
    app.add_flag("--quiet,-q", quiet, "Raise log level to warn");

    CLI11_PARSE(app, argc, argv);

    bcmd::init_logging({.level = select_log_level(quiet, verbose)});

    std::shared_ptr<::grpc::ChannelCredentials> credentials;
    if (insecure) {
        spdlog::warn("TLS disabled (--insecure)");
        credentials = ::grpc::InsecureChannelCredentials();
    } else {
        ::grpc::SslCredentialsOptions ssl_options;
        if (!ca_cert_path.empty()) {
            ssl_options.pem_root_certs = read_file(ca_cert_path);
            if (ssl_options.pem_root_certs.empty()) {
                spdlog::error("Failed to read CA certificate: {}", ca_cert_path);
                return 1;
            }
        }
        credentials = ::grpc::SslCredentials(ssl_options);
    }

    namespace grpc_adapter = bcmd::client::adapter::grpc;
    namespace tui_adapter = bcmd::client::adapter::tui;
    namespace usecase = bcmd::client::application::usecase;

    auto channel = ::grpc::CreateChannel(server_address, credentials);
    auto gateway = std::make_shared<grpc_adapter::GrpcServerGateway>(channel);
    auto inbox = std::make_shared<tui_adapter::InboxQueue>();
    auto presenter = std::make_shared<tui_adapter::FtxuiPresenter>(inbox);

    auto connect_uc = std::make_shared<usecase::ConnectToServer>(gateway);
    auto join_uc = std::make_shared<usecase::JoinChannelCommand>(gateway);
    auto send_uc = std::make_shared<usecase::SendMessageCommand>(gateway);
    auto subscribe_uc = std::make_shared<usecase::SubscribeCommand>(gateway, presenter);

    auto client_id_result = connect_uc->execute(username);
    if (!client_id_result.has_value()) {
        spdlog::error("Failed to connect: {}", bcmd::error_message(client_id_result.error()));
        return 1;
    }
    const std::string client_id = *client_id_result;
    presenter->updateConnectionStatus(true, !insecure, username);

    std::mutex channel_mutex;
    std::string active_channel_id;

    presenter->setActions(tui_adapter::FtxuiPresenter::Actions{
        .send_message =
            [send_uc, presenter, client_id, &channel_mutex,
             &active_channel_id](std::string content) {
                std::string channel_id;
                {
                    std::lock_guard<std::mutex> lock{channel_mutex};
                    channel_id = active_channel_id;
                }
                if (channel_id.empty()) {
                    presenter->showError("join a channel before sending");
                    return;
                }
                const auto sent = send_uc->execute(client_id, channel_id, content);
                if (!sent.has_value()) {
                    presenter->showError(bcmd::error_message(sent.error()));
                }
            },
        .join_channel =
            [join_uc, subscribe_uc, presenter, client_id, replay_count, &channel_mutex,
             &active_channel_id](std::string channel_name) {
                if (channel_name.empty()) {
                    presenter->showError("usage: /join <channel>");
                    return;
                }
                auto joined = join_uc->execute(client_id, channel_name);
                if (!joined.has_value()) {
                    presenter->showError(bcmd::error_message(joined.error()));
                    return;
                }
                {
                    std::lock_guard<std::mutex> lock{channel_mutex};
                    active_channel_id = *joined;
                }
                std::thread([subscribe_uc, client_id, channel_id = *joined, replay_count] {
                    (void)subscribe_uc->execute(client_id, channel_id, replay_count);
                }).detach();
            },
        .leave_channel =
            [gateway, presenter, client_id, &channel_mutex, &active_channel_id] {
                std::string channel_id;
                {
                    std::lock_guard<std::mutex> lock{channel_mutex};
                    channel_id = std::exchange(active_channel_id, {});
                }
                if (channel_id.empty()) {
                    presenter->showError("no active channel");
                    return;
                }
                const auto left = gateway->leaveChannel(client_id, channel_id);
                if (!left.has_value()) {
                    presenter->showError(bcmd::error_message(left.error()));
                }
            },
        .list_channels =
            [gateway, presenter] {
                auto channels = gateway->listChannels();
                if (!channels.has_value()) {
                    presenter->showError(bcmd::error_message(channels.error()));
                    return;
                }
                std::vector<std::string> names;
                names.reserve(channels->size());
                for (const auto& channel_info : *channels) {
                    names.push_back(channel_info.name);
                }
                presenter->showChannelList(std::move(names));
            },
    });

    const auto channels = gateway->listChannels();
    if (channels.has_value()) {
        std::vector<std::string> names;
        names.reserve(channels->size());
        for (const auto& channel_info : *channels) {
            names.push_back(channel_info.name);
        }
        presenter->showChannelList(std::move(names));
    }

    return presenter->run([gateway, client_id] { (void)gateway->disconnect(client_id); });
}
