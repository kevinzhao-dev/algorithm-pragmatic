#pragma once

#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

#include "practice.hpp"

struct Measurement {
    int processors = 1;
    double seconds = 0;
};

struct Audit {
    bool consistent = false;
    double min_work = 0;
    double max_work = 0;
    double min_span = 0;
    double max_span = 0;
    double min_parallelism = 0;
    double max_parallelism = 0;
};

// Stage 1
inline Audit audit_measurements(std::span<const Measurement> measurements) { PRACTICE_TODO(); }

// Stage 2
inline std::vector<std::size_t> minimal_conflict(std::span<const Measurement> measurements) {
    PRACTICE_TODO();
}

// Stage 3
struct RuntimeBounds {
    double low = 0;
    double high = 0;
};

inline RuntimeBounds predict_runtime(std::span<const Measurement> measurements, int processors) {
    PRACTICE_TODO();
}
