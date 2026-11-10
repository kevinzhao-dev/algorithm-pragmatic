#include "workload.hpp"

namespace workload_plain {

namespace {

// Keeps every recursive step a real call: without this, the compiler turns one of the two
// recursive calls into a loop and the "cost of a function call" would be measured on a loop.
long long (*volatile self)(int, int) = nullptr;

}  // namespace

long long fib_base(int n, int base) {
    self = &fib_base;
    if (n < base) {
        long long a = 0, b = 1;
        for (int i = 0; i < n; ++i) {
            const long long next = a + b;
            a = b;
            b = next;
        }
        return a;
    }
    return self(n - 1, base) + self(n - 2, base);
}

}  // namespace workload_plain
