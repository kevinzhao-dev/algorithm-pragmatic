#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "practice.hpp"

struct Task {
    int priority;  // all distinct; larger means more important
    int demand;
};

struct Server {
    int latency;  // all distinct
    int capacity;
};

// Stage 1, tightened in Stage 3
inline std::optional<std::int64_t> min_overload(std::span<const Task> tasks,
                                                std::span<const Server> servers) {
    PRACTICE_TODO();
}

// Stage 2
inline std::optional<std::vector<std::size_t>> assign_servers(std::span<const Task> tasks,
                                                              std::span<const Server> servers) {
    PRACTICE_TODO();
}

// Stage 4
struct DropPlan {
    std::int64_t cost;
    std::vector<std::optional<std::size_t>> assignment;
};

inline DropPlan plan_with_drops(std::span<const Task> tasks, std::span<const Server> servers,
                                std::span<const std::int64_t> drop_penalty) {
    PRACTICE_TODO();
}
