#include "bcmd/client/adapter/tui/ftxui_presenter.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "bcmd/client/adapter/cli/command_parser.hpp"
#include "bcmd/client/adapter/tui/components/channel_list.hpp"
#include "bcmd/client/adapter/tui/components/input_bar.hpp"
#include "bcmd/client/adapter/tui/components/message_view.hpp"
#include "bcmd/client/domain/inbox_message.hpp"

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
        std::lock_guard<std::mutex> lock{ui_mutex_};
        error_toast_ = std::string{error_text};
    }
    screen_.PostEvent(ftxui::Event::Custom);
}

void FtxuiPresenter::updateConnectionStatus(bool connected,
                                            bool tls,
                                            std::string_view username) {
    {
        std::lock_guard<std::mutex> lock{ui_mutex_};
        connected_ = connected;
        tls_ = tls;
        username_ = std::string{username};
    }
    screen_.PostEvent(ftxui::Event::Custom);
}

void FtxuiPresenter::showChannelList(std::vector<std::string> channel_names) {
    {
        std::lock_guard<std::mutex> lock{ui_mutex_};
        channel_names_ = std::move(channel_names);
        selected_channel_idx_ = std::clamp(selected_channel_idx_, 0,
                                           std::max(0,
                                                    static_cast<int>(channel_names_.size()) - 1));
    }
    screen_.PostEvent(ftxui::Event::Custom);
}

void FtxuiPresenter::setActions(Actions actions) {
    std::lock_guard<std::mutex> lock{ui_mutex_};
    actions_ = std::move(actions);
}

int FtxuiPresenter::run(std::function<void()> on_quit) {
    auto channels = ChannelList(&channel_names_, &selected_channel_idx_);
    auto input = InputBar(&input_text_, [this, on_quit] { handleSubmit(on_quit); });
    auto layout = ftxui::Container::Vertical({channels, input});
    auto renderer = ftxui::Renderer(layout, [this, channels, input] { return render(channels, input); });
    auto component = ftxui::CatchEvent(renderer, [this, on_quit](const ftxui::Event& event) {
        if (event == ftxui::Event::Escape) {
            on_quit();
            screen_.ExitLoopClosure()();
            return true;
        }
        return false;
    });

    screen_.Loop(component);
    return 0;
}

void FtxuiPresenter::handleSubmit(const std::function<void()>& on_quit) {
    std::string input;
    Actions actions;
    {
        std::lock_guard<std::mutex> lock{ui_mutex_};
        input = std::exchange(input_text_, {});
        actions = actions_;
    }

    const auto parsed = cli::parseCommand(input);
    switch (parsed.type) {
        case cli::CommandType::Join:
            if (actions.join_channel) {
                actions.join_channel(parsed.arg);
            }
            break;
        case cli::CommandType::Leave:
            if (actions.leave_channel) {
                actions.leave_channel();
            }
            break;
        case cli::CommandType::List:
            if (actions.list_channels) {
                actions.list_channels();
            }
            break;
        case cli::CommandType::Quit:
            on_quit();
            screen_.ExitLoopClosure()();
            break;
        case cli::CommandType::None:
            if (!input.empty() && actions.send_message) {
                actions.send_message(input);
            }
            break;
    }
}

ftxui::Element FtxuiPresenter::render(const ftxui::Component& channels,
                                      const ftxui::Component& input) {
    std::lock_guard<std::mutex> lock{ui_mutex_};
    inbox_->drainTo(messages_);
    history_count_ = static_cast<int>(std::ranges::count_if(messages_, [](const auto& message) {
        return message.is_history;
    }));

    const std::string connection = connected_ ? "connected" : "disconnected";
    const std::string tls_status = tls_ ? "TLS" : "insecure";
    const std::string status = "Status: " + connection + " (" + tls_status + ") as " + username_;
    const std::string error = error_toast_.empty() ? "" : " | Error: " + error_toast_;

    return ftxui::vbox({
               ftxui::hbox({
                   ftxui::vbox({ftxui::text("Channels") | ftxui::bold,
                                channels->Render()})
                       | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 24) | ftxui::border,
                   ftxui::vbox({ftxui::text("Messages") | ftxui::bold,
                                RenderMessageView(messages_, history_count_) | ftxui::vscroll_indicator
                                    | ftxui::frame})
                       | ftxui::flex | ftxui::border,
               }) | ftxui::flex,
               ftxui::separator(),
               input->Render() | ftxui::border,
               ftxui::text(status + error) | ftxui::dim,
           })
           | ftxui::border;
}

}  // namespace bcmd::client::adapter::tui

// NOLINTEND(readability-identifier-naming)
