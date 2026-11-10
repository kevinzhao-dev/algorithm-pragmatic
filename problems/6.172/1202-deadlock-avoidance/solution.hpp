#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

#include "practice.hpp"

// Stage 1
template <typename Mutex = std::mutex>
class Bank {
public:
    explicit Bank(std::vector<std::int64_t> balances) { PRACTICE_TODO(); }

    std::size_t accounts() const { PRACTICE_TODO(); }
    std::int64_t balance(std::size_t account) const { PRACTICE_TODO(); }
    bool transfer(std::size_t from, std::size_t to, std::int64_t amount) { PRACTICE_TODO(); }
};

// Stage 2
template <typename Mutex = std::mutex>
class DiningTable {
public:
    explicit DiningTable(std::size_t philosophers) { PRACTICE_TODO(); }

    void eat(std::size_t philosopher, const std::function<void()>& meal) { PRACTICE_TODO(); }
};

// Stage 3
template <typename Mutex = std::mutex>
class LockTable {
public:
    explicit LockTable(std::size_t locks) { PRACTICE_TODO(); }

    class Transaction {
    public:
        void access(std::size_t lock) { PRACTICE_TODO(); }
    };

    template <typename Body>
    void run(Body body) {
        PRACTICE_TODO();
    }
};
