#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "practice.hpp"

class WordFrequencies {
public:
    // Stage 1
    void add_text(std::string_view text) { PRACTICE_TODO(); }
    std::int64_t count(std::string_view word) const { PRACTICE_TODO(); }
    std::size_t distinct_words() const { PRACTICE_TODO(); }

    // Stage 4: calls callback(word, count) once per distinct word.
    template <typename Callback>
    void for_each(Callback&& callback) const {
        PRACTICE_TODO();
    }
};

// Stage 2
inline std::int64_t best_restoration_score(const WordFrequencies& frequencies,
                                           std::string_view corrupted) {
    PRACTICE_TODO();
}

// Stage 3
struct Restoration {
    std::int64_t score;
    std::vector<std::string_view> words;
};

inline Restoration restore(const WordFrequencies& frequencies, std::string_view corrupted) {
    PRACTICE_TODO();
}

// Stage 4
class FastRestorer {
public:
    explicit FastRestorer(const WordFrequencies& frequencies) { PRACTICE_TODO(); }

    Restoration restore(std::string_view corrupted) const { PRACTICE_TODO(); }
};
