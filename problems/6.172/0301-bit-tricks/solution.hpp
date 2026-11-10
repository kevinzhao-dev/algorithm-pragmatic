#pragma once

#include <cstddef>
#include <cstdint>

#include "practice.hpp"

// Stage 1
inline std::uint64_t set_bit(std::uint64_t x, unsigned k) { PRACTICE_TODO(); }
inline std::uint64_t clear_bit(std::uint64_t x, unsigned k) { PRACTICE_TODO(); }
inline std::uint64_t toggle_bit(std::uint64_t x, unsigned k) { PRACTICE_TODO(); }
inline std::uint64_t extract_field(std::uint64_t x, unsigned lo, unsigned len) { PRACTICE_TODO(); }
inline std::uint64_t insert_field(std::uint64_t x, unsigned lo, unsigned len, std::uint64_t value) {
    PRACTICE_TODO();
}

// Stage 2
inline std::int64_t min_no_branch(std::int64_t x, std::int64_t y) { PRACTICE_TODO(); }
inline std::int64_t abs_no_branch(std::int64_t x) { PRACTICE_TODO(); }
inline std::uint64_t mod_add_no_branch(std::uint64_t a, std::uint64_t b, std::uint64_t n) {
    PRACTICE_TODO();
}
inline void merge_no_branch(const int* a, std::size_t na, const int* b, std::size_t nb, int* out) {
    PRACTICE_TODO();
}

// Stage 3
inline std::uint64_t ceil_pow2(std::uint64_t x) { PRACTICE_TODO(); }
inline std::uint64_t lowest_set_bit(std::uint64_t x) { PRACTICE_TODO(); }
inline int count_trailing_zeros(std::uint64_t x) { PRACTICE_TODO(); }
inline int popcount_table(std::uint64_t x) { PRACTICE_TODO(); }
inline int popcount_parallel(std::uint64_t x) { PRACTICE_TODO(); }
inline std::uint64_t reverse_bits(std::uint64_t x) { PRACTICE_TODO(); }

// Stage 4
class SetBits {
public:
    struct Iterator {
        unsigned operator*() const { PRACTICE_TODO(); }
        Iterator& operator++() { PRACTICE_TODO(); }
        friend bool operator==(const Iterator& a, const Iterator& b) { PRACTICE_TODO(); }
    };

    explicit SetBits(std::uint64_t word) { PRACTICE_TODO(); }

    Iterator begin() const { PRACTICE_TODO(); }
    Iterator end() const { PRACTICE_TODO(); }
};

inline std::uint64_t next_with_same_popcount(std::uint64_t x) { PRACTICE_TODO(); }
