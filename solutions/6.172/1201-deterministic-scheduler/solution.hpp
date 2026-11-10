#pragma once

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

struct RunOutcome {
    std::vector<int> schedule;  // id of the thread chosen at every point with a real choice
    bool deadlocked = false;
};

namespace detail {

struct Abandon {};

// Given the runnable ids (ascending, at least two), returns the id to run next.
using Chooser = std::function<int(const std::vector<int>&)>;

class Scheduler {
public:
    explicit Scheduler(Chooser chooser) : chooser_(std::move(chooser)) {}

    RunOutcome run(const std::vector<std::function<void()>>& bodies) {
        const int count = static_cast<int>(bodies.size());
        states_.assign(bodies.size(), State::Runnable);
        blocked_on_.assign(bodies.size(), nullptr);
        for (int i = 0; i < count; ++i) {
            turns_.push_back(std::make_unique<std::condition_variable>());
        }
        std::vector<std::thread> threads;
        for (int i = 0; i < count; ++i) {
            threads.emplace_back([this, i, &bodies] { thread_main(i, bodies[static_cast<std::size_t>(i)]); });
        }
        {
            std::unique_lock lock(mu_);
            if (count > 0) {
                switch_to(pick());
                done_.wait(lock, [&] { return finished_ == count; });
            }
        }
        for (std::thread& thread : threads) {
            thread.join();
        }
        if (error_) {
            std::rethrow_exception(error_);
        }
        return RunOutcome{std::move(schedule_), deadlocked_};
    }

    static Scheduler* current() { return current_; }
    int self() const { return self_; }

    void yield() {
        std::unique_lock lock(mu_);
        leave_if_abandoned(lock);
        const int next = pick();
        if (next != self_) {
            switch_to(next);
            wait_for_turn(lock);
            leave_if_abandoned(lock);
        }
    }

    void block_on(const void* resource) {
        std::unique_lock lock(mu_);
        leave_if_abandoned(lock);
        states_[index(self_)] = State::Blocked;
        blocked_on_[index(self_)] = resource;
        const int next = pick();
        if (next < 0) {
            abandon_ = true;
            deadlocked_ = true;
            lock.unlock();
            throw Abandon{};
        }
        switch_to(next);
        wait_for_turn(lock);
        leave_if_abandoned(lock);
    }

    void wake(const void* resource) {
        const std::lock_guard lock(mu_);
        for (std::size_t i = 0; i < states_.size(); ++i) {
            if (states_[i] == State::Blocked && blocked_on_[i] == resource) {
                states_[i] = State::Runnable;
                blocked_on_[i] = nullptr;
            }
        }
    }

private:
    enum class State { Runnable, Blocked, Finished };

    static std::size_t index(int id) { return static_cast<std::size_t>(id); }

    void thread_main(int id, const std::function<void()>& body) {
        current_ = this;
        self_ = id;
        bool skip = false;
        {
            std::unique_lock lock(mu_);
            wait_for_turn(lock);
            skip = abandon_;
        }
        try {
            if (!skip) {
                body();
            }
        } catch (const Abandon&) {
        } catch (...) {
            const std::lock_guard lock(mu_);
            if (!error_) {
                error_ = std::current_exception();
            }
        }

        std::unique_lock lock(mu_);
        states_[index(id)] = State::Finished;
        ++finished_;
        if (finished_ == static_cast<int>(states_.size())) {
            done_.notify_one();
            return;
        }
        if (abandon_) {
            switch_to(next_unfinished());
            return;
        }
        const int next = pick();
        if (next >= 0) {
            switch_to(next);
        } else {
            abandon_ = true;
            deadlocked_ = true;
            switch_to(next_unfinished());
        }
    }

