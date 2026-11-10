#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

struct EditCosts {
    int insert = 1;
    int erase = 1;
    int replace = 1;
};

inline int edit_distance(std::string_view from, std::string_view to, EditCosts costs = {}) {
    if (to.size() > from.size()) {
        // Turning `to` into `from` swaps the roles of insert and erase.
        std::swap(from, to);
        std::swap(costs.insert, costs.erase);
    }
    const std::size_t m = to.size();
    std::vector<int> row(m + 1);
    for (std::size_t j = 0; j <= m; ++j) {
        row[j] = static_cast<int>(j) * costs.insert;
    }
    for (std::size_t i = 1; i <= from.size(); ++i) {
        int diagonal = row[0];
        row[0] = static_cast<int>(i) * costs.erase;
        for (std::size_t j = 1; j <= m; ++j) {
            const int above = row[j];
            row[j] = from[i - 1] == to[j - 1]
                         ? diagonal
                         : std::min({above + costs.erase, row[j - 1] + costs.insert,
                                     diagonal + costs.replace});
            diagonal = above;
        }
    }
    return row[m];
}

enum class EditOp { Insert, Erase, Replace };

struct Edit {
    EditOp op;
    std::size_t at;
    char ch;
};

inline std::vector<Edit> edit_script(std::string_view from, std::string_view to,
                                     EditCosts costs = {}) {
    const std::size_t n = from.size();
    const std::size_t m = to.size();
    std::vector<int> table((n + 1) * (m + 1));
    const auto cell = [&](std::size_t i, std::size_t j) -> int& { return table[i * (m + 1) + j]; };
    for (std::size_t i = 0; i <= n; ++i) {
        for (std::size_t j = 0; j <= m; ++j) {
            if (i == 0) {
                cell(i, j) = static_cast<int>(j) * costs.insert;
            } else if (j == 0) {
                cell(i, j) = static_cast<int>(i) * costs.erase;
            } else if (from[i - 1] == to[j - 1]) {
                cell(i, j) = cell(i - 1, j - 1);
            } else {
                cell(i, j) = std::min({cell(i - 1, j) + costs.erase, cell(i, j - 1) + costs.insert,
                                       cell(i - 1, j - 1) + costs.replace});
            }
        }
    }

    // Walk back from the end; `at` is filled in on the forward pass below, since it depends on
    // everything applied before it.
    enum class Step { Keep, Replace, Erase, Insert };
    std::vector<std::pair<Step, char>> backward;
    std::size_t i = n;
    std::size_t j = m;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && from[i - 1] == to[j - 1]) {
            backward.emplace_back(Step::Keep, '\0');
            --i;
            --j;
        } else if (i > 0 && j > 0 && cell(i, j) == cell(i - 1, j - 1) + costs.replace) {
            backward.emplace_back(Step::Replace, to[j - 1]);
            --i;
            --j;
        } else if (i > 0 && cell(i, j) == cell(i - 1, j) + costs.erase) {
            backward.emplace_back(Step::Erase, '\0');
            --i;
        } else {
            backward.emplace_back(Step::Insert, to[j - 1]);
            --j;
        }
    }

    std::vector<Edit> script;
    std::size_t position = 0;
    for (auto step = backward.rbegin(); step != backward.rend(); ++step) {
        switch (step->first) {
            case Step::Keep:
                ++position;
                break;
            case Step::Replace:
                script.push_back({EditOp::Replace, position++, step->second});
                break;
            case Step::Insert:
                script.push_back({EditOp::Insert, position++, step->second});
                break;
            case Step::Erase:
                script.push_back({EditOp::Erase, position, '\0'});
                break;
        }
    }
    return script;
}

inline std::optional<int> edit_distance_within(std::string_view from, std::string_view to,
                                               int limit) {
    if (limit < 0) {
        return std::nullopt;
    }
    if (from.size() < to.size()) {
        std::swap(from, to);
    }
    const int n = static_cast<int>(from.size());
    const int m = static_cast<int>(to.size());
    if (n - m > limit) {
        return std::nullopt;
    }
    // The distance never exceeds the longer length, so a wider band than that buys nothing.
    const int band = std::min(limit, n);
    const int infinity = band + 1;
    // A cell (i, j) is stored at index j - i + band + 1, with one sentinel on each side.
    std::vector<int> previous(static_cast<std::size_t>(2 * band + 3), infinity);
    std::vector<int> current = previous;
    for (int j = 0; j <= std::min(band, m); ++j) {
        previous[static_cast<std::size_t>(j + band + 1)] = j;
    }

    for (int i = 1; i <= n; ++i) {
        std::ranges::fill(current, infinity);
        const int low = std::max(-band, -i);
        const int high = std::min(band, m - i);
        bool within_limit = false;
        for (int d = low; d <= high; ++d) {
            const int j = i + d;
            const auto at = static_cast<std::size_t>(d + band + 1);
            int best = previous[at + 1] + 1;  // erase
            if (j == 0) {
                best = std::min(best, i);
            } else {
                best = std::min(best, previous[at] + (from[static_cast<std::size_t>(i - 1)] !=
                                                      to[static_cast<std::size_t>(j - 1)]));
                best = std::min(best, current[at - 1] + 1);  // insert
            }
            current[at] = std::min(best, infinity);
            within_limit = within_limit || current[at] <= band;
        }
        if (!within_limit) {
            return std::nullopt;  // Costs never decrease along an edit path.
        }
        std::swap(previous, current);
    }
    const int distance = previous[static_cast<std::size_t>(m - n + band + 1)];
    return distance <= limit ? std::optional<int>(distance) : std::nullopt;
}

struct Suggestion {
    std::string_view word;
    int distance;
};

inline std::vector<Suggestion> suggest(std::string_view query,
                                       std::span<const std::string> dictionary, int limit) {
    std::vector<Suggestion> matches;
    for (const std::string& word : dictionary) {
        if (const auto distance = edit_distance_within(query, word, limit)) {
            matches.push_back({word, *distance});
        }
    }
    std::ranges::sort(matches, [](const Suggestion& a, const Suggestion& b) {
        return std::pair(a.distance, a.word) < std::pair(b.distance, b.word);
    });
    return matches;
}
