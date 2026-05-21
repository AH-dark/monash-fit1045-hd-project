#include "bcmd/client/adapter/grpc/grpc_server_gateway.hpp"
#include "bcmd/client/adapter/tui/ftxui_presenter.hpp"
#include "bcmd/client/adapter/tui/inbox_queue.hpp"
#include "bcmd/client/application/port/i_server_gateway.hpp"
#include "bcmd/client/application/usecase/connect_to_server.hpp"
#include "bcmd/client/application/usecase/create_channel_command.hpp"
#include "bcmd/client/application/usecase/join_channel_command.hpp"
#include "bcmd/client/application/usecase/send_message_command.hpp"
#include "bcmd/client/application/usecase/subscribe_command.hpp"
#include "bcmd/shared/logging.hpp"
#include "bcmd/shared/result.hpp"
#include "bcmd/shared/string_utils.hpp"

#include <CLI/CLI.hpp>
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <cstdio>
#include <exception>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

namespace grpc_adapter = bcmd::client::adapter::grpc;
namespace tui_adapter = bcmd::client::adapter::tui;
namespace port = bcmd::client::application::port;
namespace usecase = bcmd::client::application::usecase;

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

// Owns mutable per-connection state and routes user actions to use cases.
// Held via shared_ptr so callback lambdas can capture it without storing
// throwable string copies in their closure types.
class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(std::shared_ptr<usecase::JoinChannelCommand> join_uc,
                  std::shared_ptr<usecase::CreateChannelCommand> create_uc,
                  std::shared_ptr<usecase::SendMessageCommand> send_uc,
                  std::shared_ptr<usecase::SubscribeCommand> subscribe_uc,
                  std::shared_ptr<port::IServerGateway> gateway,
                  std::shared_ptr<tui_adapter::FtxuiPresenter> presenter, std::string client_id,
                  std::uint32_t replay_count)
        : join_uc_{std::move(join_uc)},
          create_uc_{std::move(create_uc)},
          send_uc_{std::move(send_uc)},
          subscribe_uc_{std::move(subscribe_uc)},
          gateway_{std::move(gateway)},
          presenter_{std::move(presenter)},
          client_id_{std::move(client_id)},
          replay_count_{replay_count} {}

    void sendMessage(const std::string& content) {
        std::string channel_id;
        {
            std::scoped_lock lock{channel_mutex_};
            channel_id = active_channel_id_;
        }
        if (channel_id.empty()) {
            presenter_->showError("join a channel before sending");
            return;
        }
        const auto sent = send_uc_->execute(client_id_, channel_id, content);
        if (!sent.has_value()) {
            presenter_->showError(bcmd::error_message(sent.error()));
        }
    }

    void joinChannel(const std::string& channel_name) {
        const auto trimmed = bcmd::trim(channel_name);
        if (trimmed.empty()) {
            presenter_->showError("usage: /join <channel>");
            return;
        }
        auto joined = join_uc_->execute(client_id_, trimmed);
        if (!joined.has_value()) {
            presenter_->showError(bcmd::error_message(joined.error()));
            return;
        }
        {
            std::scoped_lock lock{channel_mutex_};
            active_channel_id_ = *joined;
        }
        std::thread{&ClientSession::runSubscription, shared_from_this(), std::move(*joined)}
            .detach();
    }

    void createChannel(const std::string& channel_name) {
        const auto trimmed = bcmd::trim(channel_name);
        if (trimmed.empty()) {
            presenter_->showError("usage: /create <channel>");
            return;
        }
        auto created = create_uc_->execute(client_id_, trimmed);
        if (!created.has_value()) {
            presenter_->showError(bcmd::error_message(created.error()));
            return;
        }
        gateway_->listChannels();
        presenter_->showError("channel created: " + std::string{trimmed} + " (use /join to enter)");
    }

    void leaveChannel() {
        std::string channel_id;
        {
            std::scoped_lock lock{channel_mutex_};
            channel_id = std::exchange(active_channel_id_, {});
        }
        if (channel_id.empty()) {
            presenter_->showError("no active channel");
            return;
        }
        const auto left = gateway_->leaveChannel(client_id_, channel_id);
        if (!left.has_value()) {
            presenter_->showError(bcmd::error_message(left.error()));
        }
    }

    void listChannels() {
        auto channels = gateway_->listChannels();
        if (!channels.has_value()) {
            presenter_->showError(bcmd::error_message(channels.error()));
            return;
        }
        std::vector<std::string> names;
        names.reserve(channels->size());
        for (const auto& channel_info : *channels) {
            names.push_back(channel_info.name);
        }
        presenter_->showChannelList(std::move(names));
    }

    void disconnect() {
        const auto result = gateway_->disconnect(client_id_);
        if (!result.has_value()) {
            spdlog::error("disconnect failed: {}", bcmd::error_message(result.error()));
        }
    }

