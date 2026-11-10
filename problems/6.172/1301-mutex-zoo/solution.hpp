#pragma once

#include <atomic>
#include <functional>
#include <thread>

#include "practice.hpp"

// Stage 1
template <template <typename> class Atomic = std::atomic>
class TtasLock {
public:
    TtasLock() { PRACTICE_TODO(); }

    void lock() { PRACTICE_TODO(); }
    bool try_lock() { PRACTICE_TODO(); }
    void unlock() { PRACTICE_TODO(); }
};

// Stage 2
template <template <typename> class Atomic = std::atomic>
class CompetitiveLock {
public:
    explicit CompetitiveLock(int spin_limit,
                             std::function<void()> yield = [] { std::this_thread::yield(); }) {
        PRACTICE_TODO();
    }

    void lock() { PRACTICE_TODO(); }
    bool try_lock() { PRACTICE_TODO(); }
    void unlock() { PRACTICE_TODO(); }
};

// Stage 3
template <template <typename> class Atomic = std::atomic>
class PetersonLock {
public:
    PetersonLock() { PRACTICE_TODO(); }

    void lock(int self) { PRACTICE_TODO(); }
    void unlock(int self) { PRACTICE_TODO(); }
};
