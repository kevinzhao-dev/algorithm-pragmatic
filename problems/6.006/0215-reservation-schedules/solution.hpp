#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <span>
#include <stdexcept>
#include <vector>

#include "practice.hpp"

struct JobRequest {
    std::int64_t start;
    std::int64_t end;
};

struct Reservation {
    std::int64_t machines;
    std::int64_t start;
    std::int64_t end;
    friend bool operator==(const Reservation&, const Reservation&) = default;
    friend std::ostream& operator<<(std::ostream& os, const Reservation& r) {
        return os << '(' << r.machines << ", [" << r.start << ',' << r.end << "))";
    }
};

using Schedule = std::vector<Reservation>;

// Stage 1
inline Schedule merge_schedules(const Schedule& a, const Schedule& b) { PRACTICE_TODO(); }

// Stage 2
inline Schedule satisfying_schedule(std::span<const JobRequest> jobs) { PRACTICE_TODO(); }

// Stage 3
class IncrementalPlanner {
public:
    void add(JobRequest job) { PRACTICE_TODO(); }
    Schedule schedule() const { PRACTICE_TODO(); }
    std::int64_t peak() const { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
};
