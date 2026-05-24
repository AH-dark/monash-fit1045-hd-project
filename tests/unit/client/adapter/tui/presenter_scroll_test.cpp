#ifndef BCMD_CLIENT_TUI_TESTING
#define BCMD_CLIENT_TUI_TESTING 1
#endif

#include "bcmd/client/adapter/cli/command_parser.hpp"
#include "bcmd/client/adapter/tui/ftxui_presenter.hpp"
#include "bcmd/client/adapter/tui/inbox_queue.hpp"
#include "bcmd/client/domain/inbox_message.hpp"

#include <catch2/catch_test_macros.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

using bcmd::client::adapter::cli::CommandType;
using bcmd::client::adapter::cli::ParsedCommand;
using bcmd::client::adapter::tui::FtxuiPresenter;
using bcmd::client::adapter::tui::InboxQueue;

// NOLINTBEGIN(misc-use-anonymous-namespace, readability-identifier-naming)

std::shared_ptr<InboxQueue> MakeInboxQueue() { return std::make_shared<InboxQueue>(); }

FtxuiPresenter MakePresenter() { return FtxuiPresenter{MakeInboxQueue()}; }

bcmd::client::domain::InboxMessage MakeMessage(std::string id, bool is_history = false) {
    return bcmd::client::domain::InboxMessage{.message_id = std::move(id),
                                              .channel_id = "alpha",
                                              .sender_name = "alice",
                                              .content = "hello",
                                              .sent_at_ms = 1,
                                              .is_history = is_history};
}

std::vector<bcmd::client::domain::InboxMessage> MakeMessages(int count) {
    std::vector<bcmd::client::domain::InboxMessage> messages;
    messages.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        messages.push_back(MakeMessage(std::to_string(index), index < 5));
    }
    return messages;
}

ftxui::Event MouseWheelEvent(ftxui::Mouse::Button button) {
    ftxui::Mouse mouse{};
    mouse.button = button;
    mouse.x = 0;
    mouse.y = 0;
    return ftxui::Event::Mouse("", mouse);
}

TEST_CASE("Presenter scroll events move the window and clamp to the bounds",
          "[client-adapter][tui]") {
    auto presenter = MakePresenter();
    presenter.testSetMessages(MakeMessages(40));
    presenter.testSetViewportHeightHint(10);

    CHECK(presenter.testHandleScrollEvent(ftxui::Event::PageUp));
    CHECK(presenter.testScrollOffset() == 10);
    CHECK_FALSE(presenter.testAutoScroll());

    CHECK(presenter.testHandleScrollEvent(ftxui::Event::ArrowUp));
    CHECK(presenter.testScrollOffset() == 11);

    CHECK(presenter.testHandleScrollEvent(MouseWheelEvent(ftxui::Mouse::WheelUp)));
    CHECK(presenter.testScrollOffset() == 14);

    CHECK(presenter.testHandleScrollEvent(ftxui::Event::PageDown));
    CHECK(presenter.testScrollOffset() == 4);

    CHECK(presenter.testHandleScrollEvent(MouseWheelEvent(ftxui::Mouse::WheelDown)));
    CHECK(presenter.testScrollOffset() == 1);

    CHECK(presenter.testHandleScrollEvent(ftxui::Event::End));
    CHECK(presenter.testScrollOffset() == 0);
    CHECK(presenter.testAutoScroll());

    CHECK(presenter.testHandleScrollEvent(ftxui::Event::Home));
    CHECK(presenter.testScrollOffset() == 30);
    CHECK_FALSE(presenter.testAutoScroll());
}

TEST_CASE("Presenter ignores scroll input while the help modal is open", "[client-adapter][tui]") {
    auto presenter = MakePresenter();
    presenter.testSetMessages(MakeMessages(40));
    presenter.testSetViewportHeightHint(10);
    presenter.testSetScrollState(7, false);
    presenter.testSetShowHelp(true);

    CHECK(presenter.testHandleScrollEvent(ftxui::Event::PageUp));
    CHECK(presenter.testScrollOffset() == 7);
    CHECK_FALSE(presenter.testAutoScroll());
}

TEST_CASE("Presenter keeps offset when auto-scroll is disabled and resets on clear",
          "[client-adapter][tui]") {
    auto presenter = MakePresenter();

    presenter.testSetScrollState(8, false);
    presenter.testShowMessage(MakeMessage("m1"));
    CHECK(presenter.testScrollOffset() == 8);

    presenter.testSetScrollState(0, true);
    presenter.testShowMessage(MakeMessage("m2"));
    CHECK(presenter.testScrollOffset() == 0);

    presenter.testSetMessages(MakeMessages(10));
    presenter.testSetHistoryCount(5);
    presenter.testSetScrollState(6, false);
    presenter.testClearMessages();

    CHECK(presenter.testMessages().empty());
    CHECK(presenter.testHistoryCount() == 0);
    CHECK(presenter.testScrollOffset() == 0);
    CHECK(presenter.testAutoScroll());
}

