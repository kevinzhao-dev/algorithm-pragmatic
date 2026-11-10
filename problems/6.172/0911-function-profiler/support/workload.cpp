#include "workload.hpp"

#include <algorithm>
#include <vector>

namespace workload {

namespace {

std::uint64_t virtual_ticks = 0;

}  // namespace

__attribute__((no_instrument_function)) std::uint64_t now() { return virtual_ticks; }
__attribute__((no_instrument_function)) void burn(std::uint64_t ticks) { virtual_ticks += ticks; }
__attribute__((no_instrument_function)) void set_now(std::uint64_t ticks) { virtual_ticks = ticks; }

void leaf() {}

long long fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }

bool is_even(int n) { return n == 0 ? true : is_odd(n - 1); }
bool is_odd(int n) { return n == 0 ? false : is_even(n - 1); }

void outer() {
    burn(10);
    inner();
    burn(5);
}

void inner() { burn(7); }

void recurse(int depth) {
    burn(2);
    if (depth > 0) {
        recurse(depth - 1);
    }
    burn(1);
}

void ping(int n) {
    burn(1);
    if (n > 0) {
        pong(n - 1);
    }
}

void pong(int n) {
    burn(2);
    if (n > 0) {
        ping(n - 1);
    }
}

void loop_leaves(int n) {
    for (int i = 0; i < n; ++i) {
        leaf();
    }
}

void nested_leaves(int n) {
    for (int i = 0; i < n; ++i) {
        loop_leaves(n);
    }
}

bool compare(int a, int b) { return a < b; }

namespace {

void merge(std::vector<int>& values, std::vector<int>& scratch, std::size_t lo, std::size_t mid,
           std::size_t hi) {
    std::size_t left = lo, right = mid, out = lo;
    while (left < mid && right < hi) {
        scratch[out++] = compare(values[right], values[left]) ? values[right++] : values[left++];
    }
    while (left < mid) {
        scratch[out++] = values[left++];
    }
    while (right < hi) {
        scratch[out++] = values[right++];
    }
    std::copy(scratch.begin() + static_cast<std::ptrdiff_t>(lo),
              scratch.begin() + static_cast<std::ptrdiff_t>(hi),
              values.begin() + static_cast<std::ptrdiff_t>(lo));
}

void merge_sort(std::vector<int>& values, std::vector<int>& scratch, std::size_t lo,
                std::size_t hi) {
    if (hi - lo < 2) {
        return;
    }
    const std::size_t mid = lo + (hi - lo) / 2;
    merge_sort(values, scratch, lo, mid);
    merge_sort(values, scratch, mid, hi);
    merge(values, scratch, lo, mid, hi);
}

}  // namespace

void sort_counted(int n) {
    std::vector<int> values(static_cast<std::size_t>(n));
    // A fixed reverse-with-noise input keeps the comparison count near n log n.
    for (int i = 0; i < n; ++i) {
        values[static_cast<std::size_t>(i)] = (n - i) ^ ((i * 7919) & 3);
    }
    std::vector<int> scratch(values.size());
    merge_sort(values, scratch, 0, values.size());
}

void run_fib(int n) {
    static volatile long long sink;
    sink = fib(n);
}

}  // namespace workload
