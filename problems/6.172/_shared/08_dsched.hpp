// SPOILER: a complete deterministic scheduler, which answers 6.172/1201. Finish 1201 first.
//
// 08_dsched.hpp — deterministic scheduling of real threads, for reproducible concurrency tests.
//
// Each logical thread is a std::thread, but exactly one runs at a time, and control changes
// hands only at scheduling points: the operations of dsched::Atomic and dsched::Mutex below.
// Because every decision goes through a chooser, a run is a pure function of its schedule, so
// a bug can be found by search (explore_dfs, explore_random) and replayed exactly (replay).
//
// Code under test is written against template parameters so the tests can swap in these types:
//     template <template <typename> class Atomic = std::atomic, typename Mutex = std::mutex>
//     class Table { Atomic<std::uint64_t> head_; Mutex lock_; ... };
//     Table<dsched::Atomic, dsched::Mutex> table;
//
// Exploration only covers sequentially consistent interleavings: it finds logic races, not
// weak-memory bugs. A missing release/acquire pair is what the TSan stress tests are for.
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace practice::dsched {

// Thrown at scheduling points to unwind a run that is being abandoned (deadlock, step limit).
struct AbandonRun {};

struct Outcome {
    std::vector<int> schedule;  // the logical thread chosen at every point with a real choice
    bool deadlocked = false;
    bool truncated = false;  // hit the step limit, which usually means a livelock
};

class Scheduler {
public:
    // Given the runnable thread ids (ascending, at least two) and the id of the thread that was
    // running (-1 before the first step), returns the id to run next.
    using Chooser = std::function<int(const std::vector<int>&, int)>;

    Scheduler(Chooser chooser, long max_steps) : chooser_(std::move(chooser)), max_steps_(max_steps) {}

    Outcome run(const std::vector<std::function<void()>>& bodies) {
        const int count = static_cast<int>(bodies.size());
        states_.assign(bodies.size(), State::Runnable);
        waiting_on_.assign(bodies.size(), nullptr);
        first_step_.assign(bodies.size(), 1);
        turns_.clear();
        for (int i = 0; i < count; ++i) {
            turns_.push_back(std::make_unique<std::condition_variable>());
        }

        std::vector<std::thread> threads;
        threads.reserve(bodies.size());
        for (int i = 0; i < count; ++i) {
            threads.emplace_back([this, i, &bodies] { thread_main(i, bodies[static_cast<std::size_t>(i)]); });
        }
        {
            std::unique_lock lock(mu_);
            if (count > 0) {
                activate(lock, pick(lock, -1));
                all_done_.wait(lock, [&] { return finished_ == count; });
            }
        }
        for (std::thread& thread : threads) {
            thread.join();
        }
        if (error_) {
            std::rethrow_exception(error_);
        }
        return Outcome{std::move(schedule_), deadlocked_, truncated_};
    }

    static Scheduler* active() { return active_; }
    int self() const { return self_; }

    // A scheduling point: the calling thread may lose the processor here.
    void point() {
        std::unique_lock lock(mu_);
        if (abandon_) {
            leave_if_abandoned(lock);
            return;
        }
        if (++steps_ > max_steps_) {
            truncated_ = true;
            abandon_ = true;
            lock.unlock();
            throw_unless_unwinding();
            return;
        }
        // Being scheduled was already a decision; a second one before any operation adds nothing.
        if (std::exchange(first_step_[static_cast<std::size_t>(self_)], 0) != 0) {
            return;
        }
        const int next = pick(lock, self_);
        if (next != self_) {
            activate(lock, next);
            wait_for_turn(lock, self_);
            leave_if_abandoned(lock);
        }
    }

    // Blocks the calling thread until wake(resource); a real wait only other threads can end.
    void block_on(const void* resource) {
        std::unique_lock lock(mu_);
        if (abandon_) {
            leave_if_abandoned(lock);
            return;
        }
        const auto index = static_cast<std::size_t>(self_);
        states_[index] = State::Blocked;
        waiting_on_[index] = resource;
        const int next = pick(lock, self_);
        if (next >= 0) {
            activate(lock, next);
            wait_for_turn(lock, self_);
        } else {
            abandon_after_deadlock();
        }
        leave_if_abandoned(lock);
    }

    void wake(const void* resource) {
        const std::lock_guard lock(mu_);
        for (std::size_t i = 0; i < states_.size(); ++i) {
            if (states_[i] == State::Blocked && waiting_on_[i] == resource) {
                states_[i] = State::Runnable;
                waiting_on_[i] = nullptr;
            }
        }
    }

private:
    enum class State { Runnable, Blocked, Finished };

    void thread_main(int id, const std::function<void()>& body) {
        active_ = this;
        self_ = id;
        bool skip = false;
        {
            std::unique_lock lock(mu_);
            wait_for_turn(lock, id);
            skip = abandon_;
        }
        try {
            if (!skip) {
                body();
            }
        } catch (const AbandonRun&) {
        } catch (...) {
            const std::lock_guard lock(mu_);
            if (!error_) {
                error_ = std::current_exception();
            }
        }

        std::unique_lock lock(mu_);
        states_[static_cast<std::size_t>(id)] = State::Finished;
        ++finished_;
        if (finished_ == static_cast<int>(states_.size())) {
            all_done_.notify_one();
            return;
        }
        const int next = pick(lock, id);
        if (next >= 0) {
            activate(lock, next);
        } else {
            abandon_after_deadlock();
            activate(lock, next_unfinished());
        }
    }

    // Runnable candidates, then the choice: -1 when nothing can run.
    int pick(std::unique_lock<std::mutex>&, int running) {
        if (abandon_) {
            return next_unfinished();
        }
        std::vector<int> runnable;
        for (std::size_t i = 0; i < states_.size(); ++i) {
            if (states_[i] == State::Runnable) {
                runnable.push_back(static_cast<int>(i));
            }
        }
        if (runnable.empty()) {
            return -1;
        }
        if (runnable.size() == 1) {
            return runnable.front();
        }
        const int chosen = chooser_(runnable, running);
        schedule_.push_back(chosen);
        return chosen;
    }

    int next_unfinished() const {
        for (std::size_t i = 0; i < states_.size(); ++i) {
            if (states_[i] != State::Finished) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    void activate(std::unique_lock<std::mutex>&, int id) {
        current_ = id;
        turns_[static_cast<std::size_t>(id)]->notify_one();
    }

    void wait_for_turn(std::unique_lock<std::mutex>& lock, int id) {
        turns_[static_cast<std::size_t>(id)]->wait(lock, [&] { return current_ == id; });
    }

    void abandon_after_deadlock() {
        deadlocked_ = true;
        abandon_ = true;
    }

    static void throw_unless_unwinding() {
        if (std::uncaught_exceptions() == 0) {
            throw AbandonRun{};
        }
    }

    void leave_if_abandoned(std::unique_lock<std::mutex>& lock) {
        if (abandon_) {
            lock.unlock();
            throw_unless_unwinding();
        }
    }

    Chooser chooser_;
    long max_steps_;
    std::mutex mu_;
    std::condition_variable all_done_;
    std::vector<std::unique_ptr<std::condition_variable>> turns_;
    std::vector<State> states_;
    std::vector<const void*> waiting_on_;
    std::vector<char> first_step_;
    std::vector<int> schedule_;
    std::exception_ptr error_;
    long steps_ = 0;
    int current_ = -1;
    int finished_ = 0;
    bool abandon_ = false;
    bool deadlocked_ = false;
    bool truncated_ = false;

    static inline thread_local Scheduler* active_ = nullptr;
    static inline thread_local int self_ = -1;
};

// A scheduling point the test can insert by hand.
inline void yield() {
    if (Scheduler* scheduler = Scheduler::active()) {
        scheduler->point();
    }
}

// A drop-in std::atomic: every operation is preceded by a scheduling point. Outside a run it
// behaves like the plain atomic, so test setup and final checks can use it freely.
template <typename T>
class Atomic {
public:
    constexpr Atomic() noexcept : value_{} {}
    constexpr Atomic(T value) noexcept : value_(value) {}  // NOLINT: mirrors std::atomic
    Atomic(const Atomic&) = delete;
    Atomic& operator=(const Atomic&) = delete;

    T load(std::memory_order order = std::memory_order_seq_cst) const {
        yield();
        return value_.load(order);
    }
    void store(T value, std::memory_order order = std::memory_order_seq_cst) {
        yield();
        value_.store(value, order);
    }
    T exchange(T value, std::memory_order order = std::memory_order_seq_cst) {
        yield();
        return value_.exchange(value, order);
    }
    bool compare_exchange_strong(T& expected, T desired,
                                 std::memory_order order = std::memory_order_seq_cst) {
        yield();
        return value_.compare_exchange_strong(expected, desired, order);
    }
    bool compare_exchange_strong(T& expected, T desired, std::memory_order success,
                                 std::memory_order failure) {
        yield();
        return value_.compare_exchange_strong(expected, desired, success, failure);
    }
    // No spurious failures here, so a loop around weak CAS explores like one around strong CAS.
    bool compare_exchange_weak(T& expected, T desired,
                               std::memory_order order = std::memory_order_seq_cst) {
        return compare_exchange_strong(expected, desired, order);
    }
    bool compare_exchange_weak(T& expected, T desired, std::memory_order success,
                               std::memory_order failure) {
        return compare_exchange_strong(expected, desired, success, failure);
    }
    T fetch_add(T delta, std::memory_order order = std::memory_order_seq_cst)
        requires std::is_integral_v<T>
    {
        yield();
        return value_.fetch_add(delta, order);
    }
    T fetch_sub(T delta, std::memory_order order = std::memory_order_seq_cst)
        requires std::is_integral_v<T>
    {
        yield();
        return value_.fetch_sub(delta, order);
    }
    T fetch_or(T bits, std::memory_order order = std::memory_order_seq_cst)
        requires std::is_integral_v<T>
    {
        yield();
        return value_.fetch_or(bits, order);
    }
    T fetch_and(T bits, std::memory_order order = std::memory_order_seq_cst)
        requires std::is_integral_v<T>
    {
        yield();
        return value_.fetch_and(bits, order);
    }
    operator T() const { return load(); }  // NOLINT: mirrors std::atomic
    T operator=(T value) {
        store(value);
        return value;
    }

private:
    std::atomic<T> value_;
};

// A cooperative mutex: blocking parks the logical thread, so a deadlock is detected exactly
// instead of hanging the process. Usable with std::lock_guard, unique_lock and scoped_lock.
class Mutex {
public:
    Mutex() = default;
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    void lock() {
        Scheduler* scheduler = Scheduler::active();
        if (scheduler == nullptr) {
            held_ = true;
            return;
        }
        scheduler->point();
        while (held_) {
            scheduler->block_on(this);
        }
        held_ = true;
    }

    bool try_lock() {
        yield();
        if (held_) {
            return false;
        }
        held_ = true;
        return true;
    }

    void unlock() {
        Scheduler* scheduler = Scheduler::active();
        if (scheduler != nullptr) {
            scheduler->point();
            held_ = false;
            scheduler->wake(this);
        } else {
            held_ = false;
        }
    }

private:
    bool held_ = false;  // protected by the baton: only the running logical thread touches it
};

// One program to explore. `check` runs on the test thread after every completed run.
struct Scenario {
    std::vector<std::function<void()>> threads;
    std::function<bool()> check;
};

struct Options {
    long max_runs = 200'000;
    int max_preemptions = -1;  // bound for explore_dfs; negative means unbounded
    long max_steps = 50'000;   // per run
};

struct Report {
    long runs = 0;
    long deadlocks = 0;
    long truncated = 0;
    long failures = 0;
    bool exhausted = false;      // explore_dfs walked every schedule within the bounds
    std::vector<int> bad_trace;  // schedule of the first bad run, for replay
    bool clean() const { return deadlocks == 0 && failures == 0 && truncated == 0; }
};

namespace detail {

template <typename Make>
Outcome run_scenario(Make& make, Scheduler::Chooser chooser, long max_steps, Report& report) {
    Scenario scenario = make();
    Scheduler scheduler(std::move(chooser), max_steps);
    Outcome outcome = scheduler.run(scenario.threads);
    ++report.runs;
    bool bad = false;
    if (outcome.deadlocked) {
        ++report.deadlocks;
        bad = true;
    } else if (outcome.truncated) {
        ++report.truncated;
        bad = true;
    } else if (scenario.check && !scenario.check()) {
        ++report.failures;
        bad = true;
    }
    if (bad && report.bad_trace.empty()) {
        report.bad_trace = outcome.schedule;
    }
    return outcome;
}

}  // namespace detail

// Runs `count` schedules, each drawn from its own seed. Finite spin loops terminate with
// probability 1 under random scheduling, so this is the explorer to use for spin locks.
template <typename Make>
Report explore_random(Make make, long count, std::uint64_t seed = 42, Options options = {}) {
    Report report;
    for (long i = 0; i < count && report.runs < options.max_runs; ++i) {
        std::mt19937_64 engine(seed + static_cast<std::uint64_t>(i));
        detail::run_scenario(
            make,
            [&engine](const std::vector<int>& runnable, int) {
                return runnable[std::uniform_int_distribution<std::size_t>(0, runnable.size() - 1)(engine)];
            },
            options.max_steps, report);
    }
    return report;
}

// Depth-first search over schedules: stateless model checking. Every point where more than one
// thread may run is a branch; the first option keeps the current thread running. With
// max_preemptions set, only that many preemptive switches are allowed per run.
template <typename Make>
Report explore_dfs(Make make, Options options = {}) {
    struct Frame {
        std::size_t choice = 0;
        std::size_t options = 0;
    };
    Report report;
    std::vector<Frame> stack;
    while (report.runs < options.max_runs) {
        std::size_t depth = 0;
        int preemptions = 0;
        detail::run_scenario(
            make,
            [&](const std::vector<int>& runnable, int running) {
                std::vector<int> candidates;
                const bool can_continue =
                    std::find(runnable.begin(), runnable.end(), running) != runnable.end();
                if (can_continue) {
                    candidates.push_back(running);
                }
                const bool budget = options.max_preemptions < 0 || preemptions < options.max_preemptions;
                if (!can_continue || budget) {
                    for (const int id : runnable) {
                        if (id != running) {
                            candidates.push_back(id);
                        }
                    }
                }
                std::size_t choice = 0;
                if (candidates.size() > 1) {
                    if (depth == stack.size()) {
                        stack.push_back({0, candidates.size()});
                    }
                    choice = stack[depth].choice;
                    ++depth;
                }
                const int next = candidates[choice];
                if (can_continue && next != running) {
                    ++preemptions;
                }
                return next;
            },
            options.max_steps, report);

        stack.resize(depth);
        while (!stack.empty() && stack.back().choice + 1 == stack.back().options) {
            stack.pop_back();
        }
        if (stack.empty()) {
            report.exhausted = true;
            break;
        }
        ++stack.back().choice;
    }
    return report;
}

// Re-runs a recorded schedule exactly.
template <typename Make>
Outcome replay(Make make, const std::vector<int>& trace, Options options = {}) {
    Report report;
    std::size_t next = 0;
    return detail::run_scenario(
        make,
        [&](const std::vector<int>& runnable, int) {
            return next < trace.size() ? trace[next++] : runnable.front();
        },
        options.max_steps, report);
}

}  // namespace practice::dsched
