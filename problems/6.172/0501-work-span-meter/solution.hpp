#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "forkjoin.hpp"
#include "practice.hpp"

// Stage 1
struct Cost {
    long long work = 0;
    long long span = 0;

    double parallelism() const { PRACTICE_TODO(); }
};

inline Cost series(Cost a, Cost b) { PRACTICE_TODO(); }
inline Cost parallel(Cost a, Cost b) { PRACTICE_TODO(); }

// Stage 2
class Meter {
public:
    void charge(long long units = 1) { PRACTICE_TODO(); }

    template <typename F, typename G>
    void fork2(F&& f, G&& g) {
        PRACTICE_TODO();
    }

    template <typename Body>
    void parallel_for(std::size_t begin, std::size_t end, Body&& body, std::size_t grain = 1) {
        PRACTICE_TODO();
    }

    Cost cost() const { PRACTICE_TODO(); }
    long long spawns() const { PRACTICE_TODO(); }
};

// Stage 3
template <typename Exec>
long long fib(Exec& exec, int n, int cutoff) {
    PRACTICE_TODO();
}

// Stage 4
class ForkJoinExec {
public:
    void charge(long long units = 1) const { PRACTICE_TODO(); }

    template <typename F, typename G>
    void fork2(F&& f, G&& g) const {
        PRACTICE_TODO();
    }

    template <typename Body>
    void parallel_for(std::size_t begin, std::size_t end, Body&& body, std::size_t grain = 1) const {
        PRACTICE_TODO();
    }
};

template <typename Exec>
long long parallel_sum(Exec& exec, std::span<const int> data, std::size_t grain) {
    PRACTICE_TODO();
}

template <typename Exec>
void transpose_in_place(Exec& exec, std::span<int> grid, std::size_t n, std::size_t grain) {
    PRACTICE_TODO();
}

// Stage 5
struct SpeedupBounds {
    double lower = 0;
    double upper = 0;
};

inline SpeedupBounds speedup_bounds(Cost cost, int processors) { PRACTICE_TODO(); }