private:
    void runSubscription(const std::string& channel_id) {
        const auto result = subscribe_uc_->execute(client_id_, channel_id, replay_count_);
        if (!result.has_value()) {
            spdlog::error("subscription ended: {}", bcmd::error_message(result.error()));
        }
    }

    std::shared_ptr<usecase::JoinChannelCommand> join_uc_;
    std::shared_ptr<usecase::CreateChannelCommand> create_uc_;
    std::shared_ptr<usecase::SendMessageCommand> send_uc_;
    std::shared_ptr<usecase::SubscribeCommand> subscribe_uc_;
    std::shared_ptr<port::IServerGateway> gateway_;
    std::shared_ptr<tui_adapter::FtxuiPresenter> presenter_;
    std::string client_id_;
    std::uint32_t replay_count_;

    std::mutex channel_mutex_;
    std::string active_channel_id_;
};

std::shared_ptr<::grpc::ChannelCredentials> build_credentials(bool insecure,
                                                              const std::string& ca_cert_path) {
    if (insecure) {
        spdlog::warn("TLS disabled (--insecure)");
        return ::grpc::InsecureChannelCredentials();
    }
    ::grpc::SslCredentialsOptions ssl_options;
    if (!ca_cert_path.empty()) {
        ssl_options.pem_root_certs = read_file(ca_cert_path);
        if (ssl_options.pem_root_certs.empty()) {
            spdlog::error("Failed to read CA certificate: {}", ca_cert_path);
            return nullptr;
        }
    }
    return ::grpc::SslCredentials(ssl_options);
}

int run(int argc, char** argv) {
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

    server_address = bcmd::trim_copy(server_address);
    username = bcmd::trim_copy(username);
    ca_cert_path = bcmd::trim_copy(ca_cert_path);

    bcmd::init_logging({.level = select_log_level(quiet, verbose)});

    auto credentials = build_credentials(insecure, ca_cert_path);
    if (!credentials) {
        return 1;
    }

    auto channel = ::grpc::CreateChannel(server_address, credentials);
    auto gateway = std::make_shared<grpc_adapter::GrpcServerGateway>(channel);
    auto inbox = std::make_shared<tui_adapter::InboxQueue>();
    auto presenter = std::make_shared<tui_adapter::FtxuiPresenter>(inbox);

    auto connect_uc = std::make_shared<usecase::ConnectToServer>(gateway);
    auto join_uc = std::make_shared<usecase::JoinChannelCommand>(gateway);
    auto create_uc = std::make_shared<usecase::CreateChannelCommand>(gateway);
    auto send_uc = std::make_shared<usecase::SendMessageCommand>(gateway);
    auto subscribe_uc = std::make_shared<usecase::SubscribeCommand>(gateway, presenter);

    auto client_id_result = connect_uc->execute(username);
    if (!client_id_result.has_value()) {
        spdlog::error("Failed to connect: {}", bcmd::error_message(client_id_result.error()));
        return 1;
    }
    const std::string& client_id = *client_id_result;
    presenter->updateConnectionStatus(true, !insecure, username);

    auto session = std::make_shared<ClientSession>(join_uc, create_uc, send_uc, subscribe_uc,
                                                   gateway, presenter, client_id, replay_count);

    presenter->setActions(tui_adapter::FtxuiPresenter::Actions{
        .send_message = [session](const std::string& content) { session->sendMessage(content); },
        .join_channel = [session](const std::string& name) { session->joinChannel(name); },
        .create_channel = [session](const std::string& name) { session->createChannel(name); },
        .leave_channel = [session] { session->leaveChannel(); },
        .list_channels = [session] { session->listChannels(); },
    });

    session->listChannels();

    return presenter->run([session] { session->disconnect(); });
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& ex) {
        std::fputs("Fatal: ", stderr);
        std::fputs(ex.what(), stderr);
        std::fputs("\n", stderr);
        return 1;
    } catch (...) {
        std::fputs("Fatal: unknown exception\n", stderr);
        return 1;
    }
}
