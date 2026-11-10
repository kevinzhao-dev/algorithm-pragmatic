#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using Alignment = std::vector<std::pair<std::size_t, std::size_t>>;

namespace detail {

// Returns the last row of the LCS table for the first `n` items of one sequence against the
// first `m` of another: row[t] is the LCS length of those n items with the first t items.
template <typename ItemA, typename ItemB>
std::vector<std::size_t> last_row(std::size_t n, std::size_t m, ItemA&& item_a, ItemB&& item_b) {
    std::vector<std::size_t> row(m + 1, 0);
    for (std::size_t i = 0; i < n; ++i) {
        std::size_t diagonal = 0;
        for (std::size_t j = 0; j < m; ++j) {
            const std::size_t above = row[j + 1];
            row[j + 1] = item_a(i) == item_b(j) ? diagonal + 1 : std::max(above, row[j]);
            diagonal = above;
        }
    }
    return row;
}

template <typename A, typename B>
void hirschberg(const A& a, std::size_t a_begin, std::size_t a_end, const B& b, std::size_t b_begin,
                std::size_t b_end, Alignment& out) {
    const std::size_t rows = a_end - a_begin;
    const std::size_t cols = b_end - b_begin;
    if (rows == 0 || cols == 0) {
        return;
    }
    if (rows == 1) {
        for (std::size_t j = b_begin; j < b_end; ++j) {
            if (a[a_begin] == b[j]) {
                out.emplace_back(a_begin, j);
                return;
            }
        }
        return;
    }

    const std::size_t mid = a_begin + rows / 2;
    std::size_t split = b_begin;
    {
        // Scoped so both rows are released before recursing.
        const auto forward = last_row(
            mid - a_begin, cols, [&](std::size_t i) -> decltype(auto) { return a[a_begin + i]; },
            [&](std::size_t j) -> decltype(auto) { return b[b_begin + j]; });
        const auto backward = last_row(
            a_end - mid, cols, [&](std::size_t i) -> decltype(auto) { return a[a_end - 1 - i]; },
            [&](std::size_t j) -> decltype(auto) { return b[b_end - 1 - j]; });
        std::size_t best = 0;
        for (std::size_t taken = 0; taken <= cols; ++taken) {
            const std::size_t score = forward[taken] + backward[cols - taken];
            if (taken == 0 || score > best) {
                best = score;
                split = b_begin + taken;
            }
        }
    }
    hirschberg(a, a_begin, mid, b, b_begin, split, out);
    hirschberg(a, mid, a_end, b, split, b_end, out);
}

}  // namespace detail

template <std::ranges::random_access_range A, std::ranges::random_access_range B>
std::size_t lcs_length(const A& a, const B& b) {
    const std::size_t n = std::ranges::size(a);
    const std::size_t m = std::ranges::size(b);
    if (m > n) {
        return lcs_length(b, a);  // The rolling row runs over the shorter sequence.
    }
    return detail::last_row(
        n, m, [&](std::size_t i) -> decltype(auto) { return a[i]; },
        [&](std::size_t j) -> decltype(auto) { return b[j]; })[m];
}

template <std::ranges::random_access_range A, std::ranges::random_access_range B>
Alignment lcs_alignment(const A& a, const B& b) {
    const std::size_t n = std::ranges::size(a);
    const std::size_t m = std::ranges::size(b);
    std::vector<std::uint32_t> table((n + 1) * (m + 1), 0);
    const auto cell = [&](std::size_t i, std::size_t j) -> std::uint32_t& {
        return table[i * (m + 1) + j];
    };
    for (std::size_t i = n; i-- > 0;) {
        for (std::size_t j = m; j-- > 0;) {
            cell(i, j) = a[i] == b[j] ? cell(i + 1, j + 1) + 1
                                      : std::max(cell(i + 1, j), cell(i, j + 1));
        }
    }

    Alignment out;
    out.reserve(cell(0, 0));
    for (std::size_t i = 0, j = 0; i < n && j < m;) {
        if (a[i] == b[j]) {
            out.emplace_back(i++, j++);
        } else if (cell(i + 1, j) >= cell(i, j + 1)) {
            ++i;
        } else {
            ++j;
        }
    }
    return out;
}

template <std::ranges::random_access_range A, std::ranges::random_access_range B>
Alignment lcs_alignment_linear(const A& a, const B& b) {
    Alignment out;
    detail::hirschberg(a, 0, std::ranges::size(a), b, 0, std::ranges::size(b), out);
    return out;
}

enum class DiffKind { Keep, Remove, Add };

struct DiffLine {
    DiffKind kind;
    std::string_view text;
};

inline std::vector<DiffLine> diff_lines(std::span<const std::string> before,
                                        std::span<const std::string> after) {
    const Alignment kept = lcs_alignment_linear(before, after);
    std::vector<DiffLine> out;
    out.reserve(before.size() + after.size() - kept.size());
    std::size_t i = 0;
    std::size_t j = 0;
    const auto emit_changes_until = [&](std::size_t a_end, std::size_t b_end) {
        for (; i < a_end; ++i) {
            out.push_back({DiffKind::Remove, before[i]});
        }
        for (; j < b_end; ++j) {
            out.push_back({DiffKind::Add, after[j]});
        }
    };
    for (const auto& [a_at, b_at] : kept) {
        emit_changes_until(a_at, b_at);
        out.push_back({DiffKind::Keep, before[i]});
        ++i;
        ++j;
    }
    emit_changes_until(before.size(), after.size());
    return out;
}
