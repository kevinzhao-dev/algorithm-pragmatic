#include "solution.hpp"

// Called by the compiler at every entry and exit of an instrumented function.
extern "C" void __cyg_profile_func_enter(void* this_fn, void* call_site) {}
extern "C" void __cyg_profile_func_exit(void* this_fn, void* call_site) {}

RunCounts count_run(int n, int base) { PRACTICE_TODO(); }

Sample sample_virtual(int n, int base) { PRACTICE_TODO(); }
CostModel fit_cost_model(std::span<const Sample> samples) { PRACTICE_TODO(); }
double counts_correlation(std::span<const Sample> samples) { PRACTICE_TODO(); }

std::vector<Input> design_inputs() { PRACTICE_TODO(); }

double time_fib_ns(int n, int base, int repeats) { PRACTICE_TODO(); }
CostModel measure_real_cost(std::span<const Input> inputs, int repeats) { PRACTICE_TODO(); }
