#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "practice.hpp"

struct Pipe {
    int a;
    int b;
    std::int64_t length;
};

struct HeadDrop {
    int junction;
    std::int64_t new_head;
};

struct PipeNetwork {
    std::vector<std::int64_t> head;
    std::vector<Pipe> pipes;
    std::vector<HeadDrop> drops;
    int valve;
};

// Stages 1 and 2
inline std::optional<std::int64_t> shortest_drift(const PipeNetwork& network, int source, int sink) {
    PRACTICE_TODO();
}

// Stage 3
struct DriftPlan {
    std::int64_t length;
    std::vector<int> route;
    std::optional<std::size_t> trigger;
};

inline std::optional<DriftPlan> plan_drift(const PipeNetwork& network, int source, int sink) {
    PRACTICE_TODO();
}
