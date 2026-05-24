#include "bcmd/client/adapter/tui/ftxui_presenter.hpp"

#include "bcmd/client/adapter/cli/command_parser.hpp"
#include "bcmd/client/adapter/tui/components/channel_list.hpp"
#include "bcmd/client/adapter/tui/components/input_bar.hpp"
#include "bcmd/client/adapter/tui/components/message_view.hpp"
#include "bcmd/client/adapter/tui/inbox_queue.hpp"
#include "bcmd/client/application/port/i_presenter.hpp"
#include "bcmd/client/domain/inbox_message.hpp"
#include "bcmd/shared/string_utils.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace bcmd::client::adapter::tui {

// NOLINTBEGIN(readability-identifier-naming)

FtxuiPresenter::FtxuiPresenter(std::shared_ptr<InboxQueue> inbox) : inbox_(std::move(inbox)) {}

void FtxuiPresenter::showMessage(domain::InboxMessage message) {
    inbox_->push(std::move(message));
    screen_.PostEvent(ftxui::Event::Custom);
}

void FtxuiPresenter::showReplayComplete(std::string_view /*channel_id*/) {
    screen_.PostEvent(ftxui::Event::Custom);
}

void FtxuiPresenter::showError(std::string_view error_text) {
    {
        std::scoped_lock lock{ui_mutex_};
        error_toast_ = std::string{error_text};
        info_toast_.clear();
    }
    screen_.PostEvent(ftxui::Event::Custom);
}

void FtxuiPresenter::showInfo(std::string_view info_text) {
    {
        std::scoped_lock lock{ui_mutex_};
        info_toast_ = std::string{info_text};
        error_toast_.clear();
    }
    screen_.PostEvent(ftxui::Event::Custom);
}

void FtxuiPresenter::updateConnectionStatus(bool connected, bool tls, std::string_view username) {
    {
        std::scoped_lock lock{ui_mutex_};
        connected_ = connected;
        tls_ = tls;
        username_ = std::string{username};
    }
    screen_.PostEvent(ftxui::Event::Custom);
}

void FtxuiPresenter::updateConnectionState(bcmd::client::application::port::ConnectionState state) {
    {
        std::scoped_lock lock{ui_mutex_};
        connection_state_ = state;
    }
    screen_.PostEvent(ftxui::Event::Custom);
}

void FtxuiPresenter::showChannelList(std::vector<std::string> channel_names) {
    {
        std::scoped_lock lock{ui_mutex_};
        channel_names_ = std::move(channel_names);
        selected_channel_idx_ = std::clamp(
            selected_channel_idx_, 0, std::max(0, static_cast<int>(channel_names_.size()) - 1));
    }
    screen_.PostEvent(ftxui::Event::Custom);
}

void FtxuiPresenter::clearMessages() {
    inbox_->clear();
    {
        std::scoped_lock lock{ui_mutex_};
        messages_.clear();
        history_count_ = 0;
    }
    screen_.PostEvent(ftxui::Event::Custom);
}

void FtxuiPresenter::setActions(Actions actions) {
    std::scoped_lock lock{ui_mutex_};
    actions_ = std::move(actions);
}

// NOLINTNEXTLINE(performance-unnecessary-value-param)
int FtxuiPresenter::run(std::function<void()> on_quit) {
    auto channels = ChannelList(&channel_names_, &selected_channel_idx_, [](int) {});
    auto input = InputBar(&input_text_, [this] { handleSubmit(); });
    auto layout = ftxui::Container::Vertical({channels, input});
    auto renderer =
        ftxui::Renderer(layout, [this, channels, input] { return render(channels, input); });
    auto component = ftxui::CatchEvent(renderer, [this](const ftxui::Event& event) {
        if (event == ftxui::Event::Escape) {
            bool was_help_open{false};
            {
                std::scoped_lock lock{ui_mutex_};
                if (show_help_) {
                    show_help_ = false;
                    was_help_open = true;
                }
            }
            if (was_help_open) {
                screen_.PostEvent(ftxui::Event::Custom);
            }
            return true;
        }
        if (event == ftxui::Event::Character('?')) {
            {
                std::scoped_lock lock{ui_mutex_};
                show_help_ = !show_help_;
            }
            screen_.PostEvent(ftxui::Event::Custom);
            return true;
        }
        return false;
    });

    screen_.Loop(component);
    // FTXUI converts SIGINT/SIGTERM into a normal Loop() return, so this is
    // the single cleanup site that guarantees the server receives Disconnect.
    if (on_quit) {
        on_quit();
    }
    return 0;
}

