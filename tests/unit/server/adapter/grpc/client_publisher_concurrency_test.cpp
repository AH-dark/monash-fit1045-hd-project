#include "bcmd/server/adapter/grpc/client_publisher.hpp"
#include "bcmd/server/domain/model/message.hpp"
#include "bcmd/server/domain/model/message_content.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/v1/broadcast.pb.h"

#include <catch2/catch_test_macros.hpp>
#include <grpcpp/support/status.h>
#include <grpcpp/support/sync_stream.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <thread>
#include <unordered_set>
#include <vector>

namespace {

class RecordingWriter final : public ::grpc::ServerWriterInterface<bcmd::v1::ChannelEvent> {
public:
    void SendInitialMetadata() override {}

    bool Write(const bcmd::v1::ChannelEvent& /*msg*/, ::grpc::WriteOptions /*options*/) override {
        const auto in_flight = active_writes_.fetch_add(1, std::memory_order_acq_rel) + 1;
        if (in_flight > 1) {
            saw_overlap_.store(true, std::memory_order_release);
        }
        const auto peak_before = max_concurrent_.load(std::memory_order_acquire);
        if (in_flight > peak_before) {
            max_concurrent_.store(in_flight, std::memory_order_release);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        active_writes_.fetch_sub(1, std::memory_order_acq_rel);
        total_writes_.fetch_add(1, std::memory_order_acq_rel);
        return true;
    }

    [[nodiscard]] bool sawOverlap() const { return saw_overlap_.load(std::memory_order_acquire); }
    [[nodiscard]] std::size_t maxConcurrent() const {
        return max_concurrent_.load(std::memory_order_acquire);
    }
    [[nodiscard]] std::size_t totalWrites() const {
        return total_writes_.load(std::memory_order_acquire);
    }

private:
    std::atomic<std::size_t> active_writes_{0};
    std::atomic<std::size_t> max_concurrent_{0};
    std::atomic<std::size_t> total_writes_{0};
    std::atomic<bool> saw_overlap_{false};
};

bcmd::server::domain::Message makeMessage(const bcmd::ClientId& sender,
                                          const bcmd::ChannelId& channel,
                                          const bcmd::server::domain::MessageContent& content) {
    return bcmd::server::domain::Message{bcmd::MessageId::generate(), sender, channel, content};
}

}  // namespace

TEST_CASE("GrpcClientPublisher serializes Write() calls on the same writer",
          "[adapter][grpc][client-publisher][concurrency]") {
    namespace adapter = bcmd::server::adapter::grpc;

    adapter::GrpcClientPublisher publisher;
    const auto recipient_id = bcmd::ClientId::generate();
    const auto channel_id = bcmd::ChannelId::generate();

    auto message_content = bcmd::server::domain::MessageContent::create("stress");
    REQUIRE(message_content.has_value());
    auto username = bcmd::server::domain::Username::create("tester");
    REQUIRE(username.has_value());

    RecordingWriter writer;
    publisher.registerSubscriber(recipient_id, &writer);

    constexpr int thread_count = 8;
    constexpr int writes_per_thread = 250;

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (int thread_index = 0; thread_index < thread_count; ++thread_index) {
        threads.emplace_back([&publisher, &recipient_id, &channel_id, &message_content, &username,
                              thread_index] {
            std::unordered_set<bcmd::ClientId> recipients{recipient_id};
            for (int call_index = 0; call_index < writes_per_thread; ++call_index) {
                switch ((thread_index + call_index) % 3) {
                    case 0:
                        publisher.publish(recipient_id,
                                          makeMessage(recipient_id, channel_id, *message_content));
                        break;
                    case 1:
                        publisher.publishReplayComplete(recipient_id, channel_id);
                        break;
                    default:
                        publisher.broadcastMemberLeft(channel_id, recipients, recipient_id,
                                                      *username);
                        break;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    publisher.unregisterSubscriber(recipient_id);

    CHECK_FALSE(writer.sawOverlap());
    CHECK(writer.maxConcurrent() <= 1);
    CHECK(writer.totalWrites() == static_cast<std::size_t>(thread_count * writes_per_thread));
}

TEST_CASE("GrpcClientPublisher allows concurrent Write() calls across different writers",
          "[adapter][grpc][client-publisher][concurrency]") {
    namespace adapter = bcmd::server::adapter::grpc;

    adapter::GrpcClientPublisher publisher;
    const auto channel_id = bcmd::ChannelId::generate();

    auto message_content = bcmd::server::domain::MessageContent::create("per-writer");
    REQUIRE(message_content.has_value());

    constexpr int writer_count = 4;
    constexpr int writes_per_thread = 250;

    std::vector<std::unique_ptr<RecordingWriter>> writers;
    std::vector<bcmd::ClientId> recipients;
    writers.reserve(writer_count);
    recipients.reserve(writer_count);
    for (int writer_index = 0; writer_index < writer_count; ++writer_index) {
        writers.push_back(std::make_unique<RecordingWriter>());
        recipients.push_back(bcmd::ClientId::generate());
        publisher.registerSubscriber(recipients.back(), writers.back().get());
    }

    std::vector<std::thread> threads;
    threads.reserve(static_cast<std::size_t>(writer_count));
    for (int thread_index = 0; thread_index < writer_count; ++thread_index) {
        threads.emplace_back(
            [&publisher, &recipients, &channel_id, &message_content, thread_index] {
                const auto& recipient_id = recipients[static_cast<std::size_t>(thread_index)];
                for (int call_index = 0; call_index < writes_per_thread; ++call_index) {
                    publisher.publish(recipient_id,
                                      makeMessage(recipient_id, channel_id, *message_content));
                }
            });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    for (std::size_t writer_index = 0; writer_index < writers.size(); ++writer_index) {
        publisher.unregisterSubscriber(recipients[writer_index]);
        CHECK_FALSE(writers[writer_index]->sawOverlap());
        CHECK(writers[writer_index]->totalWrites() == static_cast<std::size_t>(writes_per_thread));
    }
}

TEST_CASE("GrpcClientPublisher unregisterSubscriber waits for in-flight Write() to complete",
          "[adapter][grpc][client-publisher][concurrency]") {
    namespace adapter = bcmd::server::adapter::grpc;

    adapter::GrpcClientPublisher publisher;
    const auto recipient_id = bcmd::ClientId::generate();
    const auto channel_id = bcmd::ChannelId::generate();

    auto message_content = bcmd::server::domain::MessageContent::create("race");
    REQUIRE(message_content.has_value());

    RecordingWriter writer;
    publisher.registerSubscriber(recipient_id, &writer);

    constexpr int worker_count = 4;
    std::atomic<bool> stop{false};
    std::vector<std::thread> workers;
    workers.reserve(worker_count);
    for (int worker_index = 0; worker_index < worker_count; ++worker_index) {
        workers.emplace_back([&publisher, &recipient_id, &channel_id, &message_content, &stop] {
            while (!stop.load(std::memory_order_acquire)) {
                publisher.publish(recipient_id,
                                  makeMessage(recipient_id, channel_id, *message_content));
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    publisher.unregisterSubscriber(recipient_id);
    stop.store(true, std::memory_order_release);
    for (auto& worker : workers) {
        worker.join();
    }

    CHECK_FALSE(writer.sawOverlap());
}
