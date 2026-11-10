#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <thread>
#include <utility>

template <template <typename> class Atomic = std::atomic>
class TtasLock {
public:
    TtasLock() = default;

    void lock() {
        while (true) {
            if (!locked_.exchange(true, std::memory_order_acquire)) {
                return;
            }
            while (locked_.load(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
        }
    }

    bool try_lock() { return !locked_.exchange(true, std::memory_order_acquire); }
    void unlock() { locked_.store(false, std::memory_order_release); }

private:
    Atomic<bool> locked_{false};
};

template <template <typename> class Atomic = std::atomic>
class CompetitiveLock {
public:
    explicit CompetitiveLock(int spin_limit,
                             std::function<void()> yield = [] { std::this_thread::yield(); })
        : spin_limit_(spin_limit), yield_(std::move(yield)) {}

    void lock() {
        int failures = 0;
        while (locked_.exchange(true, std::memory_order_acquire)) {
            if (++failures > spin_limit_) {
                yield_();
                failures = 0;
            }
        }
    }

    bool try_lock() { return !locked_.exchange(true, std::memory_order_acquire); }
    void unlock() { locked_.store(false, std::memory_order_release); }

private:
    int spin_limit_;
    std::function<void()> yield_;
    Atomic<bool> locked_{false};
};

template <template <typename> class Atomic = std::atomic>
class PetersonLock {
public:
    PetersonLock() = default;

    void lock(int self) {
        const int other = 1 - self;
        wants_[static_cast<std::size_t>(self)].store(true);
        turn_.store(other);
        while (wants_[static_cast<std::size_t>(other)].load() && turn_.load() == other) {
            std::this_thread::yield();
        }
    }

    void unlock(int self) { wants_[static_cast<std::size_t>(self)].store(false); }

private:
    std::array<Atomic<bool>, 2> wants_{false, false};
    Atomic<int> turn_{0};
};
