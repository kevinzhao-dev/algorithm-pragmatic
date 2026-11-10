// Test runner. Normally driven by ./practice, but works by hand too:
//     ./run                 run everything that fits this build
//     ./run --stage 2       only stage 2
//     ./run --verbose       also list passing tests
#include <cstring>
#include <exception>
#include <iostream>
#include <map>
#include <string>

#include "practice.hpp"

namespace {

using practice::TestCase;
using practice::detail::registry;
using practice::detail::run_state;

#ifdef NDEBUG
constexpr bool kOptimizedBuild = true;
#else
constexpr bool kOptimizedBuild = false;
#endif

enum class Status { Pass, Fail, Todo };

struct Options {
    int only_stage = 0;
    bool skip_perf = false;
    bool perf_only = false;
    bool machine = false;  // emit @@ lines for ./practice
    bool verbose = false;
};

struct Outcome {
    Status status = Status::Pass;
    double elapsed_ms = 0;
    std::string details;
};

Outcome run_one(const TestCase& test) {
    auto& state = run_state();
    state.failed_checks = 0;
    state.details.clear();

    Outcome outcome;
    const practice::Stopwatch watch;
    try {
        test.body();
    } catch (const practice::NotImplemented& todo) {
        outcome.status = Status::Todo;
        state.details += std::string("    not implemented yet: ") + todo.what() + "\n";
    } catch (const practice::detail::AbortTest&) {
        // The failing REQUIRE has already been recorded.
    } catch (const std::exception& error) {
        outcome.status = Status::Fail;
        state.details += std::string("    uncaught exception: ") + error.what() + "\n";
    } catch (...) {
        outcome.status = Status::Fail;
        state.details += "    uncaught exception of unknown type\n";
    }
    outcome.elapsed_ms = watch.elapsed_ms();

    if (state.failed_checks.load() > 0 && outcome.status != Status::Todo) {
        outcome.status = Status::Fail;
    }
    if (test.budget_ms > 0 && outcome.status == Status::Pass &&
        outcome.elapsed_ms > static_cast<double>(test.budget_ms)) {
        outcome.status = Status::Fail;
        state.details += "    too slow: " + std::to_string(outcome.elapsed_ms) +
                         " ms exceeds the budget of " + std::to_string(test.budget_ms) + " ms\n";
    }
    outcome.details = state.details;
    return outcome;
}

const char* label(Status status) {
    switch (status) {
        case Status::Pass:
            return "pass";
        case Status::Fail:
            return "fail";
        case Status::Todo:
            return "todo";
    }
    return "?";
}

bool should_run(const TestCase& test, const Options& options) {
    const bool is_perf = test.budget_ms > 0;
    if (options.only_stage != 0 && test.stage != options.only_stage) {
        return false;
    }
    if (is_perf && options.skip_perf) {
        return false;
    }
    if (!is_perf && options.perf_only) {
        return false;
    }
    // Timing budgets mean nothing in a sanitizer build.
    return !is_perf || kOptimizedBuild;
}

}  // namespace

int main(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--stage" && i + 1 < argc) {
            options.only_stage = std::stoi(argv[++i]);
        } else if (arg == "--skip-perf") {
            options.skip_perf = true;
        } else if (arg == "--perf-only") {
            options.perf_only = true;
        } else if (arg == "--machine") {
            options.machine = true;
        } else if (arg == "--verbose") {
            options.verbose = true;
        } else {
            std::cerr << "unknown option: " << arg << '\n';
            return 2;
        }
    }

    auto tests = registry();
    std::stable_sort(tests.begin(), tests.end(),
                     [](const TestCase& a, const TestCase& b) { return a.stage < b.stage; });

    std::map<int, std::pair<int, int>> per_stage;  // stage -> (passed, total)
    int ran = 0;
    int not_passed = 0;
    for (const TestCase& test : tests) {
        if (!should_run(test, options)) {
            continue;
        }
        ++ran;
        const char* kind = test.budget_ms > 0 ? "perf" : "func";
        if (options.machine) {
            std::cout << "@@start " << test.stage << ' ' << kind << ' ' << test.name << '\n'
                      << std::flush;
        }
        const Outcome outcome = run_one(test);
        auto& [passed, total] = per_stage[test.stage];
        ++total;
        if (outcome.status == Status::Pass) {
            ++passed;
        } else {
            ++not_passed;
        }
        if (options.machine) {
            std::cout << outcome.details << "@@result " << test.stage << ' ' << kind << ' '
                      << label(outcome.status) << ' ' << static_cast<long>(outcome.elapsed_ms)
                      << ' ' << test.name << '\n'
                      << std::flush;
        } else if (outcome.status != Status::Pass || options.verbose) {
            std::cout << '[' << label(outcome.status) << "] stage " << test.stage << ": "
                      << test.name << " (" << static_cast<long>(outcome.elapsed_ms) << " ms)\n"
                      << outcome.details;
        }
    }

    if (!options.machine) {
        for (const auto& [stage, counts] : per_stage) {
            std::cout << "stage " << stage << ": " << counts.first << '/' << counts.second
                      << " passed\n";
        }
        if (ran == 0) {
            std::cout << "no tests selected for this build\n";
        }
    }
    return not_passed == 0 ? 0 : 1;
}
