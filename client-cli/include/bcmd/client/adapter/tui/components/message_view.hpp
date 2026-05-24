#pragma once

#include "bcmd/client/domain/inbox_message.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
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

struct ScrollWindow {
    int first;
    int last;
};

[[nodiscard]] inline ScrollWindow computeScrollWindow(int message_count, int scroll_offset,
                                                      int viewport_height) {
    const int clamped_message_count = std::max(message_count, 0);
    if (viewport_height <= 0) {
        return ScrollWindow{.first = 0, .last = clamped_message_count};
    }

    const int max_scroll_offset = std::max(clamped_message_count - viewport_height, 0);
    const int clamped_scroll_offset = std::clamp(scroll_offset, 0, max_scroll_offset);
    const int first = std::max(clamped_message_count - viewport_height - clamped_scroll_offset, 0);
    const int last = std::min(first + viewport_height, clamped_message_count);
    return ScrollWindow{.first = first, .last = last};
}

[[nodiscard]] inline bool shouldRenderLiveSeparator(int message_count, int history_count,
                                                    int scroll_offset, int viewport_height) {
    const bool show_separator = history_count > 0 && std::cmp_less(history_count, message_count);
    if (!show_separator) {
        return false;
    }

    if (viewport_height <= 0) {
        return true;
    }

    const ScrollWindow window = computeScrollWindow(message_count, scroll_offset, viewport_height);
    return window.first < history_count && std::cmp_less(history_count, window.last);
}

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
                                        int history_count, int scroll_offset = 0,
                                        int viewport_height = 0) {
    const ScrollWindow window =
        computeScrollWindow(static_cast<int>(messages.size()), scroll_offset, viewport_height);
    ftxui::Elements rows;
    const bool use_window = viewport_height > 0;
    const bool show_window_separator = shouldRenderLiveSeparator(
        static_cast<int>(messages.size()), history_count, scroll_offset, viewport_height);

    const int first = use_window ? window.first : 0;
    const int last = use_window ? window.last : static_cast<int>(messages.size());

    for (int idx = first; idx < last; ++idx) {
        if (show_window_separator && idx == history_count) {
            rows.push_back(ftxui::text("--- live ---") | ftxui::dim | ftxui::center);
        }
        const auto& message = messages[static_cast<std::size_t>(idx)];
        const std::string sender = message.sender_name.empty() ? "unknown" : message.sender_name;
        const std::string timestamp = formatTimestamp(message.sent_at_ms);
        rows.push_back(ftxui::hbox({
            ftxui::text(timestamp + " ") | ftxui::dim,
            ftxui::text(sender + ": ") | ftxui::bold,
            ftxui::paragraph(message.content),
        }));
    }

    if (rows.empty()) {
        rows.push_back(ftxui::text("No messages yet") | ftxui::dim);
    }
    return ftxui::vbox(std::move(rows));
}

// NOLINTEND(readability-identifier-naming)

}  // namespace bcmd::client::adapter::tui
