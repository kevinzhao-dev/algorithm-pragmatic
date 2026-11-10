#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <set>
#include <utility>
#include <vector>

template <typename Mutex = std::mutex>
class Bank {
public:
    explicit Bank(std::vector<std::int64_t> balances)
        : balances_(std::move(balances)), locks_(balances_.size()) {}

    std::size_t accounts() const { return balances_.size(); }
    std::int64_t balance(std::size_t account) const { return balances_[account]; }

    bool transfer(std::size_t from, std::size_t to, std::int64_t amount) {
        if (from == to) {
            return true;
        }
        const std::lock_guard first(locks_[std::min(from, to)]);
        const std::lock_guard second(locks_[std::max(from, to)]);
        if (balances_[from] < amount) {
            return false;
        }
        balances_[from] -= amount;
        balances_[to] += amount;
        return true;
    }

private:
    std::vector<std::int64_t> balances_;
    std::vector<Mutex> locks_;
};

template <typename Mutex = std::mutex>
class DiningTable {
public:
    explicit DiningTable(std::size_t philosophers) : forks_(philosophers) {}

    void eat(std::size_t philosopher, const std::function<void()>& meal) {
        const std::size_t left = philosopher;
        const std::size_t right = (philosopher + 1) % forks_.size();
        const std::lock_guard first(forks_[std::min(left, right)]);
        const std::lock_guard second(forks_[std::max(left, right)]);
        meal();
    }

private:
    std::vector<Mutex> forks_;
};

template <typename Mutex = std::mutex>
class LockTable {
    struct Restart {};

public:
    explicit LockTable(std::size_t locks) : locks_(locks) {}

    class Transaction {
    public:
        void access(std::size_t index) {
            if (held_.contains(index)) {
                return;
            }
            const std::vector<std::size_t> larger(held_.upper_bound(index), held_.end());
            held_.insert(index);
            if (larger.empty()) {
                table_.locks_[index].lock();
                return;
            }
            if (table_.locks_[index].try_lock()) {
                return;
            }
            for (const std::size_t lock : larger) {
                table_.locks_[lock].unlock();
            }
            table_.locks_[index].lock();
            for (const std::size_t lock : larger) {
                table_.locks_[lock].lock();
            }
            throw Restart{};
        }

    private:
        friend class LockTable;
        explicit Transaction(LockTable& table) : table_(table) {}
        void release_all() {
            for (const std::size_t lock : held_) {
                table_.locks_[lock].unlock();
            }
            held_.clear();
        }

        LockTable& table_;
        std::set<std::size_t> held_;
    };

    template <typename Body>
    void run(Body body) {
        Transaction transaction(*this);
        struct Release {
            Transaction& txn;
            ~Release() { txn.release_all(); }
        } release{transaction};
        while (true) {
            try {
                body(transaction);
                return;
            } catch (const Restart&) {
                // held locks are kept; the next attempt makes progress
            }
        }
    }

private:
    std::vector<Mutex> locks_;
};
