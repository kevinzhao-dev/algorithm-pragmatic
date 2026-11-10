#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class AnagramIndex {
public:
    AnagramIndex(std::string_view text, std::size_t k) : k_(k), ring_(k, '\0') {
        if (k == 0) {
            throw std::invalid_argument("k must be positive");
        }
        counts_.reserve(text.size() / 2 + 8);
        for (const char ch : text) {
            append(ch);
        }
    }

    std::size_t count(std::string_view query) const {
        if (query.size() != k_) {
            throw std::invalid_argument("query length must equal k");
        }
        Frequency frequency{};
        for (const char ch : query) {
            ++frequency[static_cast<std::size_t>(ch - 'a')];
        }
        const auto it = counts_.find(frequency);
        return it == counts_.end() ? 0 : it->second;
    }

    void append(char ch) {
        ++window_[static_cast<std::size_t>(ch - 'a')];
        ++length_;
        const std::size_t slot = (length_ - 1) % k_;
        if (length_ > k_) {
            --window_[static_cast<std::size_t>(ring_[slot] - 'a')];
        }
        ring_[slot] = ch;
        if (length_ >= k_) {
            ++counts_[window_];
        }
    }

    std::size_t length() const { return length_; }

private:
    using Frequency = std::array<std::uint32_t, 26>;

    struct FrequencyHash {
        std::size_t operator()(const Frequency& frequency) const {
            std::uint64_t state = 0xCBF29CE484222325ULL;
            for (const std::uint32_t value : frequency) {
                state ^= value;
                state *= 0x100000001B3ULL;
            }
            return static_cast<std::size_t>(state ^ (state >> 32));
        }
    };

    std::size_t k_;
    std::string ring_;  // the last k characters, only needed to expire the oldest one
    Frequency window_{};
    std::size_t length_ = 0;
    std::unordered_map<Frequency, std::size_t, FrequencyHash> counts_;
};

inline std::vector<std::size_t> count_anagram_substrings(std::string_view text,
                                                         std::span<const std::string> queries) {
    if (queries.empty()) {
        return {};
    }
    const AnagramIndex index(text, queries.front().size());
    std::vector<std::size_t> answers;
    answers.reserve(queries.size());
    for (const std::string& query : queries) {
        answers.push_back(index.count(query));
    }
    return answers;
}
