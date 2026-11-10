#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>
#include <vector>

#include "practice.hpp"

// Stage 1
class ComputationDag {
public:
    explicit ComputationDag(std::size_t strands) { PRACTICE_TODO(); }

    void add_edge(std::size_t before, std::size_t after) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    const std::vector<std::size_t>& successors(std::size_t strand) const { PRACTICE_TODO(); }

    std::size_t work() const { PRACTICE_TODO(); }
    std::size_t span() const { PRACTICE_TODO(); }
    double parallelism() const { PRACTICE_TODO(); }
};

// Stage 2
struct Schedule {
    std::vector<std::size_t> step_of;
    std::size_t length = 0;
};

inline Schedule greedy_schedule(const ComputationDag& dag, std::size_t processors,
                                const std::vector<std::size_t>& priority) {
    PRACTICE_TODO();
}

// Stage 3
struct NearTwiceInstance {
    ComputationDag dag;
    std::vector<std::size_t> good_priority;
    std::vector<std::size_t> bad_priority;
};

inline NearTwiceInstance near_twice_instance(std::size_t processors, std::size_t chain_length) {
    PRACTICE_TODO();
}

// Stage 4
struct BuildTask {
    long long duration = 1;
    std::vector<std::size_t> deps;
};

struct BuildPlan {
    long long total_work = 0;
    long long critical_path = 0;
    long long makespan = 0;
};

inline BuildPlan plan_build(std::span<const BuildTask> tasks, std::size_t workers) {
    PRACTICE_TODO();
}