void FtxuiPresenter::handleSubmit() {
    std::string input;
    Actions actions;
    {
        std::scoped_lock lock{ui_mutex_};
        input = bcmd::trim_copy(std::exchange(input_text_, {}));
        actions = actions_;
    }

    const auto parsed = cli::parseCommand(input);

    switch (parsed.type) {
        case cli::CommandType::Join:
            if (actions.join_channel) {
                actions.join_channel(parsed.arg);
            }
            break;
        case cli::CommandType::Create:
            if (actions.create_channel) {
                actions.create_channel(parsed.arg);
            }
            break;
        case cli::CommandType::Quit:
            screen_.ExitLoopClosure()();
            break;
        case cli::CommandType::Leave:
            actions.leave_channel();
            break;
        case cli::CommandType::List:
            actions.list_channels();
            break;
        case cli::CommandType::Unknown:
            break;
        case cli::CommandType::None:
            if (!input.empty() && actions.send_message) {
                actions.send_message(input);
            }
            break;
    }
}

// NOLINTNEXTLINE(readability-make-member-function-const)
ftxui::Element FtxuiPresenter::render(const ftxui::Component& channels,
                                      const ftxui::Component& input) {
    std::scoped_lock lock{ui_mutex_};
    inbox_->drainTo(messages_);
    history_count_ = static_cast<int>(
        std::ranges::count_if(messages_, [](const auto& message) { return message.is_history; }));

    using ConnectionState = bcmd::client::application::port::ConnectionState;
    const auto connection_state_str = [this] {
        switch (connection_state_) {
            case ConnectionState::Connecting:
                return std::string{"status: connecting"};
            case ConnectionState::Connected:
                return std::string{"status: connected"};
            case ConnectionState::NetworkError:
                return std::string{"status: network error"};
            case ConnectionState::Closed:
                return std::string{"status: closed"};
        }
        return std::string{"status: unknown"};
    }();
    const std::string tls_status = tls_ ? "TLS" : "insecure";
    const std::string status = "(" + tls_status + ") as " + username_;
    const std::string error = error_toast_.empty() ? "" : " | Error: " + error_toast_;
    const std::string info = info_toast_.empty() ? "" : " | " + info_toast_;
    const std::string hint = show_help_ ? "" : " | press ? for help";

    auto main_view =
        ftxui::vbox({
            ftxui::hbox({
                ftxui::vbox({ftxui::text("Channels") | ftxui::bold, channels->Render()}) |
                    ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 24) | ftxui::border,
                ftxui::vbox({ftxui::text("Messages") | ftxui::bold,
                             RenderMessageView(messages_, history_count_) |
                                 ftxui::vscroll_indicator | ftxui::frame}) |
                    ftxui::flex | ftxui::border,
            }) | ftxui::flex,
            ftxui::separator(),
            input->Render() | ftxui::border,
            ftxui::hbox({
                ftxui::text(connection_state_str) | ftxui::dim,
                ftxui::filler(),
                ftxui::text(status + error + info + hint) | ftxui::dim,
            }),
        }) |
        ftxui::border;

    if (!show_help_) {
        return main_view;
    }

    auto help_panel = ftxui::vbox({
                          ftxui::text("Keyboard Shortcuts") | ftxui::bold | ftxui::center,
                          ftxui::separator(),
                          ftxui::text("  Enter             send message to current channel"),
                          ftxui::text("  /create <name>    create a new channel"),
                          ftxui::text("  /join <name>      join an existing channel"),
                          ftxui::text("  /leave            leave the current channel"),
                          ftxui::text("  /list             list all channels"),
                          ftxui::text("  /quit             disconnect and exit"),
                          ftxui::text("  ?                 toggle this help"),
                          ftxui::text("  Esc               close this help"),
                          ftxui::separator(),
                          ftxui::text("Channel names: 1-64 chars of [a-zA-Z0-9-]") | ftxui::dim,
                      }) |
                      ftxui::border | ftxui::bgcolor(ftxui::Color::Black) | ftxui::clear_under |
                      ftxui::center;

    return ftxui::dbox({main_view, help_panel});
}

}  // namespace bcmd::client::adapter::tui

// NOLINTEND(readability-identifier-naming)
