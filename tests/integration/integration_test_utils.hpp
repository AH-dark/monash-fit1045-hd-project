#pragma once

#include "bcmd/server/adapter/grpc/broadcast_service_impl.hpp"
#include "bcmd/server/adapter/grpc/channel_list_publisher.hpp"
#include "bcmd/server/adapter/grpc/client_publisher.hpp"
#include "bcmd/server/adapter/grpc/heartbeat_sweeper.hpp"
#include "bcmd/server/adapter/grpc/server_runner.hpp"
#include "bcmd/server/adapter/persistence/in_memory_channel_repository.hpp"
#include "bcmd/server/adapter/persistence/in_memory_client_registry.hpp"
#include "bcmd/server/adapter/persistence/in_memory_message_repository.hpp"
#include "bcmd/server/application/usecase/create_channel.hpp"
#include "bcmd/server/application/usecase/expire_inactive_clients.hpp"
#include "bcmd/server/application/usecase/join_channel.hpp"
#include "bcmd/server/application/usecase/leave_channel.hpp"
#include "bcmd/server/application/usecase/list_channels.hpp"
#include "bcmd/server/application/usecase/list_messages.hpp"
#include "bcmd/server/application/usecase/send_message.hpp"
#include "bcmd/server/application/usecase/subscribe_to_channel.hpp"
#include "bcmd/server/application/usecase/subscribe_to_channel_list.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/v1/broadcast.grpc.pb.h"
#include "bcmd/v1/broadcast.pb.h"

