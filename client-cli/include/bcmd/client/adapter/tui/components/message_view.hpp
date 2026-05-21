#pragma once

#include "bcmd/client/domain/inbox_message.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace bcmd::client::adapter::tui {

// NOLINTBEGIN(readability-identifier-naming)
inline ftxui::Element RenderMessageView(const std::vector<domain::InboxMessage>& messages,
                                        int history_count) {
    ftxui::Elements rows;
    const bool show_separator = history_count > 0 && std::cmp_less(history_count, messages.size());

    std::size_t idx{0};
    for (const auto& message : messages) {
        if (show_separator && std::cmp_equal(idx, history_count)) {
            rows.push_back(ftxui::text("--- live ---") | ftxui::dim | ftxui::center);
        }
        const std::string sender = message.sender_name.empty() ? "unknown" : message.sender_name;
        rows.push_back(ftxui::hbox(
            {ftxui::text(sender + ": ") | ftxui::bold, ftxui::paragraph(message.content)}));
        ++idx;
    }

    if (rows.empty()) {
        rows.push_back(ftxui::text("No messages yet") | ftxui::dim);
    }
    return ftxui::vbox(std::move(rows));
}

// NOLINTEND(readability-identifier-naming)

}  // namespace bcmd::client::adapter::tui
