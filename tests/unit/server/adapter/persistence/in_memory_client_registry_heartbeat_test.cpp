#include "bcmd/server/adapter/persistence/in_memory_client_registry.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <thread>

namespace {

using bcmd::server::adapter::persistence::InMemoryClientRegistry;
using bcmd::server::domain::Username;

bcmd::ClientId registerClient(InMemoryClientRegistry& registry, const char* name) {
    auto username = Username::create(name);
    REQUIRE(username.has_value());
    auto session = registry.registerClient(*username);
    REQUIRE(session.has_value());
    return session->id();
}

}  // namespace

TEST_CASE("InMemoryClientRegistry stamps a recent lastHeartbeatAt on registration",
          "[adapter][persistence][in-memory-client-registry][heartbeat]") {
    InMemoryClientRegistry registry;
    const auto before = std::chrono::steady_clock::now();

    const auto client_id = registerClient(registry, "alice");

    const auto after = std::chrono::steady_clock::now();
    auto session = registry.findById(client_id);
    REQUIRE(session.has_value());
    const auto heartbeat = session->lastHeartbeatAt();
    CHECK(heartbeat >= before);
    CHECK(heartbeat <= after);
}

TEST_CASE("InMemoryClientRegistry::touchHeartbeat advances the timestamp",
          "[adapter][persistence][in-memory-client-registry][heartbeat]") {
    InMemoryClientRegistry registry;
    const auto client_id = registerClient(registry, "alice");
    auto before_session = registry.findById(client_id);
    REQUIRE(before_session.has_value());
    const auto before = before_session->lastHeartbeatAt();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const auto touched = registry.touchHeartbeat(client_id);

    REQUIRE(touched.has_value());
    auto after_session = registry.findById(client_id);
    REQUIRE(after_session.has_value());
    CHECK(after_session->lastHeartbeatAt() > before);
}

TEST_CASE("InMemoryClientRegistry::touchHeartbeat returns ClientNotFound for an unknown id",
          "[adapter][persistence][in-memory-client-registry][heartbeat]") {
    InMemoryClientRegistry registry;
    const auto unknown = bcmd::ClientId::generate();

    const auto result = registry.touchHeartbeat(unknown);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::ClientNotFound);
}

TEST_CASE("InMemoryClientRegistry::collectExpired returns sessions older than the deadline only",
          "[adapter][persistence][in-memory-client-registry][heartbeat]") {
    InMemoryClientRegistry registry;
    const auto alice = registerClient(registry, "alice");
    const auto bob = registerClient(registry, "bob");
    const auto carol = registerClient(registry, "carol");

    REQUIRE(registry.touchHeartbeat(carol).has_value());
    auto carol_session = registry.findById(carol);
    REQUIRE(carol_session.has_value());
    const auto deadline = carol_session->lastHeartbeatAt();

    const auto expired = registry.collectExpired(deadline);

    REQUIRE(expired.size() == 2);
    bool has_alice = false;
    bool has_bob = false;
    bool has_carol = false;
    for (const auto& session : expired) {
        if (session.id() == alice) has_alice = true;
        if (session.id() == bob) has_bob = true;
        if (session.id() == carol) has_carol = true;
    }
    CHECK(has_alice);
    CHECK(has_bob);
    CHECK_FALSE(has_carol);
}

TEST_CASE("InMemoryClientRegistry::collectExpired returns copies that survive later mutations",
          "[adapter][persistence][in-memory-client-registry][heartbeat]") {
    InMemoryClientRegistry registry;
    const auto alice = registerClient(registry, "alice");
    const auto bob = registerClient(registry, "bob");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::hours(1);

    const auto snapshot = registry.collectExpired(deadline);
    REQUIRE(snapshot.size() == 2);

    REQUIRE(registry.remove(alice).has_value());
    REQUIRE(registry.remove(bob).has_value());

    CHECK(snapshot.size() == 2);
    bool snapshot_has_alice = false;
    bool snapshot_has_bob = false;
    for (const auto& session : snapshot) {
        if (session.id() == alice) snapshot_has_alice = true;
        if (session.id() == bob) snapshot_has_bob = true;
    }
    CHECK(snapshot_has_alice);
    CHECK(snapshot_has_bob);

    CHECK(registry.collectExpired(deadline).empty());
}

TEST_CASE("InMemoryClientRegistry::lookupUsername returns the registered username",
          "[adapter][persistence][in-memory-client-registry][lookup-username]") {
    InMemoryClientRegistry registry;
    const auto client_id = registerClient(registry, "alice");

    auto username = registry.lookupUsername(client_id);

    REQUIRE(username.has_value());
    CHECK(username->value() == "alice");
}

TEST_CASE("InMemoryClientRegistry::lookupUsername survives remove",
          "[adapter][persistence][in-memory-client-registry][lookup-username]") {
    InMemoryClientRegistry registry;
    const auto client_id = registerClient(registry, "alice");
    REQUIRE(registry.remove(client_id).has_value());

    auto username = registry.lookupUsername(client_id);

    REQUIRE(username.has_value());
    CHECK(username->value() == "alice");
    CHECK_FALSE(registry.findById(client_id).has_value());
}

