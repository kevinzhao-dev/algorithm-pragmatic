#include "workload.hpp"

#include "forkjoin.hpp"

namespace workload {

namespace {

std::uint64_t units = 0;
std::uint64_t clock_ticks = 0;

__attribute__((no_instrument_function)) void charge(std::uint64_t cost) { clock_ticks += cost; }

}  // namespace

__attribute__((no_instrument_function)) std::uint64_t work_units() { return units; }
__attribute__((no_instrument_function)) std::uint64_t ticks() { return clock_ticks; }

void spawn_marker() {}

long long fib_spawn(int n, int base) {
    ++units;
    charge(kUnitCost);
    if (n < 2) {
        return n;
    }
    if (n < base) {
        return fib_spawn(n - 1, base) + fib_spawn(n - 2, base);
    }
    charge(kSpawnCost);
    spawn_marker();
    long long x = 0, y = 0;
    practice::fork2([&] { x = fib_spawn(n - 1, base); }, [&] { y = fib_spawn(n - 2, base); });
    return x + y;
}

}  // namespace workload
