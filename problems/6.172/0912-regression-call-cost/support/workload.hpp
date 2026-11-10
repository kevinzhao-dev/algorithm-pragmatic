// Program under test: the same fib_base twice, once instrumented and once plain.
#pragma once

#include <cstdint>

namespace workload {

// A virtual machine model, in ticks: T = kUnitCost * I + kCallCost * F exactly.
inline constexpr std::uint64_t kUnitCost = 3;
inline constexpr std::uint64_t kCallCost = 11;

std::uint64_t work_units();
std::uint64_t ticks();

// Recursive above `base`, iterative below it. `base` is at least 2.
long long fib_base(int n, int base);

}  // namespace workload

namespace workload_plain {

// The same computation with no instrumentation and no accounting, for honest wall-clock timing.
long long fib_base(int n, int base);

}  // namespace workload_plain