TEST_CASE("Presenter joins the clicked channel by index", "[client-adapter][tui]") {
    auto presenter = MakePresenter();
    presenter.showChannelList({"alpha", "beta", "gamma"});

    std::string joined_channel;
    int join_count{0};
    FtxuiPresenter::Actions actions{};
    actions.join_channel = [&join_count, &joined_channel](const std::string& channel) {
        ++join_count;
        joined_channel = channel;
    };
    presenter.testSetActions(actions);

    testOnChannelEnter(presenter, 0);

    CHECK(join_count == 1);
    CHECK(joined_channel == "alpha");
}

TEST_CASE("Presenter joins the clicked channel when selection has moved", "[client-adapter][tui]") {
    auto presenter = MakePresenter();
    presenter.showChannelList({"alpha", "beta", "gamma"});

    std::string joined_channel;
    int join_count{0};
    FtxuiPresenter::Actions actions{};
    actions.join_channel = [&join_count, &joined_channel](const std::string& channel) {
        ++join_count;
        joined_channel = channel;
    };
    presenter.testSetActions(actions);

    testOnChannelEnter(presenter, 2);

    CHECK(join_count == 1);
    CHECK(joined_channel == "gamma");
}

TEST_CASE("Presenter ignores out-of-range channel clicks", "[client-adapter][tui]") {
    auto presenter = MakePresenter();
    presenter.showChannelList({"alpha", "beta", "gamma"});

    int join_count{0};
    FtxuiPresenter::Actions actions{};
    actions.join_channel = [&join_count](const std::string&) { ++join_count; };
    presenter.testSetActions(actions);

    testOnChannelEnter(presenter, -1);
    testOnChannelEnter(presenter, 3);

    CHECK(join_count == 0);
}

TEST_CASE("Presenter sends plain messages when input is not a command", "[client-adapter][tui]") {
    auto presenter = MakePresenter();
    int sent_count{0};
    std::string last_message;
    FtxuiPresenter::Actions actions{};
    actions.send_message = [&sent_count, &last_message](const std::string& message) {
        ++sent_count;
        last_message = message;
    };
    presenter.testSetActions(actions);
    presenter.testSetInputText("hello");

    presenter.testHandleSubmit();

    CHECK(sent_count == 1);
    CHECK(last_message == "hello");
}

TEST_CASE("Presenter dispatches join commands to the join action", "[client-adapter][tui]") {
    auto presenter = MakePresenter();
    int join_count{0};
    std::string joined_channel;
    FtxuiPresenter::Actions actions{};
    actions.join_channel = [&join_count, &joined_channel](const std::string& channel) {
        ++join_count;
        joined_channel = channel;
    };
    presenter.testSetActions(actions);
    presenter.testSetInputText("/join lobby");

    presenter.testHandleSubmit();

    CHECK(join_count == 1);
    CHECK(joined_channel == "lobby");
}

TEST_CASE("Presenter reports unknown slash commands and does not send them",
          "[client-adapter][tui]") {
    auto presenter = MakePresenter();
    int sent_count{0};
    FtxuiPresenter::Actions actions{};
    actions.send_message = [&sent_count](const std::string&) { ++sent_count; };
    presenter.testSetActions(actions);
    presenter.testSetInputText("/ghost");

    presenter.testHandleSubmit();

    CHECK(presenter.testErrorToast() == "unknown command: /ghost - type ? for help");
    CHECK(sent_count == 0);
}

TEST_CASE("Presenter rejects slash-prefixed input even if parsing returns none",
          "[client-adapter][tui]") {
    auto presenter = MakePresenter();
    int sent_count{0};
    FtxuiPresenter::Actions actions{};
    actions.send_message = [&sent_count](const std::string&) { ++sent_count; };
    presenter.testSetActions(actions);

    ParsedCommand parsed{};
    parsed.type = CommandType::None;

    presenter.testHandleParsedCommand(parsed, "/ghost", actions);

    CHECK(presenter.testErrorToast() == "unknown command: /ghost - type ? for help");
    CHECK(sent_count == 0);
}

// NOLINTEND(misc-use-anonymous-namespace, readability-identifier-naming)

}  // namespace
