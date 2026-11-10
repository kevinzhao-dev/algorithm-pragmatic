#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "08_ols.hpp"
#include "practice.hpp"

namespace profiler {

struct FunctionStats {
    std::uint64_t calls = 0;
    std::uint64_t inclusive_ticks = 0;
    std::uint64_t exclusive_ticks = 0;
};

struct ReportLine {
    std::string name;
    std::uint64_t calls = 0;
    friend bool operator==(const ReportLine&, const ReportLine&) = default;
};

using Clock = std::uint64_t (*)();

// Stage 1
void start();
void stop();
std::uint64_t calls(const void* function);

// Stage 2
std::vector<ReportLine> report();

// Stage 3
void set_clock(Clock clock);
FunctionStats stats(const void* function);

// Stage 4
practice::ols::Growth estimate_growth(const void* function, void (*run)(int),
                                      std::span<const int> sizes);

}  // namespace profiler

inline std::ostream& operator<<(std::ostream& os, const profiler::ReportLine& line) {
    return os << line.name << " x" << line.calls;
}
