// Program under test: fib with fork2 above the cut-off, once instrumented and once plain.
#pragma once

#include <cstdint>

namespace workload {

// A virtual machine model, in ticks: T = kUnitCost * I + kSpawnCost * S exactly.
inline constexpr std::uint64_t kUnitCost = 2;
inline constexpr std::uint64_t kSpawnCost = 37;

std::uint64_t work_units();
std::uint64_t ticks();

// Called immediately before every fork2: the equivalent of the CSI detach hook.
void spawn_marker();

// Spawns the first recursive call when n >= base, recurses serially below base.
long long fib_spawn(int n, int base);

}  // namespace workload

namespace workload_plain {

long long fib_spawn(int n, int base);

// An opaque function whose cost is one real call.
void empty_call();

}  // namespace workload_plain