    // Returns -1 when nothing can run.
    int pick() {
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
        const int chosen = chooser_(runnable);
        if (std::find(runnable.begin(), runnable.end(), chosen) == runnable.end()) {
            if (!error_) {
                error_ = std::make_exception_ptr(
                    std::invalid_argument("schedule chooses a thread that cannot run"));
            }
            abandon_ = true;
            return next_unfinished();
        }
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

    void switch_to(int id) {
        current_turn_ = id;
        turns_[index(id)]->notify_one();
    }

    void wait_for_turn(std::unique_lock<std::mutex>& lock) {
        turns_[index(self_)]->wait(lock, [&] { return current_turn_ == self_; });
    }

    void leave_if_abandoned(std::unique_lock<std::mutex>& lock) {
        if (abandon_) {
            lock.unlock();
            if (std::uncaught_exceptions() == 0) {
                throw Abandon{};
            }
        }
    }

    Chooser chooser_;
    std::mutex mu_;
    std::condition_variable done_;
    std::vector<std::unique_ptr<std::condition_variable>> turns_;
    std::vector<State> states_;
    std::vector<const void*> blocked_on_;
    std::vector<int> schedule_;
    std::exception_ptr error_;
    int current_turn_ = -1;
    int finished_ = 0;
    bool abandon_ = false;
    bool deadlocked_ = false;

    static inline thread_local Scheduler* current_ = nullptr;
    static inline thread_local int self_ = -1;
};

}  // namespace detail

inline RunOutcome run_schedule(std::vector<std::function<void()>> bodies,
                               const std::vector<int>& choices) {
    std::size_t next = 0;
    detail::Scheduler scheduler([&](const std::vector<int>& runnable) {
        return next < choices.size() ? choices[next++] : runnable.front();
    });
    return scheduler.run(bodies);
}

inline void yield_point() {
    if (detail::Scheduler* scheduler = detail::Scheduler::current()) {
        scheduler->yield();
    }
}

inline RunOutcome run_random(std::vector<std::function<void()>> bodies, std::uint64_t seed) {
    std::mt19937_64 engine(seed);
    detail::Scheduler scheduler([&](const std::vector<int>& runnable) {
        return runnable[std::uniform_int_distribution<std::size_t>(0, runnable.size() - 1)(engine)];
    });
    return scheduler.run(bodies);
}

inline RunOutcome replay_schedule(std::vector<std::function<void()>> bodies,
                                  const std::vector<int>& schedule) {
    return run_schedule(std::move(bodies), schedule);
}

// A cooperative mutex: a blocked thread is parked in the scheduler, so deadlocks are detected.
class SchedMutex {
public:
    SchedMutex() = default;
    SchedMutex(const SchedMutex&) = delete;
    SchedMutex& operator=(const SchedMutex&) = delete;

    void lock() {
        detail::Scheduler* scheduler = detail::Scheduler::current();
        while (held_) {
            scheduler->block_on(this);
        }
        held_ = true;
    }

    bool try_lock() {
        if (held_) {
            return false;
        }
        held_ = true;
        return true;
    }

    void unlock() {
        held_ = false;
        if (detail::Scheduler* scheduler = detail::Scheduler::current()) {
            scheduler->wake(this);
            scheduler->yield();
        }
    }

private:
    bool held_ = false;
};

struct Scenario {
    std::vector<std::function<void()>> bodies;
    std::function<bool()> check;
};

struct ExploreSummary {
    long runs = 0;
    long failures = 0;
    long deadlocks = 0;
    std::vector<int> bad_schedule;  // the first failing schedule, for replay
};

inline ExploreSummary explore_all(const std::function<Scenario()>& make) {
    struct Frame {
        std::size_t choice = 0;
        std::size_t options = 0;
    };
    ExploreSummary summary;
    std::vector<Frame> stack;
    while (true) {
        std::size_t depth = 0;
        Scenario scenario = make();
        detail::Scheduler scheduler([&](const std::vector<int>& runnable) {
            if (depth == stack.size()) {
                stack.push_back({0, runnable.size()});
            }
            return runnable[stack[depth++].choice];
        });
        const RunOutcome outcome = scheduler.run(scenario.bodies);
        ++summary.runs;
        bool bad = false;
        if (outcome.deadlocked) {
            ++summary.deadlocks;
            bad = true;
        } else if (scenario.check && !scenario.check()) {
            ++summary.failures;
            bad = true;
        }
        if (bad && summary.bad_schedule.empty()) {
            summary.bad_schedule = outcome.schedule;
        }
        stack.resize(depth);
        while (!stack.empty() && stack.back().choice + 1 == stack.back().options) {
            stack.pop_back();
        }
        if (stack.empty()) {
            return summary;
        }
        ++stack.back().choice;
    }
}