TEST_CASE("InMemoryClientRegistry::lookupUsername returns ClientNotFound for unknown id",
          "[adapter][persistence][in-memory-client-registry][lookup-username]") {
    InMemoryClientRegistry registry;
    const auto unknown_id = bcmd::ClientId::generate();

    auto username = registry.lookupUsername(unknown_id);

    REQUIRE_FALSE(username.has_value());
    CHECK(username.error() == bcmd::Error::ClientNotFound);
}

TEST_CASE("InMemoryClientRegistry::joinChannelAtomic mutates joined channels in place",
          "[adapter][persistence][in-memory-client-registry][atomic-channels]") {
    InMemoryClientRegistry registry;
    const auto client_id = registerClient(registry, "alice");
    const auto channel_a = bcmd::ChannelId::generate();
    const auto channel_b = bcmd::ChannelId::generate();

    REQUIRE(registry.joinChannelAtomic(client_id, channel_a).has_value());
    REQUIRE(registry.joinChannelAtomic(client_id, channel_b).has_value());

    auto session = registry.findById(client_id);
    REQUIRE(session.has_value());
    CHECK(session->isInChannel(channel_a));
    CHECK(session->isInChannel(channel_b));
}

TEST_CASE("InMemoryClientRegistry::joinChannelAtomic is idempotent",
          "[adapter][persistence][in-memory-client-registry][atomic-channels]") {
    InMemoryClientRegistry registry;
    const auto client_id = registerClient(registry, "alice");
    const auto channel_id = bcmd::ChannelId::generate();

    REQUIRE(registry.joinChannelAtomic(client_id, channel_id).has_value());
    REQUIRE(registry.joinChannelAtomic(client_id, channel_id).has_value());

    auto session = registry.findById(client_id);
    REQUIRE(session.has_value());
    CHECK(session->joinedChannels().size() == 1);
    CHECK(session->isInChannel(channel_id));
}

TEST_CASE("InMemoryClientRegistry::joinChannelAtomic returns ClientNotFound for unknown id",
          "[adapter][persistence][in-memory-client-registry][atomic-channels]") {
    InMemoryClientRegistry registry;
    const auto unknown_id = bcmd::ClientId::generate();
    const auto channel_id = bcmd::ChannelId::generate();

    const auto result = registry.joinChannelAtomic(unknown_id, channel_id);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::ClientNotFound);
}

TEST_CASE("InMemoryClientRegistry::leaveChannelAtomic removes channel and is idempotent",
          "[adapter][persistence][in-memory-client-registry][atomic-channels]") {
    InMemoryClientRegistry registry;
    const auto client_id = registerClient(registry, "alice");
    const auto channel_id = bcmd::ChannelId::generate();

    REQUIRE(registry.joinChannelAtomic(client_id, channel_id).has_value());
    REQUIRE(registry.leaveChannelAtomic(client_id, channel_id).has_value());
    REQUIRE(registry.leaveChannelAtomic(client_id, channel_id).has_value());

    auto session = registry.findById(client_id);
    REQUIRE(session.has_value());
    CHECK_FALSE(session->isInChannel(channel_id));
}

TEST_CASE("InMemoryClientRegistry::leaveChannelAtomic returns ClientNotFound for unknown id",
          "[adapter][persistence][in-memory-client-registry][atomic-channels]") {
    InMemoryClientRegistry registry;
    const auto unknown_id = bcmd::ClientId::generate();
    const auto channel_id = bcmd::ChannelId::generate();

    const auto result = registry.leaveChannelAtomic(unknown_id, channel_id);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == bcmd::Error::ClientNotFound);
}

// Regression: prior to the atomic join/leave methods, JoinChannel did a
// read-modify-write on the entire session via `save()`. That clobbered
// concurrent `touchHeartbeat()` updates and produced lost-update on the
// `last_heartbeat_at_` timestamp. This test interleaves continuous heartbeats
// with join/leave churn and asserts the timestamp never goes backwards.
TEST_CASE("joinChannelAtomic does not clobber concurrent heartbeat updates",
          "[adapter][persistence][in-memory-client-registry][heartbeat][race]") {
    InMemoryClientRegistry registry;
    const auto client_id = registerClient(registry, "alice");

    REQUIRE(registry.touchHeartbeat(client_id).has_value());
    const auto baseline = registry.findById(client_id).value().lastHeartbeatAt();

    std::atomic<bool> stop{false};
    std::atomic<bool> heartbeat_failed{false};

    std::thread heartbeater([&] {
        while (!stop.load(std::memory_order_relaxed)) {
            if (!registry.touchHeartbeat(client_id).has_value()) {
                heartbeat_failed.store(true, std::memory_order_relaxed);
                return;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    });

    constexpr int iterations = 500;
    std::thread churner([&] {
        for (int i = 0; i < iterations; ++i) {
            const auto channel_id = bcmd::ChannelId::generate();
            [[maybe_unused]] auto joined = registry.joinChannelAtomic(client_id, channel_id);
            [[maybe_unused]] auto left = registry.leaveChannelAtomic(client_id, channel_id);
        }
    });

    churner.join();
    stop.store(true, std::memory_order_relaxed);
    heartbeater.join();

    CHECK_FALSE(heartbeat_failed.load(std::memory_order_relaxed));
    const auto after = registry.findById(client_id).value().lastHeartbeatAt();
    CHECK(after > baseline);
}
