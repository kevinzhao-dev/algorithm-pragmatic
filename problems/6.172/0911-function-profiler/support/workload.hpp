// The instrumented program under test. Only workload.cpp is compiled with -finstrument-functions.
#pragma once

#include <cstdint>
#include <span>

namespace workload {

// A virtual clock: burn(n) advances it by n, and is deliberately not instrumented so the time
// is charged to its caller.
std::uint64_t now();
void burn(std::uint64_t ticks);
void set_now(std::uint64_t ticks);

void leaf();
long long fib(int n);
bool is_even(int n);
bool is_odd(int n);

// Call shapes for Stage 3.
void outer();                 // burn(10); inner(); burn(5);
void inner();                 // burn(7);
void recurse(int depth);      // burn(2); recurse(depth - 1) while depth > 0; burn(1);
void ping(int n);             // burn(1); pong(n - 1) while n > 0
void pong(int n);             // burn(2); ping(n - 1) while n > 0

// Workloads with known complexity for Stage 4.
void loop_leaves(int n);      // calls leaf() n times
void nested_leaves(int n);    // calls leaf() n * n times
void sort_counted(int n);     // merge sorts n values, comparing through compare()
bool compare(int a, int b);
void run_fib(int n);

}  // namespace workload
