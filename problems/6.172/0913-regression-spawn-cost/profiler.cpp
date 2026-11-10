#include "solution.hpp"

// Called by the compiler at every entry and exit of an instrumented function.
extern "C" void __cyg_profile_func_enter(void* this_fn, void* call_site) {}
extern "C" void __cyg_profile_func_exit(void* this_fn, void* call_site) {}

RunCounts count_run(int n, int base) { PRACTICE_TODO(); }

Sample sample_virtual(int n, int base) { PRACTICE_TODO(); }
CostModel fit_spawn_cost(std::span<const Sample> samples) { PRACTICE_TODO(); }

double time_fib_spawn_ns(int n, int base, int repeats) { PRACTICE_TODO(); }
std::vector<Input> design_inputs() { PRACTICE_TODO(); }
double measure_spawn_cost(std::span<const Input> inputs, int repeats) { PRACTICE_TODO(); }
double call_cost_ns() { PRACTICE_TODO(); }
