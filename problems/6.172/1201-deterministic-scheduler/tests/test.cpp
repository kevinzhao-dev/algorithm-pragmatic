#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using Log = std::vector<std::string>;

namespace {

// Two threads, two segments each, appending to a shared log.
struct TwoSteps {
    std::shared_ptr<Log> log = std::make_shared<Log>();
    std::vector<std::function<void()>> bodies() const {
        auto log_copy = log;
        auto first = [log_copy] {
            log_copy->push_back("a1");
            yield_point();
            log_copy->push_back("a2");
        };
        auto second = [log_copy] {
            log_copy->push_back("b1");
            yield_point();
            log_copy->push_back("b2");
        };
        return {first, second};
    }
};

}  // namespace

TEST(1, "the requested interleaving runs exactly") {
    const TwoSteps program;
    const RunOutcome outcome = run_schedule(program.bodies(), {1, 0, 1});
    CHECK_EQ(*program.log, Log({"b1", "a1", "b2", "a2"}));
    CHECK_EQ(outcome.schedule, std::vector<int>({1, 0, 1}));
    CHECK(!outcome.deadlocked);
}

TEST(1, "after the choices run out the lowest id goes first") {
    const TwoSteps program;
    const RunOutcome outcome = run_schedule(program.bodies(), {});
    CHECK_EQ(*program.log, Log({"a1", "a2", "b1", "b2"}));
    // The first segment and a1's yield are decisions; once thread 0 ends, only 1 remains.
    CHECK_EQ(outcome.schedule, std::vector<int>({0, 0}));
}

TEST(1, "one body makes no decisions at all") {
    auto log = std::make_shared<Log>();
    const RunOutcome outcome = run_schedule(
        {[log] {
             log->push_back("x");
             yield_point();
             log->push_back("y");
         }},
        {});
    CHECK_EQ(*log, Log({"x", "y"}));
    CHECK(outcome.schedule.empty());
}

TEST(1, "choosing a thread that cannot run is rejected") {
    const TwoSteps program;
    CHECK_THROWS_AS(run_schedule(program.bodies(), {5}), std::invalid_argument);
}

TEST(1, "exceptions in a body surface from the run") {
    const auto boom = [] { throw std::runtime_error("from a thread"); };
    CHECK_THROWS_AS(run_schedule({boom}, {}), std::runtime_error);
}

TEST(1, "a lost update appears under the right schedule and only then") {
    const auto run = [](const std::vector<int>& choices) {
        auto x = std::make_shared<int>(0);
        auto body = [x] {
            const int read = *x;
            yield_point();
            *x = read + 1;
        };
        run_schedule({body, body}, choices);
        return *x;
    };
    CHECK_EQ(run({0, 1, 0, 1}), 1);  // both read 0 before either writes
    CHECK_EQ(run({0, 0, 1, 1}), 2);  // one finishes before the other starts
}

TEST(2, "the same seed gives the same schedule") {
    const TwoSteps a, b;
    const RunOutcome first = run_random(a.bodies(), 7);
    const RunOutcome second = run_random(b.bodies(), 7);
    CHECK_EQ(first.schedule, second.schedule);
    CHECK_EQ(*a.log, *b.log);
}

TEST(2, "different seeds explore different interleavings") {
    std::set<std::vector<int>> seen;
    for (std::uint64_t seed = 0; seed < 64; ++seed) {
        const TwoSteps program;
        seen.insert(run_random(program.bodies(), seed).schedule);
    }
    CHECK_GE(seen.size(), 3);
}

TEST(2, "a recorded run replays to the same log") {
    for (std::uint64_t seed = 0; seed < 20; ++seed) {
        INFO("seed ", seed);
        const TwoSteps recorded;
        const RunOutcome original = run_random(recorded.bodies(), seed);
        const TwoSteps again;
        const RunOutcome replayed = replay_schedule(again.bodies(), original.schedule);
        REQUIRE_EQ(*recorded.log, *again.log);
        REQUIRE_EQ(original.schedule, replayed.schedule);
    }
}

TEST(2, "a lost update found by seeds replays exactly") {
    std::uint64_t failing = 1000;
    std::vector<int> schedule;
    for (std::uint64_t seed = 0; seed < 200 && failing == 1000; ++seed) {
        auto x = std::make_shared<int>(0);
        auto body = [x] {
            const int read = *x;
            yield_point();
            *x = read + 1;
        };
        const RunOutcome outcome = run_random({body, body}, seed);
        if (*x == 1) {
            failing = seed;
            schedule = outcome.schedule;
        }
    }
    REQUIRE(failing != 1000);
    auto x = std::make_shared<int>(0);
    auto body = [x] {
        const int read = *x;
        yield_point();
        *x = read + 1;
    };
    replay_schedule({body, body}, schedule);
    CHECK_EQ(*x, 1);
}

TEST(3, "a contended lock serializes the critical section") {
    auto mutex = std::make_shared<SchedMutex>();
    auto x = std::make_shared<int>(0);
    auto body = [mutex, x] {
        const std::lock_guard guard(*mutex);
        const int read = *x;
        yield_point();
        *x = read + 1;
    };
    run_random({body, body}, 3);
    CHECK_EQ(*x, 2);
}

TEST(3, "try_lock on a held mutex fails without blocking") {
    auto mutex = std::make_shared<SchedMutex>();
    auto held = std::make_shared<int>(-1);
    run_schedule(
        {[mutex, held] {
             mutex->lock();
             yield_point();
             mutex->unlock();
         },
         [mutex, held] { *held = mutex->try_lock() ? 1 : 0; }},
        {0, 1});
    CHECK_EQ(*held, 0);
}

