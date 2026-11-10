#pragma once

#include <span>
#include <utility>
#include <vector>

#include "practice.hpp"

// Stage 1
struct Cnf {
    int num_vars = 0;
    std::vector<std::vector<int>> clauses;
};
using Assignment = std::vector<bool>;  // size num_vars + 1, index 0 unused

inline bool satisfies(const Cnf& f, const Assignment& a) { PRACTICE_TODO(); }
inline Cnf reduce_sat_to_triple_sat(const Cnf& f) { PRACTICE_TODO(); }
inline Assignment lift_triple_sat_to_sat(const Cnf& f, const Assignment& a) { PRACTICE_TODO(); }

// Stage 2
struct DonutInstance {
    int n = 0;
    std::vector<std::pair<int, int>> edges;
    std::vector<long long> profit;
    long long k = 0;
};
inline DonutInstance reduce_3sat_to_donut(const Cnf& f) { PRACTICE_TODO(); }
inline Assignment lift_donut_to_3sat(const Cnf& f, const DonutInstance& inst,
                                     std::span<const int> chosen) {
    PRACTICE_TODO();
}

// Stage 3
struct Task {
    long long time = 0;
    long long profit = 0;
    long long deadline = 0;
};
struct ScheduledTask {
    int task = 0;
    long long start = 0;
};
using Schedule = std::vector<ScheduledTask>;

inline bool valid_schedule(std::span<const Task> tasks, const Schedule& schedule) {
    PRACTICE_TODO();
}
inline long long schedule_profit(std::span<const Task> tasks, const Schedule& schedule) {
    PRACTICE_TODO();
}

struct SubsetSumInstance {
    std::vector<long long> a;
    long long t = 0;
};
struct SchedInstance {
    std::vector<Task> tasks;
    long long k = 0;
};
inline SchedInstance reduce_subset_sum_to_sched(const SubsetSumInstance& src) { PRACTICE_TODO(); }
inline std::vector<int> lift_sched_to_subset_sum(const SubsetSumInstance& src,
                                                 const SchedInstance& dst,
                                                 const Schedule& schedule) {
    PRACTICE_TODO();
}

// Stage 4
inline Schedule best_schedule(std::span<const Task> tasks) { PRACTICE_TODO(); }
