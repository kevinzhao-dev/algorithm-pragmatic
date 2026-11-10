#include <cxxabi.h>
#include <dlfcn.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

#include "solution.hpp"

namespace {

std::uint64_t steady_nanoseconds() {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(
        duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count());
}

struct Record {
    profiler::FunctionStats stats;
    int active = 0;                 // live activations of this function on the stack
    std::uint64_t outermost_at = 0;  // when the outermost activation started
};

struct State {
    bool recording = false;
    profiler::Clock clock = steady_nanoseconds;
    std::uint64_t last_event = 0;
    std::vector<Record*> stack;
    std::unordered_map<const void*, Record> records;
};

State state;

std::string demangle(const char* symbol) {
    int status = 0;
    char* text = abi::__cxa_demangle(symbol, nullptr, nullptr, &status);
    std::string name = status == 0 ? text : symbol;
    std::free(text);
    return name;
}

std::string name_of(const void* function) {
    Dl_info info{};
    if (dladdr(function, &info) != 0 && info.dli_sname != nullptr) {
        return demangle(info.dli_sname);
    }
    return "<unknown>";
}

// Charges the time since the last event to the function on top of the stack.
void charge_exclusive(std::uint64_t now) {
    if (!state.stack.empty()) {
        state.stack.back()->stats.exclusive_ticks += now - state.last_event;
    }
    state.last_event = now;
}

}  // namespace

extern "C" void __cyg_profile_func_enter(void* this_fn, void* /*call_site*/) {
    if (!state.recording) {
        return;
    }
    const std::uint64_t now = state.clock();
    charge_exclusive(now);
    Record& record = state.records[this_fn];
    ++record.stats.calls;
    if (record.active++ == 0) {
        record.outermost_at = now;
    }
    state.stack.push_back(&record);
}

extern "C" void __cyg_profile_func_exit(void* this_fn, void* /*call_site*/) {
    if (!state.recording || state.stack.empty()) {
        return;
    }
    const std::uint64_t now = state.clock();
    charge_exclusive(now);
    Record* record = state.stack.back();
    state.stack.pop_back();
    if (--record->active == 0) {
        record->stats.inclusive_ticks += now - record->outermost_at;
    }
}

namespace profiler {

void start() {
    state.records.clear();
    state.stack.clear();
    state.recording = true;
    state.last_event = state.clock();
}

void stop() { state.recording = false; }

std::uint64_t calls(const void* function) {
    const auto found = state.records.find(function);
    return found == state.records.end() ? 0 : found->second.stats.calls;
}

std::vector<ReportLine> report() {
    std::vector<ReportLine> lines;
    for (const auto& [function, record] : state.records) {
        lines.push_back({name_of(function), record.stats.calls});
    }
    std::sort(lines.begin(), lines.end(),
              [](const ReportLine& a, const ReportLine& b) { return a.name < b.name; });
    return lines;
}

void set_clock(Clock clock) { state.clock = clock != nullptr ? clock : steady_nanoseconds; }

FunctionStats stats(const void* function) {
    const auto found = state.records.find(function);
    return found == state.records.end() ? FunctionStats{} : found->second.stats;
}

practice::ols::Growth estimate_growth(const void* function, void (*run)(int),
                                      std::span<const int> sizes) {
    std::vector<double> ns, counts;
    for (const int n : sizes) {
        start();
        run(n);
        stop();
        ns.push_back(static_cast<double>(n));
        counts.push_back(static_cast<double>(calls(function)));
    }
    return practice::ols::classify_growth(ns, counts);
}

}  // namespace profiler
