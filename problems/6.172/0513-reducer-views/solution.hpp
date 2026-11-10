#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "forkjoin.hpp"
#include "practice.hpp"

template <typename M>
concept Monoid = requires(typename M::value_type& left, typename M::value_type&& right) {
    { M::identity() } -> std::convertible_to<typename M::value_type>;
    M::reduce(left, std::move(right));
};

template <Monoid M>
class Reducer {
public:
    using value_type = typename M::value_type;

    // Stage 1
    Reducer() { PRACTICE_TODO(); }
    explicit Reducer(value_type initial) { PRACTICE_TODO(); }

    value_type& view() { PRACTICE_TODO(); }

    template <typename F, typename G>
    void fork2(F&& f, G&& g) {
        PRACTICE_TODO();
    }

    template <typename Body>
    void parallel_for(std::size_t begin, std::size_t end, Body&& body, std::size_t grain = 1) {
        PRACTICE_TODO();
    }

    value_type take() { PRACTICE_TODO(); }
};

// Stage 3
inline std::vector<std::uint64_t> queens_ordered(int n, int serial_depth) { PRACTICE_TODO(); }
