#include "bcmd/client/adapter/tui/components/message_view.hpp"
#include "bcmd/client/domain/inbox_message.hpp"

#include <catch2/catch_test_macros.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include <string>
#include <vector>

namespace {

using bcmd::client::adapter::tui::computeScrollWindow;
using bcmd::client::adapter::tui::RenderMessageView;
using bcmd::client::domain::InboxMessage;

std::string RenderToString(ftxui::Element element) {
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Full(), ftxui::Dimension::Fit(element));
    ftxui::Render(screen, element);
    return screen.ToString();
}

// NOLINTBEGIN(misc-use-anonymous-namespace, readability-identifier-naming)

TEST_CASE("Scroll window keeps empty history stable", "[client-adapter][tui]") {
    const auto window = computeScrollWindow(0, 0, 10);

    CHECK(window.first == 0);
    CHECK(window.last == 0);
}

TEST_CASE("Scroll window shows all messages when content is shorter than the viewport",
          "[client-adapter][tui]") {
    const auto window = computeScrollWindow(5, 0, 10);

    CHECK(window.first == 0);
    CHECK(window.last == 5);
}

TEST_CASE("Scroll window defaults to the live tail", "[client-adapter][tui]") {
    const auto window = computeScrollWindow(100, 0, 10);

    CHECK(window.first == 90);
    CHECK(window.last == 100);
}

TEST_CASE("Scroll window moves upward by the requested offset", "[client-adapter][tui]") {
    const auto window = computeScrollWindow(100, 5, 10);

    CHECK(window.first == 85);
    CHECK(window.last == 95);
}

TEST_CASE("Scroll window clamps large positive offsets to the top", "[client-adapter][tui]") {
    const auto window = computeScrollWindow(100, 90, 10);

    CHECK(window.first == 0);
    CHECK(window.last == 10);
}

TEST_CASE("Scroll window clamps oversized offsets above the top", "[client-adapter][tui]") {
    const auto window = computeScrollWindow(100, 999, 10);

    CHECK(window.first == 0);
    CHECK(window.last == 10);
}

TEST_CASE("Scroll window clamps negative offsets back to the live tail", "[client-adapter][tui]") {
    const auto window = computeScrollWindow(100, -3, 10);

    CHECK(window.first == 90);
    CHECK(window.last == 100);
}

TEST_CASE("Scroll window preserves full history when the viewport hint is disabled",
          "[client-adapter][tui]") {
    const auto window = computeScrollWindow(100, 0, 0);

    CHECK(window.first == 0);
    CHECK(window.last == 100);
}

TEST_CASE("RenderMessageView preserves the live separator without a viewport hint",
          "[client-adapter][tui]") {
    const std::vector<InboxMessage> messages{
        InboxMessage{.message_id = "m1",
                     .channel_id = "alpha",
                     .sender_name = "alice",
                     .content = "hist",
                     .sent_at_ms = 1,
                     .is_history = true},
        InboxMessage{.message_id = "m2",
                     .channel_id = "alpha",
                     .sender_name = "bob",
                     .content = "live",
                     .sent_at_ms = 2,
                     .is_history = false},
    };

    const auto rendered = RenderToString(RenderMessageView(messages, 1));

    CHECK(rendered.contains("--- live ---"));
}

TEST_CASE("RenderMessageView shows the separator when a viewport crosses the boundary",
          "[client-adapter][tui]") {
    const std::vector<InboxMessage> messages{
        InboxMessage{.message_id = "m1",
                     .channel_id = "alpha",
                     .sender_name = "alice",
                     .content = "hist",
                     .sent_at_ms = 1,
                     .is_history = true},
        InboxMessage{.message_id = "m2",
                     .channel_id = "alpha",
                     .sender_name = "bob",
                     .content = "live",
                     .sent_at_ms = 2,
                     .is_history = false},
    };

    const auto rendered = RenderToString(RenderMessageView(messages, 1, 0, 2));

    CHECK(rendered.contains("--- live ---"));
}

// NOLINTEND(misc-use-anonymous-namespace, readability-identifier-naming)

}  // namespace
