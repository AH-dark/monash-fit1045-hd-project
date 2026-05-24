#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace bcmd::client::adapter::tui {

// NOLINTBEGIN(readability-identifier-naming)
inline ftxui::Component ChannelList(std::vector<std::string>* channels, int* selected_idx,
                                    std::function<void(int)> on_enter) {
    auto option = ftxui::MenuOption::Vertical();
    option.on_enter = [channels, selected_idx, on_enter = std::move(on_enter)]() mutable {
        if (channels == nullptr || selected_idx == nullptr) {
            return;
        }
        if (*selected_idx < 0 || *selected_idx >= static_cast<int>(channels->size())) {
            return;
        }
        if (on_enter) {
            on_enter(*selected_idx);
        }
    };
    return ftxui::Menu(channels, selected_idx, option);
}
// NOLINTEND(readability-identifier-naming)

}  // namespace bcmd::client::adapter::tui
