#include "bcmd/server/adapter/persistence/in_memory_client_registry.hpp"
#include "bcmd/server/domain/model/username.hpp"
#include "bcmd/shared/ids.hpp"
#include "bcmd/shared/result.hpp"

#include <catch2/catch_test_macros.hpp>

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
    CHECK((after - heartbeat) < std::chrono::milliseconds(100));
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
