#pragma once

#include "bcmd/client/domain/inbox_message.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include <string>
#include <vector>

namespace bcmd::client::adapter::tui {

// NOLINTBEGIN(readability-identifier-naming)
inline ftxui::Element RenderMessageView(const std::vector<domain::InboxMessage>& messages,
                                        int history_count) {
    ftxui::Elements rows;
    const bool show_separator =
        history_count > 0 && history_count < static_cast<int>(messages.size());

    for (int idx{0}; idx < static_cast<int>(messages.size()); ++idx) {
        if (show_separator && idx == history_count) {
            rows.push_back(ftxui::text("--- live ---") | ftxui::dim | ftxui::center);
        }
        const auto& message = messages[static_cast<std::size_t>(idx)];
        const std::string sender = message.sender_name.empty() ? "unknown" : message.sender_name;
        rows.push_back(ftxui::hbox(
            {ftxui::text(sender + ": ") | ftxui::bold, ftxui::paragraph(message.content)}));
    }

    if (rows.empty()) {
        rows.push_back(ftxui::text("No messages yet") | ftxui::dim);
    }
    return ftxui::vbox(std::move(rows));
}

// NOLINTEND(readability-identifier-naming)

}  // namespace bcmd::client::adapter::tui