TEST(3, "a wait on a free mutex does not lose its turn") {
    auto mutex = std::make_shared<SchedMutex>();
    auto log = std::make_shared<Log>();
    auto body = [mutex, log](const std::string& name) {
        return [mutex, log, name] {
            mutex->lock();
            log->push_back(name);
            mutex->unlock();
        };
    };
    run_random({body("a"), body("b"), body("c")}, 11);
    CHECK_EQ(log->size(), 3);
}

TEST(3, "lock order inversion deadlocks and is reported") {
    struct Locks {
        SchedMutex a, b;
    };
    auto locks = std::make_shared<Locks>();
    auto first = [locks] {
        locks->a.lock();
        yield_point();
        locks->b.lock();
        locks->b.unlock();
        locks->a.unlock();
    };
    auto second = [locks] {
        locks->b.lock();
        yield_point();
        locks->a.lock();
        locks->a.unlock();
        locks->b.unlock();
    };
    // Interleave so each holds one lock and waits for the other.
    const RunOutcome outcome = run_schedule({first, second}, {0, 1, 0, 1});
    CHECK(outcome.deadlocked);
}

TEST(3, "without the inversion the same locks do not deadlock") {
    struct Locks {
        SchedMutex a, b;
    };
    auto locks = std::make_shared<Locks>();
    auto body = [locks] {
        locks->a.lock();
        yield_point();
        locks->b.lock();
        locks->b.unlock();
        locks->a.unlock();
    };
    for (std::uint64_t seed = 0; seed < 50; ++seed) {
        const RunOutcome outcome = run_random({body, body}, seed);
        CHECK(!outcome.deadlocked);
    }
}

TEST(4, "exploration visits every interleaving exactly once") {
    std::set<std::vector<std::string>> traces;
    long check_calls = 0;
    const auto make = [&] {
        auto log = std::make_shared<Log>();
        Scenario scenario;
        scenario.bodies = {[log] {
                               log->push_back("a1");
                               yield_point();
                               log->push_back("a2");
                           },
                           [log] {
                               log->push_back("b1");
                               yield_point();
                               log->push_back("b2");
                           }};
        scenario.check = [log, &traces, &check_calls] {
            traces.insert(*log);
            ++check_calls;
            return true;
        };
        return scenario;
    };
    const ExploreSummary summary = explore_all(make);
    CHECK_EQ(summary.runs, 6);
    CHECK_EQ(check_calls, 6);
    CHECK_EQ(traces.size(), 6);
    CHECK_EQ(summary.failures, 0);
    CHECK_EQ(summary.deadlocks, 0);
}

TEST(4, "three threads produce the multinomial number of schedules") {
    const auto make = [] {
        Scenario scenario;
        const auto body = [] { yield_point(); };
        scenario.bodies = {body, body, body};
        return scenario;
    };
    // Three threads with two segments each interleave in 6! / (2! 2! 2!) = 90 ways.
    CHECK_EQ(explore_all(make).runs, 90);
}

TEST(4, "failures are counted and the first bad schedule is kept") {
    const auto make = [] {
        auto x = std::make_shared<int>(0);
        Scenario scenario;
        auto body = [x] {
            const int read = *x;
            yield_point();
            *x = read + 1;
        };
        scenario.bodies = {body, body};
        scenario.check = [x] { return *x == 2; };
        return scenario;
    };
    const ExploreSummary summary = explore_all(make);
    CHECK_EQ(summary.runs, 6);
    CHECK_EQ(summary.failures, 4);  // only the two serial orders keep both increments
    CHECK(!summary.bad_schedule.empty());

    // The recorded schedule reproduces a failure.
    Scenario again = make();
    replay_schedule(again.bodies, summary.bad_schedule);
    CHECK(!again.check());
}

TEST(4, "deadlocking schedules are counted") {
    const auto make = [] {
        struct Locks {
            SchedMutex a, b;
        };
        auto locks = std::make_shared<Locks>();
        Scenario scenario;
        scenario.bodies = {[locks] {
                               locks->a.lock();
                               yield_point();
                               locks->b.lock();
                               locks->b.unlock();
                               locks->a.unlock();
                           },
                           [locks] {
                               locks->b.lock();
                               yield_point();
                               locks->a.lock();
                               locks->a.unlock();
                               locks->b.unlock();
                           }};
        return scenario;
    };
    const ExploreSummary summary = explore_all(make);
    CHECK_GT(summary.deadlocks, 0);
    CHECK_GT(summary.runs, summary.deadlocks);  // other orderings do not deadlock
}

TEST(4, "the two-core example of Recitation 9 has exactly these outcomes") {
    std::set<std::pair<int, int>> outcomes;
    const auto make = [&] {
        struct State {
            int x = 0;
            int y = 0;
        };
        auto state = std::make_shared<State>();
        Scenario scenario;
        // Every read and every write is a separate step: if (x == 0) y = x + 1;
        scenario.bodies = {[state] {
                               const int a = state->x;
                               yield_point();
                               if (a == 0) {
                                   const int b = state->x;
                                   yield_point();
                                   state->y = b + 1;
                               }
                           },
                           [state] {
                               const int a = state->y;
                               yield_point();
                               if (a == 0) {
                                   const int b = state->y;
                                   yield_point();
                                   state->x = b + 1;
                               }
                           }};
        scenario.check = [state, &outcomes] {
            outcomes.emplace(state->x, state->y);
            return true;
        };
        return scenario;
    };
    explore_all(make);
    const std::set<std::pair<int, int>> expected{{0, 1}, {1, 0}, {1, 1}, {1, 2}, {2, 1}};
    CHECK_EQ(outcomes, expected);
    // The recitation's options B and E never happen.
    CHECK(!outcomes.contains({0, 2}));
    CHECK(!outcomes.contains({2, 2}));
}
