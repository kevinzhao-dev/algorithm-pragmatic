#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "practice.hpp"

class Profiler {
public:
    using Clock = std::function<std::uint64_t()>;

    struct Entry {
        std::string name;
        std::uint64_t calls = 0;
        std::uint64_t inclusive = 0;
        std::uint64_t self = 0;
    };

    // Stage 1
    explicit Profiler(Clock clock) { PRACTICE_TODO(); }

    void enter(std::string_view name) { PRACTICE_TODO(); }
    void exit() { PRACTICE_TODO(); }
    std::size_t depth() const { PRACTICE_TODO(); }
    std::vector<Entry> report() const { PRACTICE_TODO(); }

    // Stage 3
    std::vector<std::string> folded() const { PRACTICE_TODO(); }
};

// Stage 2
class ScopedPhase {
public:
    ScopedPhase(Profiler& profiler, std::string_view name) { PRACTICE_TODO(); }
    ~ScopedPhase() {}

    ScopedPhase(const ScopedPhase&) = delete;
    ScopedPhase& operator=(const ScopedPhase&) = delete;
};

// Stage 4
inline std::vector<Profiler::Entry> report_from_samples(
    std::span<const std::vector<std::string>> stacks) {
    PRACTICE_TODO();
}
