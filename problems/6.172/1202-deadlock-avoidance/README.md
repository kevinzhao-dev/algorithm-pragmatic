---
title: Deadlock Avoidance (bank transfers, dining philosophers, and release-sort-reacquire)
unit: 12 · Deterministic Execution (HW9)
source: 6.172 Lecture 16 Deadlock; Recitation 9 Problem 1
pdf: materials/6.172/lectures/lec16.pdf
answer: materials/6.172/recitations/r9/solutions.pdf
read: materials/6.172/recitations/r9/problems.pdf
kind: code
minutes: 90
sanitizer: tsan
---

# 1202 · Deadlock Avoidance

**Based on:** [Nondeterministic Parallel Programming](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/e4fe97d2a250b57bb1e330fba8cf5992_MIT6_172F18_lec16.pdf) · **Read first:** [R9: Deterministic Execution Problems](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/edc0cd9509aecd107836c3fbd90bc86c_MIT6_172F18_rec9.pdf)

> **Why this comes up in practice:** database row locks, file-system inode locks, and any transfer-style operation that "takes two locks" all run into deadlock. The most common fix is a global order, but when the set of locks is known only at run time you need release-sort-reacquire, which is also the foundation of software transactional memory.

## Original problem

6.172 Recitation 9 Problem 1 is a true-or-false question about a bank. The bank keeps one balance and one mutex per account, and a transaction names a source account, a destination account, and an amount. Handling a transaction locks the source account's mutex, then the destination account's mutex, moves the amount from one balance to the other, and unlocks the two mutexes in reverse order. A parallel loop (`cilk_for`) handles a whole array of transactions at once.

The claim is that some input to this loop makes it deadlock. Say whether that is true or false; if it is true, explain why and how to fix it, and if it is false, explain why not.

The stages also follow the deadlock part of Lecture 16:

- Deadlock needs three conditions at once: mutual exclusion, nonpreemption, and circular waiting.
- A theorem shows that if the mutexes can be put in a linear order, and a thread that holds a mutex only ever tries to lock a later one, no deadlock can occur.
- The dining philosophers: n philosophers sit around a table with one chopstick between each pair of neighbors, and each needs the chopsticks on both sides of their plate to eat. If they all pick up their left chopstick at the same moment, they all starve.
- Algorithm L is a lock-based transactional-memory scheme that combines a finite ownership array of locks with release-sort-reacquire.

## Your task

Every class in this exercise takes a template parameter `Mutex` (default `std::mutex`). Some tests run TSan stress tests with a real `std::mutex`; others swap in `dsched::Mutex` from `_shared/08_dsched.hpp` and use deterministic scheduling to **enumerate every interleaving**, so any deadlock is detected precisely. Your code only needs to use the `Mutex` type parameter; do not sleep or assume any scheduling order yourself.

You build a deadlock-free version of the recitation's bank, then the dining philosophers, then Lecture 16's Algorithm L. Instead of a written argument, the tests check each one under every interleaving.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Transfers (Recitation 9 Problem 1)

```cpp
template <typename Mutex = std::mutex>
class Bank {
public:
    explicit Bank(std::vector<std::int64_t> balances);

    std::size_t accounts() const;
    std::int64_t balance(std::size_t account) const;
    bool transfer(std::size_t from, std::size_t to, std::int64_t amount);
};
```

`Bank<Mutex>(balances)`: one lock per account.

- `transfer(from, to, amount)`: lock both accounts. If the balance is insufficient, return `false` and change nothing; otherwise move the money and return `true`. When `from == to`, do nothing and return `true` (you must not lock the same lock twice).
- `balance(i)`, `accounts()`: queries (only called when no transfer is running concurrently).

There must be no deadlock under any interleaving: acquire the locks in a global order (always lock the smaller index first).

## Stage 2 — Dining philosophers (Lecture 16)

```cpp
template <typename Mutex = std::mutex>
class DiningTable {
public:
    explicit DiningTable(std::size_t philosophers);

    void eat(std::size_t philosopher, const std::function<void()>& meal);
};
```

`DiningTable<Mutex>(n)`: n chopsticks and n people. `eat(philosopher, meal)`: once you hold both the left and right chopsticks (chopsticks i and (i+1) mod n), call `meal()`, then put them back. It must not deadlock, and neighbors on either side must not eat at the same time.

## Stage 3 — Release-Sort-Reacquire (Lecture 16 Algorithm L)

```cpp
template <typename Mutex = std::mutex>
class LockTable {
public:
    explicit LockTable(std::size_t locks);

    class Transaction {
    public:
        void access(std::size_t lock);
    };

    template <typename Body>
    void run(Body body);
};
```

This is the case where the set of locks is known only at run time. `LockTable<Mutex>(n)`:

- `table.run(body)`: run one transaction. `body` is a callable that can safely run more than once, and it takes a `Transaction&`; `body` may be called several times (restarting partway through), so any code before the last `access` may run more than once: put side effects only after the last `access`. When the transaction ends (or `body` throws an exception), release all of its locks.
- `txn.access(i)`: this transaction needs lock i. The rules follow Algorithm L:

  1. Already holding `i`: do nothing.
  2. Every lock currently held is smaller than `i` (or none is held): acquire `i` directly, blocking.
  3. Otherwise, first `try_lock(i)`; if that succeeds, you have it. If it fails: release every lock larger than `i`, acquire `i` with blocking, take the released locks back in order, and then **restart the transaction** (`body` runs again from the start, but the locks already held are kept).

Every restart holds more locks than the one before, so the number of restarts is bounded.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. What are the four Coffman conditions for deadlock (Lecture 16 groups them as three, folding hold-and-wait into nonpreemption)? Which one does your transfer code break, and which one do your philosophers break?
2. Why does step 3 of Algorithm L have to "restart the transaction" instead of simply carrying on once it has the locks back?
3. Recitation 9 Problem 2: why doesn't an n-thread mutex implemented with CAS need Ω(n) space?

---

**Official solution:** [R9 Solutions](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/c6c9289ff27bb530da2bf81dd96b8627_MIT6_172F18_rec9sol.pdf). Read it after you finish.
