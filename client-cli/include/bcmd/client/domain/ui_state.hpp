#pragma once

#include <string>

namespace bcmd::client::domain {

// Mutable UI state that drives what the TUI renders.
struct UiState {
    std::string current_channel_id;
    std::string current_channel_name;
    std::string client_id;
    std::string username;
    bool connected{false};
    bool tls_active{false};
    bool replay_complete{false};
    std::string error_toast;
};

}  // namespace bcmd::client::domain
