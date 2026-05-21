#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>

#include <string>
#include <vector>

namespace bcmd::client::adapter::tui {

// NOLINTBEGIN(readability-identifier-naming)
inline ftxui::Component ChannelList(std::vector<std::string>* channels, int* selected_idx) {
    return ftxui::Menu(channels, selected_idx);
}
// NOLINTEND(readability-identifier-naming)

}  // namespace bcmd::client::adapter::tui
