#include "bcmd/client/adapter/tui/components/channel_list.hpp"

#include <catch2/catch_test_macros.hpp>
#include <ftxui/component/event.hpp>

#include <string>
#include <vector>

namespace {

using bcmd::client::adapter::tui::ChannelList;

// NOLINTBEGIN(misc-use-anonymous-namespace, readability-identifier-naming)

TEST_CASE("ChannelList invokes on_enter with the current selection", "[client-adapter][tui]") {
    std::vector<std::string> channels{"alpha", "beta", "gamma"};
    int selected_idx{1};
    int invoked_idx{-1};

    auto component =
        ChannelList(&channels, &selected_idx, [&invoked_idx](int index) { invoked_idx = index; });

    CHECK(component->OnEvent(ftxui::Event::Return));
    CHECK(invoked_idx == 1);
}

TEST_CASE("ChannelList ArrowDown changes selection without invoking on_enter",
          "[client-adapter][tui]") {
    std::vector<std::string> channels{"alpha", "beta", "gamma"};
    int selected_idx{0};
    int invoked_count{0};

    auto component =
        ChannelList(&channels, &selected_idx, [&invoked_count](int) { ++invoked_count; });

    CHECK(component->OnEvent(ftxui::Event::ArrowDown));
    CHECK(selected_idx == 1);
    CHECK(invoked_count == 0);
}

TEST_CASE("ChannelList Return on empty list does not invoke on_enter", "[client-adapter][tui]") {
    std::vector<std::string> channels{};
    int selected_idx{0};
    int invoked_count{0};

    auto component =
        ChannelList(&channels, &selected_idx, [&invoked_count](int) { ++invoked_count; });

    CHECK(component->OnEvent(ftxui::Event::Return));
    CHECK(invoked_count == 0);
}

TEST_CASE("ChannelList Return uses the current selected index", "[client-adapter][tui]") {
    std::vector<std::string> channels{"alpha", "beta", "gamma"};
    int selected_idx{1};
    int invoked_idx{-1};

    auto component =
        ChannelList(&channels, &selected_idx, [&invoked_idx](int index) { invoked_idx = index; });

    selected_idx = 2;
    CHECK(component->OnEvent(ftxui::Event::Return));
    CHECK(invoked_idx == 2);
}

// NOLINTEND(misc-use-anonymous-namespace, readability-identifier-naming)

}  // namespace
