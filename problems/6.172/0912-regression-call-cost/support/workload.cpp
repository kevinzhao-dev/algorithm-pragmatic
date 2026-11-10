#include "workload.hpp"

namespace workload {

namespace {

std::uint64_t units = 0;
std::uint64_t clock_ticks = 0;

}  // namespace

__attribute__((no_instrument_function)) std::uint64_t work_units() { return units; }
__attribute__((no_instrument_function)) std::uint64_t ticks() { return clock_ticks; }

long long fib_base(int n, int base) {
    clock_ticks += kCallCost;
    if (n < base) {
        long long a = 0, b = 1;
        for (int i = 0; i < n; ++i) {
            ++units;
            clock_ticks += kUnitCost;
            const long long next = a + b;
            a = b;
            b = next;
        }
        return a;
    }
    return fib_base(n - 1, base) + fib_base(n - 2, base);
}

}  // namespace workload
