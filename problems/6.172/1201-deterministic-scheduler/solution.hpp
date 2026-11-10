#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "practice.hpp"

struct RunOutcome {
    std::vector<int> schedule;  // id of the thread chosen at every point with a real choice
    bool deadlocked = false;
};

// Stage 1
inline RunOutcome run_schedule(std::vector<std::function<void()>> bodies,
                               const std::vector<int>& choices) {
    PRACTICE_TODO();
}
inline void yield_point() { PRACTICE_TODO(); }

// Stage 2
inline RunOutcome run_random(std::vector<std::function<void()>> bodies, std::uint64_t seed) {
    PRACTICE_TODO();
}
inline RunOutcome replay_schedule(std::vector<std::function<void()>> bodies,
                                  const std::vector<int>& schedule) {
    PRACTICE_TODO();
}

// Stage 3
class SchedMutex {
public:
    SchedMutex() { PRACTICE_TODO(); }
    SchedMutex(const SchedMutex&) = delete;
    SchedMutex& operator=(const SchedMutex&) = delete;

    void lock() { PRACTICE_TODO(); }
    bool try_lock() { PRACTICE_TODO(); }
    void unlock() { PRACTICE_TODO(); }
};

// Stage 4
struct Scenario {
    std::vector<std::function<void()>> bodies;
    std::function<bool()> check;
};

struct ExploreSummary {
    long runs = 0;
    long failures = 0;
    long deadlocks = 0;
    std::vector<int> bad_schedule;  // the first failing schedule, for replay
};

inline ExploreSummary explore_all(const std::function<Scenario()>& make) { PRACTICE_TODO(); }
