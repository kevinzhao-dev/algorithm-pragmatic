#include "workload.hpp"

#include "forkjoin.hpp"

namespace workload_plain {

long long fib_spawn(int n, int base) {
    if (n < 2) {
        return n;
    }
    if (n < base) {
        return fib_spawn(n - 1, base) + fib_spawn(n - 2, base);
    }
    long long x = 0, y = 0;
    practice::fork2([&] { x = fib_spawn(n - 1, base); }, [&] { y = fib_spawn(n - 2, base); });
    return x + y;
}

__attribute__((noinline)) void empty_call() {
    asm volatile("");  // keeps the call and the empty body from being optimized away
}

}  // namespace workload_plain
