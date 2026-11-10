#pragma once

#include <span>
#include <vector>

#include "practice.hpp"

// Stage 1
inline std::vector<int> spt_order(std::span<const long long> durations) { PRACTICE_TODO(); }
inline long long total_completion_time(std::span<const long long> durations,
                                       std::span<const int> order) {
    PRACTICE_TODO();
}

// Stage 2
struct Job {
    long long release = 0;
    long long duration = 0;
};
inline std::vector<long long> srpt_completion(std::span<const Job> jobs) { PRACTICE_TODO(); }

// Stage 3
struct Interval {
    long long start = 0;
    long long finish = 0;
};
struct Allocation {
    int machines = 0;
    std::vector<int> assignment;
};
inline Allocation allocate(std::span<const Interval> intervals) { PRACTICE_TODO(); }
