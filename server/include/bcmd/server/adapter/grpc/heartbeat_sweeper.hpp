#pragma once

#include "bcmd/server/application/usecase/expire_inactive_clients.hpp"

#include <chrono>
#include <memory>
#include <thread>

namespace bcmd::server::adapter::grpc {

// RAII background thread that periodically runs ExpireInactiveClients.
// Construction starts the thread; destruction stops and joins.
class HeartbeatSweeper {
public:
    HeartbeatSweeper(std::shared_ptr<application::usecase::ExpireInactiveClients> expire,
                     std::chrono::seconds sweep_interval, std::chrono::seconds heartbeat_timeout);
    ~HeartbeatSweeper();
    HeartbeatSweeper(const HeartbeatSweeper&) = delete;
    HeartbeatSweeper& operator=(const HeartbeatSweeper&) = delete;
    HeartbeatSweeper(HeartbeatSweeper&&) = delete;
    HeartbeatSweeper& operator=(HeartbeatSweeper&&) = delete;

private:
    void loop(std::stop_token stop);

    std::shared_ptr<application::usecase::ExpireInactiveClients> expire_;
    std::chrono::seconds sweep_interval_;
    std::chrono::seconds heartbeat_timeout_;
    std::jthread thread_;
};

}  // namespace bcmd::server::adapter::grpc
