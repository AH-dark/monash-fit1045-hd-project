#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>

#include <functional>
#include <string>
#include <utility>

namespace bcmd::client::adapter::tui {

// NOLINTBEGIN(readability-identifier-naming)
inline ftxui::Component InputBar(std::string* text, std::function<void()> on_submit) {
    ftxui::InputOption options;
    options.on_enter = std::move(on_submit);
    return ftxui::Input(text, "Type a message or /command...", options);
}
// NOLINTEND(readability-identifier-naming)

}  // namespace bcmd::client::adapter::tui
