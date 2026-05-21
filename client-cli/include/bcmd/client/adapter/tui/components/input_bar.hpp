#pragma once

#include <functional>
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>

namespace bcmd::client::adapter::tui {

inline ftxui::Component InputBar(std::string* text, std::function<void()> on_submit) {
    ftxui::InputOption options;
    options.on_enter = std::move(on_submit);
    return ftxui::Input(text, "Type a message or /command...", options);
}

}  // namespace bcmd::client::adapter::tui
