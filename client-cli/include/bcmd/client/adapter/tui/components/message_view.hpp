#pragma once

#include "bcmd/client/domain/inbox_message.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string>
#include <utility>
#include <vector>

namespace bcmd::client::adapter::tui {

// NOLINTBEGIN(readability-identifier-naming)

inline std::string formatTimestamp(std::int64_t epoch_ms) {
    if (epoch_ms <= 0) {
        return "--:--";
    }
    const auto time_point =
        std::chrono::system_clock::time_point{std::chrono::milliseconds{epoch_ms}};
    const auto time_value = std::chrono::system_clock::to_time_t(time_point);
    std::tm tm_value{};
#if defined(_WIN32)
    localtime_s(&tm_value, &time_value);
#else
    localtime_r(&time_value, &tm_value);
#endif
    std::array<char, 8> buffer{};
    const auto written = std::strftime(buffer.data(), buffer.size(), "%H:%M", &tm_value);
    return written == 0 ? std::string{"--:--"} : std::string{buffer.data(), written};
}

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
        const std::string timestamp = formatTimestamp(message.sent_at_ms);
        rows.push_back(ftxui::hbox({
            ftxui::text(timestamp + " ") | ftxui::dim,
            ftxui::text(sender + ": ") | ftxui::bold,
            ftxui::paragraph(message.content),
        }));
        ++idx;
    }

    if (rows.empty()) {
        rows.push_back(ftxui::text("No messages yet") | ftxui::dim);
    }
    return ftxui::vbox(std::move(rows));
}

// NOLINTEND(readability-identifier-naming)

}  // namespace bcmd::client::adapter::tui
