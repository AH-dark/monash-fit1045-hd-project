#pragma once

#include <string>
#include <vector>

#include <ftxui/component/component.hpp>

namespace bcmd::client::adapter::tui {

inline ftxui::Component ChannelList(std::vector<std::string>* channels, int* selected_idx) {
    return ftxui::Menu(channels, selected_idx);
}

}  // namespace bcmd::client::adapter::tui