#include <catch2/catch_test_macros.hpp>
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/support/status.h>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace bcmd::tests::integration {

using BroadcastStub = bcmd::v1::BroadcastService::Stub;
using Clock = std::chrono::system_clock;

inline std::filesystem::path project_root() {
    auto current = std::filesystem::current_path();
    while (!current.empty()) {
        if (std::filesystem::exists(current / "certs" / "test" / "ca.pem")) {
            return current;
        }
        current = current.parent_path();
    }
    return std::filesystem::current_path();
}

inline std::string read_file(const std::filesystem::path& path) {
    std::ifstream file{path};
    REQUIRE(file.good());
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

inline std::shared_ptr<::grpc::Channel> make_tls_channel(std::string_view address) {
    ::grpc::SslCredentialsOptions options;
    options.pem_root_certs = read_file(project_root() / "certs" / "test" / "ca.pem");
    return ::grpc::CreateChannel(std::string{address}, ::grpc::SslCredentials(options));
}

inline std::shared_ptr<::grpc::Channel> make_insecure_channel(std::string_view address) {
    return ::grpc::CreateChannel(std::string{address}, ::grpc::InsecureChannelCredentials());
}

class TestServer {
public:
    explicit TestServer(bool insecure = false,
                        std::chrono::seconds heartbeat_timeout = std::chrono::seconds{0},
                        std::chrono::seconds heartbeat_sweep_interval = std::chrono::seconds{1}) {
        namespace grpc_adapter = bcmd::server::adapter::grpc;
        namespace persistence = bcmd::server::adapter::persistence;
        namespace usecase = bcmd::server::application::usecase;

        auto channel_repo = std::make_shared<persistence::InMemoryChannelRepository>();
        auto client_registry = std::make_shared<persistence::InMemoryClientRegistry>();
        auto message_repo = std::make_shared<persistence::InMemoryMessageRepository>();
        auto publisher = std::make_shared<grpc_adapter::GrpcClientPublisher>(client_registry);
        auto channel_list_publisher =
            std::make_shared<grpc_adapter::GrpcChannelListPublisher>(channel_repo);

        auto join_channel = std::make_shared<usecase::JoinChannel>(
            channel_repo, client_registry, publisher, channel_list_publisher);
        auto leave_channel = std::make_shared<usecase::LeaveChannel>(
            channel_repo, client_registry, publisher, channel_list_publisher);
        auto send_message = std::make_shared<usecase::SendMessage>(channel_repo, client_registry,
                                                                   message_repo, publisher);
        auto list_channels = std::make_shared<usecase::ListChannels>(channel_repo);
        auto create_channel = std::make_shared<usecase::CreateChannel>(
            channel_repo, client_registry, channel_list_publisher);
        auto list_messages = std::make_shared<usecase::ListMessages>(channel_repo, message_repo);
        auto subscribe = std::make_shared<usecase::SubscribeToChannel>(channel_repo);
        auto subscribe_channel_list = std::make_shared<usecase::SubscribeToChannelList>(
            client_registry, channel_list_publisher);

        service_ = std::make_unique<grpc_adapter::BroadcastServiceImpl>(
            join_channel, leave_channel, send_message, list_channels, create_channel, list_messages,
            subscribe, publisher, client_registry, subscribe_channel_list, channel_list_publisher);
        runner_ = std::make_unique<grpc_adapter::GrpcServerRunner>("127.0.0.1:0");
        runner_->add_service(*service_);

        if (insecure) {
            runner_->set_insecure();
        } else {
            const auto cert_path = project_root() / "certs" / "test" / "server.pem";
            const auto key_path = project_root() / "certs" / "test" / "server.key";
            runner_->set_ssl_credentials(read_file(cert_path), read_file(key_path));
        }

        if (heartbeat_timeout > std::chrono::seconds{0}) {
            auto expire = std::make_shared<usecase::ExpireInactiveClients>(client_registry,
                                                                           channel_repo, publisher);
            sweeper_ = std::make_unique<grpc_adapter::HeartbeatSweeper>(
                std::move(expire), heartbeat_sweep_interval, heartbeat_timeout);
        }

        thread_ = std::thread([this] { (void)runner_->run_and_block(); });
        REQUIRE(runner_->wait_until_started(std::chrono::seconds{3}));
    }

    TestServer(TestServer&&) = delete;
    TestServer& operator=(TestServer&&) = delete;
    ~TestServer() {
        sweeper_.reset();
        if (runner_ != nullptr) {
            runner_->shutdown();
        }
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    TestServer(const TestServer&) = delete;
    TestServer& operator=(const TestServer&) = delete;

    std::string address() const { return runner_->bound_address(); }

private:
    std::unique_ptr<bcmd::server::adapter::grpc::BroadcastServiceImpl> service_;
    std::unique_ptr<bcmd::server::adapter::grpc::GrpcServerRunner> runner_;
    std::unique_ptr<bcmd::server::adapter::grpc::HeartbeatSweeper> sweeper_;
    std::thread thread_;
};

inline std::unique_ptr<BroadcastStub> make_stub(std::shared_ptr<::grpc::Channel> channel) {
    return bcmd::v1::BroadcastService::NewStub(std::move(channel));
}

inline void set_timeout(::grpc::ClientContext& context, std::chrono::milliseconds timeout) {
    context.set_deadline(Clock::now() + timeout);
}

inline std::string connect(BroadcastStub& stub, std::string_view username) {
    ::grpc::ClientContext context;
    set_timeout(context, std::chrono::seconds{2});
    bcmd::v1::ConnectRequest request;
    bcmd::v1::ConnectResponse response;
    request.set_username(std::string{username});

    const auto status = stub.Connect(&context, request, &response);
    REQUIRE(status.ok());
    REQUIRE_FALSE(response.client_id().empty());
    return response.client_id();
}

inline std::string join_channel(BroadcastStub& stub, std::string_view client_id,
                                std::string_view channel) {
    if (!bcmd::ChannelId::parse(channel).has_value()) {
        ::grpc::ClientContext list_context;
        set_timeout(list_context, std::chrono::seconds{2});
        bcmd::v1::ListChannelsRequest list_request;
        bcmd::v1::ListChannelsResponse list_response;
        REQUIRE(stub.ListChannels(&list_context, list_request, &list_response).ok());
        bool already_exists = false;
        for (const auto& summary : list_response.channels()) {
            if (summary.name() == channel) {
                already_exists = true;
                break;
            }
        }
        if (!already_exists) {
            ::grpc::ClientContext create_context;
            set_timeout(create_context, std::chrono::seconds{2});
            bcmd::v1::CreateChannelRequest create_request;
            bcmd::v1::CreateChannelResponse create_response;
            create_request.set_client_id(std::string{client_id});
            create_request.set_channel_name(std::string{channel});
            REQUIRE(stub.CreateChannel(&create_context, create_request, &create_response).ok());
        }
    }

    ::grpc::ClientContext join_context;
    set_timeout(join_context, std::chrono::seconds{2});
    bcmd::v1::JoinChannelRequest join_request;
    bcmd::v1::JoinChannelResponse join_response;
    join_request.set_client_id(std::string{client_id});
    join_request.set_channel_id(std::string{channel});
    REQUIRE(stub.JoinChannel(&join_context, join_request, &join_response).ok());

    if (bcmd::ChannelId::parse(channel).has_value()) {
        return std::string{channel};
    }

    ::grpc::ClientContext list_context;
    set_timeout(list_context, std::chrono::seconds{2});
    bcmd::v1::ListChannelsRequest list_request;
    bcmd::v1::ListChannelsResponse list_response;
    REQUIRE(stub.ListChannels(&list_context, list_request, &list_response).ok());
    for (const auto& summary : list_response.channels()) {
        if (summary.name() == channel) {
            return summary.id();
        }
    }
    FAIL("joined channel was not visible in ListChannels");
    return {};
}

inline ::grpc::Status send_heartbeat(BroadcastStub& stub, std::string_view client_id) {
    ::grpc::ClientContext context;
    set_timeout(context, std::chrono::seconds{2});
    bcmd::v1::HeartbeatRequest request;
    bcmd::v1::HeartbeatResponse response;
    request.set_client_id(std::string{client_id});
    return stub.Heartbeat(&context, request, &response);
}

inline ::grpc::Status disconnect(BroadcastStub& stub, std::string_view client_id) {
    ::grpc::ClientContext context;
    set_timeout(context, std::chrono::seconds{2});
    bcmd::v1::DisconnectRequest request;
    bcmd::v1::DisconnectResponse response;
    request.set_client_id(std::string{client_id});
    return stub.Disconnect(&context, request, &response);
}

inline void send_message(BroadcastStub& stub, std::string_view client_id,
                         std::string_view channel_id, std::string_view content) {
    ::grpc::ClientContext context;
    set_timeout(context, std::chrono::seconds{2});
    bcmd::v1::SendMessageRequest request;
    bcmd::v1::SendMessageResponse response;
    request.set_client_id(std::string{client_id});
    request.set_channel_id(std::string{channel_id});
    request.set_content(std::string{content});
    REQUIRE(stub.SendMessage(&context, request, &response).ok());
    REQUIRE_FALSE(response.message_id().empty());
}

class Subscription {
public:
    Subscription(BroadcastStub& stub, std::string client_id, std::string channel_id,
                 std::uint32_t /*replay_count*/ = 0) {
        bcmd::v1::SubscribeRequest request;
        request.set_client_id(std::move(client_id));
        request.set_channel_id(std::move(channel_id));
        reader_ = stub.SubscribeToChannel(&context_, request);
        thread_ = std::thread([this] { read_loop(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    ~Subscription() { stop(); }

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    Subscription(Subscription&&) = delete;
    Subscription& operator=(Subscription&&) = delete;

    void stop() {
        context_.TryCancel();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    bool wait_for_events(std::size_t count, std::chrono::milliseconds timeout) {
        std::unique_lock lock(mutex_);
        return cv_.wait_for(lock, timeout, [this, count] { return events_.size() >= count; });
    }

    bool wait_for(std::chrono::milliseconds timeout,
                  bool (*predicate)(const std::vector<bcmd::v1::ChannelEvent>&)) {
        std::unique_lock lock(mutex_);
        return cv_.wait_for(lock, timeout, [this, predicate] { return predicate(events_); });
    }

    std::vector<bcmd::v1::ChannelEvent> events() const {
        const std::lock_guard lock(mutex_);
        return events_;
    }

    ::grpc::Status finish_status() const {
        const std::lock_guard lock(mutex_);
        return status_;
    }

private:
    void read_loop() {
        bcmd::v1::ChannelEvent event;
        while (reader_->Read(&event)) {
            {
                const std::lock_guard lock(mutex_);
                events_.push_back(event);
            }
            cv_.notify_all();
        }
        const auto status = reader_->Finish();
        {
            const std::lock_guard lock(mutex_);
            status_ = status;
        }
        cv_.notify_all();
    }

    ::grpc::ClientContext context_;
    std::unique_ptr<::grpc::ClientReader<bcmd::v1::ChannelEvent>> reader_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<bcmd::v1::ChannelEvent> events_;
    ::grpc::Status status_;
    std::thread thread_;
};

inline std::size_t count_messages(const std::vector<bcmd::v1::ChannelEvent>& events,
                                  std::string_view content) {
    std::size_t count{0};
    for (const auto& event : events) {
        if (event.has_message() && event.message().content() == content) {
            ++count;
        }
    }
    return count;
}

}  // namespace bcmd::tests::integration
