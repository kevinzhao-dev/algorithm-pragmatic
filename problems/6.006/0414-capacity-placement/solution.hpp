#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "practice.hpp"

class CapacityPlanner {
public:
    using MachineId = std::uint64_t;

    struct Machine {
        MachineId id;
        std::uint64_t capacity;
    };

    // Stage 1
    explicit CapacityPlanner(std::span<const Machine> machines) { PRACTICE_TODO(); }
    std::optional<MachineId> place(const std::string& task, std::uint64_t demand) { PRACTICE_TODO(); }
    std::uint64_t available(MachineId machine) const { PRACTICE_TODO(); }

    // Stage 2
    void release(const std::string& task) { PRACTICE_TODO(); }

    // Stage 3
    std::vector<std::string> tasks_on(MachineId machine) const { PRACTICE_TODO(); }

    // Stage 4
    bool drain(MachineId machine) { PRACTICE_TODO(); }
};
