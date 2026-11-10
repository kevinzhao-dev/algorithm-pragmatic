#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace detail {

struct Layout {
    std::int64_t badness;
    std::vector<std::size_t> starts;
};

inline std::optional<Layout> solve_layout(std::span<const int> widths, int line_width) {
    const std::size_t n = widths.size();
    constexpr std::int64_t kInfinity = std::numeric_limits<std::int64_t>::max();
    if (std::ranges::any_of(widths, [&](int width) { return width > line_width; })) {
        return std::nullopt;
    }

    std::vector<std::int64_t> best(n + 1, kInfinity);
    std::vector<std::size_t> next_start(n + 1, n);
    best[n] = 0;
    for (std::size_t i = n; i-- > 0;) {
        std::int64_t used = -1;
        for (std::size_t j = i; j < n; ++j) {
            used += widths[j] + 1;
            if (used > line_width) {
                break;
            }
            const std::int64_t slack = line_width - used;
            const std::int64_t line_badness = j + 1 == n ? 0 : slack * slack * slack;
            if (best[j + 1] != kInfinity && line_badness + best[j + 1] < best[i]) {
                best[i] = line_badness + best[j + 1];
                next_start[i] = j + 1;
            }
        }
    }

    Layout layout{best[0], {}};
    for (std::size_t at = 0; at < n; at = next_start[at]) {
        layout.starts.push_back(at);
    }
    return layout;
}

}  // namespace detail

inline std::optional<std::int64_t> min_badness(std::span<const int> widths, int line_width) {
    const auto layout = detail::solve_layout(widths, line_width);
    if (!layout) {
        return std::nullopt;
    }
    return layout->badness;
}

inline std::optional<std::vector<std::size_t>> line_starts(std::span<const int> widths,
                                                           int line_width) {
    auto layout = detail::solve_layout(widths, line_width);
    if (!layout) {
        return std::nullopt;
    }
    return std::move(layout->starts);
}

inline std::optional<std::vector<std::string>> wrap_text(std::string_view text, int line_width) {
    const auto is_space = [](char ch) { return ch == ' ' || ch == '\t' || ch == '\n'; };
    std::vector<std::string_view> words;
    for (std::size_t at = 0; at < text.size();) {
        if (is_space(text[at])) {
            ++at;
            continue;
        }
        std::size_t end = at;
        while (end < text.size() && !is_space(text[end])) {
            ++end;
        }
        words.push_back(text.substr(at, end - at));
        at = end;
    }

    std::vector<int> widths;
    widths.reserve(words.size());
    for (const std::string_view word : words) {
        widths.push_back(static_cast<int>(word.size()));
    }
    const auto starts = line_starts(widths, line_width);
    if (!starts) {
        return std::nullopt;
    }

    std::vector<std::string> lines;
    for (std::size_t k = 0; k < starts->size(); ++k) {
        const std::size_t begin = (*starts)[k];
        const std::size_t end = k + 1 < starts->size() ? (*starts)[k + 1] : words.size();
        std::string line(words[begin]);
        for (std::size_t w = begin + 1; w < end; ++w) {
            line += ' ';
            line += words[w];
        }
        lines.push_back(std::move(line));
    }
    return lines;
}
