#pragma once

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include "practice.hpp"

// Stage 1
inline std::uint64_t binomial(std::uint64_t n, std::uint64_t k) { PRACTICE_TODO(); }

template <typename Domain, typename F>
std::vector<std::decay_t<std::invoke_result_t<F&, std::ranges::range_reference_t<const Domain>>>>
set_of(const Domain& domain, F f) {
    PRACTICE_TODO();
}

// Stage 2
template <typename T>
std::vector<T> intersect(std::span<const T> a, std::span<const T> b) {
    PRACTICE_TODO();
}

template <typename T>
std::size_t union_size(std::span<const T> a, std::span<const T> b) {
    PRACTICE_TODO();
}

template <typename T>
std::size_t difference_size(std::span<const T> a, std::span<const T> b) {
    PRACTICE_TODO();
}

// Stage 3
template <typename T>
std::vector<T> gallop_intersect(std::span<const T> a, std::span<const T> b) {
    PRACTICE_TODO();
}

// Stage 4
class PostingIndex {
public:
    void add(std::string term, std::vector<std::int64_t> doc_ids) { PRACTICE_TODO(); }

    std::vector<std::int64_t> query(std::span<const std::string> must,
                                    std::span<const std::string> must_not = {}) const {
        PRACTICE_TODO();
    }
};
