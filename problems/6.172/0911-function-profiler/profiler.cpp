#include "solution.hpp"

// Called by the compiler at every entry and exit of an instrumented function.
extern "C" void __cyg_profile_func_enter(void* this_fn, void* call_site) {}
extern "C" void __cyg_profile_func_exit(void* this_fn, void* call_site) {}

namespace profiler {

void start() { PRACTICE_TODO(); }
void stop() { PRACTICE_TODO(); }
std::uint64_t calls(const void* function) { PRACTICE_TODO(); }

std::vector<ReportLine> report() { PRACTICE_TODO(); }

void set_clock(Clock clock) { PRACTICE_TODO(); }
FunctionStats stats(const void* function) { PRACTICE_TODO(); }

practice::ols::Growth estimate_growth(const void* function, void (*run)(int),
                                      std::span<const int> sizes) {
    PRACTICE_TODO();
}

}  // namespace profiler
