#pragma once

#include "bcmd/client/adapter/cli/command_parser.hpp"
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
    void showInfo(std::string_view info_text) override;
    void updateConnectionStatus(bool connected, bool tls, std::string_view username) override;
    void updateConnectionState(bcmd::client::application::port::ConnectionState state) override;
    void showChannelList(std::vector<std::string> channel_names) override;
    void clearMessages() override;

    void setActions(Actions actions);

#ifdef BCMD_CLIENT_TUI_TESTING
    // Test-only inspectors and helpers.
    [[nodiscard]] int testScrollOffset() const { return scroll_offset_; }
    [[nodiscard]] bool testAutoScroll() const { return auto_scroll_; }
    [[nodiscard]] int testViewportHeightHint() const { return viewport_height_hint_; }
    [[nodiscard]] bool testShowHelp() const { return show_help_; }
    [[nodiscard]] const std::string& testErrorToast() const { return error_toast_; }
    [[nodiscard]] int testHistoryCount() const { return history_count_; }
    [[nodiscard]] const std::vector<domain::InboxMessage>& testMessages() const {
        return messages_;
    }
    void testSetMessages(std::vector<domain::InboxMessage> messages) { messages_.swap(messages); }
    void testSetScrollState(int scroll_offset, bool auto_scroll) {
        scroll_offset_ = scroll_offset;
        auto_scroll_ = auto_scroll;
    }
    void testSetViewportHeightHint(int viewport_height_hint) {
        viewport_height_hint_ = viewport_height_hint;
    }
    void testSetShowHelp(bool show_help) { show_help_ = show_help; }
    void testSetInputText(std::string input_text) { input_text_.swap(input_text); }
    void testSetActions(const Actions& actions) { actions_ = actions; }
    void testSetHistoryCount(int history_count) { history_count_ = history_count; }
    void testClearMessages() { clearMessages(); }
    void testShowMessage(domain::InboxMessage message) { showMessage(std::move(message)); }
    void testHandleSubmit() { handleSubmit(); }
    bool testHandleScrollEvent(ftxui::Event event) { return handleScrollEvent(std::move(event)); }
    void testHandleParsedCommand(const cli::ParsedCommand& parsed, const std::string& input,
                                 const Actions& actions) {
        handleParsedCommandImpl(parsed, input, actions);
    }
#endif

    // Runs the FTXUI event loop (blocking). Call from main UI thread.
    int run(std::function<void()> on_quit);

private:
    friend void testOnChannelEnter(FtxuiPresenter& presenter, int idx) {
        presenter.onChannelEnter(idx);
    }

    void handleSubmit();
    void onChannelEnter(int idx);
    void handleParsedCommandImpl(const cli::ParsedCommand& parsed, const std::string& input,
                                 const Actions& actions);
    bool handleScrollEvent(ftxui::Event event);
    void clampScrollOffsetLocked();
    void scrollByLocked(int delta);
    void scrollToTopLocked();
    void scrollToBottomLocked();
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
    bcmd::client::application::port::ConnectionState connection_state_{
        bcmd::client::application::port::ConnectionState::Connecting};
    bool show_help_{false};
    std::string username_{};
    std::string error_toast_{};
    std::string info_toast_{};
    std::string input_text_{};
    int scroll_offset_{0};
    bool auto_scroll_{true};
    int viewport_height_hint_{20};
    Actions actions_{};
};

}  // namespace bcmd::client::adapter::tui
