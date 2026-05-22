#include "bcmd/server/adapter/grpc/heartbeat_sweeper.hpp"

#include "bcmd/server/application/usecase/expire_inactive_clients.hpp"

#include <spdlog/spdlog.h>

#include <chrono>
#include <stop_token>
#include <thread>
#include <utility>

namespace bcmd::server::adapter::grpc {

HeartbeatSweeper::HeartbeatSweeper(
    std::shared_ptr<application::usecase::ExpireInactiveClients> expire,
    std::chrono::seconds sweep_interval, std::chrono::seconds heartbeat_timeout)
    : expire_(std::move(expire)),
      sweep_interval_(sweep_interval),
      heartbeat_timeout_(heartbeat_timeout),
      thread_([this](std::stop_token stop) { loop(stop); }) {}

HeartbeatSweeper::~HeartbeatSweeper() {
    thread_.request_stop();
    // jthread auto-joins in destructor.
}

void HeartbeatSweeper::loop(std::stop_token stop) {
    spdlog::info("Heartbeat sweeper started (interval={}s, timeout={}s)", sweep_interval_.count(),
                 heartbeat_timeout_.count());
    while (!stop.stop_requested()) {
        // Sleep with interruption checks (poll every 200ms for responsive shutdown).
        const auto sleep_until_t = std::chrono::steady_clock::now() + sweep_interval_;
        while (std::chrono::steady_clock::now() < sleep_until_t && !stop.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        if (stop.stop_requested()) {
            break;
        }
        const auto deadline = std::chrono::steady_clock::now() - heartbeat_timeout_;
        const auto count = expire_->run(deadline);
        if (count > 0) {
            spdlog::info("Heartbeat sweeper expired {} client(s)", count);
        }
    }
    spdlog::info("Heartbeat sweeper stopped");
}

}  // namespace bcmd::server::adapter::grpc
