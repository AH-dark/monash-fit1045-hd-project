#pragma once

#include "bcmd/client/adapter/tui/inbox_queue.hpp"
#include "bcmd/client/application/port/i_presenter.hpp"
#include "bcmd/client/domain/inbox_message.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace bcmd::client::adapter::tui {

class FtxuiPresenter final : public application::port::IPresenter {
public:
    struct Actions {
        std::function<void(std::string)> send_message{};
        std::function<void(std::string)> join_channel{};
        std::function<void(std::string)> create_channel{};
        std::function<void()> leave_channel{};
        std::function<void()> list_channels{};
    };

    explicit FtxuiPresenter(std::shared_ptr<InboxQueue> inbox);

    void showMessage(domain::InboxMessage message) override;
    void showReplayComplete(std::string_view channel_id) override;
    void showError(std::string_view error_text) override;
    void updateConnectionStatus(bool connected, bool tls, std::string_view username) override;
    void showChannelList(std::vector<std::string> channel_names) override;

    void setActions(Actions actions);

    // Runs the FTXUI event loop (blocking). Call from main UI thread.
    int run(std::function<void()> on_quit);

private:
    void handleSubmit(const std::function<void()>& on_quit);
    ftxui::Element render(const ftxui::Component& channels, const ftxui::Component& input);

    std::shared_ptr<InboxQueue> inbox_{};
    ftxui::ScreenInteractive screen_{ftxui::ScreenInteractive::Fullscreen()};

    mutable std::mutex ui_mutex_{};
    std::vector<domain::InboxMessage> messages_{};
    std::vector<std::string> channel_names_{};
    int selected_channel_idx_{0};
    int history_count_{0};
    bool connected_{false};
    bool tls_{false};
    bool show_help_{false};
    std::string username_{};
    std::string error_toast_{};
    std::string input_text_{};
    Actions actions_{};
};

}  // namespace bcmd::client::adapter::tui
