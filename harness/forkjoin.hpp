// forkjoin.hpp — a small work-stealing fork-join runtime that stands in for Cilk.
//
//     practice::set_workers(8);
//     practice::fork2([&] { left(); }, [&] { right(); });      // cilk_spawn left(); right(); cilk_sync;
//     practice::parallel_for(0, n, [&](std::size_t i) { ... });  // cilk_for
//
// Each worker owns a deque: it pushes and pops at the back, thieves take from the front.
// A forking frame that finds its task stolen helps run other tasks until the thief finishes.
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace practice {

namespace detail {

class ForkJoinRuntime {
public:
    struct Task {
        void (*invoke)(void*) = nullptr;
        void* argument = nullptr;
        std::atomic<bool> finished{false};
        std::exception_ptr error;
    };

    static ForkJoinRuntime& instance() {
        static ForkJoinRuntime runtime;
        return runtime;
    }

    ~ForkJoinRuntime() { stop_threads(); }

    unsigned workers() {
        ensure_started();
        return static_cast<unsigned>(workers_.size());
    }

    void set_workers(unsigned count) {
        stop_threads();
        start_threads(std::max(1U, count));
    }

    // In [0, workers()). The thread that enters from outside the pool acts as worker 0.
    unsigned worker_index() const { return index_ < 0 ? 0U : static_cast<unsigned>(index_); }

    template <typename F, typename G>
    void fork2(F&& f, G&& g) {
        ensure_started();
        const bool outermost = index_ < 0;
        if (outermost) {
            index_ = 0;
        }
        const ScopeExit restore{outermost};
        Worker& self = *workers_[static_cast<std::size_t>(index_)];

        auto second = [&] { g(); };
        Task task;
        task.invoke = [](void* closure) { (*static_cast<decltype(second)*>(closure))(); };
        task.argument = &second;
        self.push(&task);

        std::exception_ptr first_error;
        try {
            f();
        } catch (...) {
            first_error = std::current_exception();
        }

        if (self.pop_if_back(&task)) {
            execute(&task);
        } else {
            help_until_finished(task);
        }

        if (first_error) {
            std::rethrow_exception(first_error);
        }
        if (task.error) {
            std::rethrow_exception(task.error);
        }
    }

private:
    struct Worker {
        std::mutex lock;
        std::deque<Task*> tasks;

        void push(Task* task) {
            const std::lock_guard guard(lock);
            tasks.push_back(task);
        }
        bool pop_if_back(Task* task) {
            const std::lock_guard guard(lock);
            if (!tasks.empty() && tasks.back() == task) {
                tasks.pop_back();
                return true;
            }
            return false;
        }
        Task* steal_front() {
            const std::lock_guard guard(lock);
            if (tasks.empty()) {
                return nullptr;
            }
            Task* task = tasks.front();
            tasks.pop_front();
            return task;
        }
    };

    struct ScopeExit {
        bool active;
        ~ScopeExit() {
            if (active) {
                index_ = -1;
            }
        }
    };

    ForkJoinRuntime() = default;

    static void execute(Task* task) {
        try {
            task->invoke(task->argument);
        } catch (...) {
            task->error = std::current_exception();
        }
        // The task lives on the forking frame's stack; it must not be touched after this store.
        task->finished.store(true, std::memory_order_release);
    }

    Task* find_work() {
        const std::size_t count = workers_.size();
        const std::size_t self = worker_index();
        victim_ = (victim_ + 1) % count;
        for (std::size_t offset = 0; offset < count; ++offset) {
            const std::size_t candidate = (self + victim_ + offset) % count;
            if (Task* task = workers_[candidate]->steal_front()) {
                return task;
            }
        }
        return nullptr;
    }

    static void back_off(int& idle) {
        if (++idle < 64) {
            std::this_thread::yield();
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    }

    void help_until_finished(const Task& task) {
        int idle = 0;
        while (!task.finished.load(std::memory_order_acquire)) {
            if (Task* other = find_work()) {
                execute(other);
                idle = 0;
            } else {
                back_off(idle);
            }
        }
    }

    void worker_loop(int index) {
        index_ = index;
        int idle = 0;
        while (!stopping_.load(std::memory_order_acquire)) {
            if (Task* task = find_work()) {
                execute(task);
                idle = 0;
            } else {
                back_off(idle);
            }
        }
    }

    void ensure_started() {
        if (workers_.empty()) {
            start_threads(std::max(1U, std::thread::hardware_concurrency()));
        }
    }

    void start_threads(unsigned count) {
        stopping_.store(false, std::memory_order_release);
        workers_.clear();
        for (unsigned i = 0; i < count; ++i) {
            workers_.push_back(std::make_unique<Worker>());
        }
        for (unsigned i = 1; i < count; ++i) {
            threads_.emplace_back([this, i] { worker_loop(static_cast<int>(i)); });
        }
    }

    void stop_threads() {
        stopping_.store(true, std::memory_order_release);
        for (std::thread& thread : threads_) {
            thread.join();
        }
        threads_.clear();
        workers_.clear();
    }

    std::vector<std::unique_ptr<Worker>> workers_;
    std::vector<std::thread> threads_;
    std::atomic<bool> stopping_{false};

    static inline thread_local int index_ = -1;
    static inline thread_local std::size_t victim_ = 0;
};

}  // namespace detail

// Call only from outside any parallel region.
inline void set_workers(unsigned count) { detail::ForkJoinRuntime::instance().set_workers(count); }
inline unsigned workers() { return detail::ForkJoinRuntime::instance().workers(); }
inline unsigned worker_index() { return detail::ForkJoinRuntime::instance().worker_index(); }

// Runs f and g, potentially in parallel, and returns when both are done. Exceptions propagate.
template <typename F, typename G>
void fork2(F&& f, G&& g) {
    detail::ForkJoinRuntime::instance().fork2(std::forward<F>(f), std::forward<G>(g));
}

// Calls body(i) for every i in [begin, end). Ranges at or below `grain` run serially.
template <typename Body>
void parallel_for(std::size_t begin, std::size_t end, Body&& body, std::size_t grain = 1) {
    if (end <= begin) {
        return;
    }
    if (end - begin <= std::max<std::size_t>(grain, 1)) {
        for (std::size_t i = begin; i < end; ++i) {
            body(i);
        }
        return;
    }
    const std::size_t middle = begin + (end - begin) / 2;
    fork2([&] { parallel_for(begin, middle, body, grain); },
          [&] { parallel_for(middle, end, body, grain); });
}

}  // namespace practice
