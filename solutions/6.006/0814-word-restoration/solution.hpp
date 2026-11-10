#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

class WordFrequencies {
public:
    void add_text(std::string_view text) {
        for (std::size_t at = 0; at < text.size();) {
            if (text[at] == ' ') {
                ++at;
                continue;
            }
            std::size_t end = at;
            while (end < text.size() && text[end] != ' ') {
                ++end;
            }
            ++counts_[std::string(text.substr(at, end - at))];
            at = end;
        }
    }

    std::int64_t count(std::string_view word) const {
        const auto found = counts_.find(word);
        return found == counts_.end() ? 0 : found->second;
    }

    std::size_t distinct_words() const { return counts_.size(); }

    template <typename Callback>
    void for_each(Callback&& callback) const {
        for (const auto& [word, count] : counts_) {
            callback(std::string_view(word), count);
        }
    }

private:
    struct TransparentHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view text) const {
            return std::hash<std::string_view>{}(text);
        }
    };

    std::unordered_map<std::string, std::int64_t, TransparentHash, std::equal_to<>> counts_;
};

struct Restoration {
    std::int64_t score;
    std::vector<std::string_view> words;
};

inline Restoration restore(const WordFrequencies& frequencies, std::string_view corrupted) {
    const std::size_t n = corrupted.size();
    std::vector<std::int64_t> best(n + 1, 0);
    std::vector<std::size_t> cut(n + 1, n);
    for (std::size_t j = n; j-- > 0;) {
        best[j] = std::numeric_limits<std::int64_t>::min();
        for (std::size_t end = j + 1; end <= n; ++end) {
            const std::int64_t candidate =
                frequencies.count(corrupted.substr(j, end - j)) + best[end];
            if (candidate > best[j]) {
                best[j] = candidate;
                cut[j] = end;
            }
        }
    }
    Restoration result{best[0], {}};
    for (std::size_t j = 0; j < n; j = cut[j]) {
        result.words.push_back(corrupted.substr(j, cut[j] - j));
    }
    return result;
}

inline std::int64_t best_restoration_score(const WordFrequencies& frequencies,
                                           std::string_view corrupted) {
    return restore(frequencies, corrupted).score;
}

class FastRestorer {
public:
    explicit FastRestorer(const WordFrequencies& frequencies) : nodes_(1) {
        frequencies.for_each([&](std::string_view word, std::int64_t count) {
            std::size_t at = 0;
            for (const char ch : word) {
                const auto letter = static_cast<std::size_t>(ch - 'a');
                if (nodes_[at].next[letter] == 0) {
                    nodes_[at].next[letter] = static_cast<std::int32_t>(nodes_.size());
                    nodes_.emplace_back();
                }
                at = static_cast<std::size_t>(nodes_[at].next[letter]);
            }
            nodes_[at].count = count;
        });
    }

    Restoration restore(std::string_view corrupted) const {
        const std::size_t n = corrupted.size();
        std::vector<Cell> best(n + 1);
        std::vector<std::size_t> cut(n + 1, n);
        best[n] = {0, 0};
        // Best suffix among all positions after j, for the "one long unknown word" choice.
        Cell tail = best[n];
        std::size_t tail_at = n;
        for (std::size_t j = n; j-- > 0;) {
            Cell candidate{tail.score, tail.words + 1};
            std::size_t choice = tail_at;
            std::size_t node = 0;
            for (std::size_t end = j; end < n; ++end) {
                const auto letter = static_cast<std::size_t>(corrupted[end] - 'a');
                const auto next = static_cast<std::size_t>(nodes_[node].next[letter]);
                if (next == 0) {
                    break;
                }
                node = next;
                if (nodes_[node].count > 0) {
                    const Cell through{nodes_[node].count + best[end + 1].score,
                                       best[end + 1].words + 1};
                    if (through.better_than(candidate)) {
                        candidate = through;
                        choice = end + 1;
                    }
                }
            }
            best[j] = candidate;
            cut[j] = choice;
            if (best[j].better_than(tail)) {
                tail = best[j];
                tail_at = j;
            }
        }

        Restoration result{best[0].score, {}};
        for (std::size_t j = 0; j < n; j = cut[j]) {
            result.words.push_back(corrupted.substr(j, cut[j] - j));
        }
        return result;
    }

private:
    struct Cell {
        std::int64_t score = 0;
        std::int64_t words = 0;
        // Higher score wins; among equal scores, fewer words wins.
        bool better_than(const Cell& other) const {
            return score != other.score ? score > other.score : words < other.words;
        }
    };

    struct Node {
        std::array<std::int32_t, 26> next{};  // 0 means no child: the root is never a child
        std::int64_t count = 0;
    };

    std::vector<Node> nodes_;
};
