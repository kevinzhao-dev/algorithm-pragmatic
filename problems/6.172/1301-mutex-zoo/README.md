---
title: Building Mutexes from Atomics (TTAS, competitive, and Peterson)
unit: 13 · Data Synchronization (HW10)
source: 6.172 Lecture 16 Properties of Mutexes; Lecture 17 Peterson's Algorithm, Mutex Using CAS
pdf: materials/6.172/lectures/lec17.pdf
read: materials/6.172/lectures/lec16.pdf
kind: code
minutes: 80
sanitizer: tsan
---

# 1301 · Building Mutexes from Atomics

**Based on:** [Synchronization Without Locks](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/fc3b846f6e13e15e4982a92369ca3825_MIT6_172F18_lec17.pdf) · **Read first:** [Nondeterministic Parallel Programming](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/e4fe97d2a250b57bb1e330fba8cf5992_MIT6_172F18_lec16.pdf)

> **Why this comes up in practice:** most of the time you just use `std::mutex`, but to understand why a spinlock collapses under heavy contention, and why a futex spins for a while before it sleeps, you have to build one yourself at least once. Peterson's algorithm is the smallest example for understanding memory ordering: loads and stores alone are enough for mutual exclusion, provided memory is sequentially consistent.

## Your task

Declare every shared field with the template parameter `Atomic` (default `std::atomic`). The tests use `dsched::Atomic` from `_shared/08_dsched.hpp` to explore schedules at random, and they also run TSan with a real `std::atomic` plus a **non-atomic counter**: if your lock doesn't establish happens-before, TSan reports a data race. So you can't make every memory order relaxed.

You build three locks out of atomics alone: a test-and-test-and-set spinlock, the competitive spin-then-yield mutex from Lecture 16, and Peterson's two-thread lock from Lecture 17.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Test-and-Test-and-Set

```cpp
template <template <typename> class Atomic = std::atomic>
class TtasLock {
public:
    TtasLock();

    void lock();
    bool try_lock();
    void unlock();
};
```

`TtasLock<Atomic>`:

- `lock()`: first wait with loads until the lock looks free, and only then try to grab it with an exchange (this causes far less cache-line contention than exchanging over and over).
- `try_lock()`: try once; return `true` on success.
- `unlock()`: release the lock.

## Stage 2 — Competitive mutex (spin, then yield)

```cpp
template <template <typename> class Atomic = std::atomic>
class CompetitiveLock {
public:
    explicit CompetitiveLock(int spin_limit,
                             std::function<void()> yield = [] { std::this_thread::yield(); });

    void lock();
    bool try_lock();
    void unlock();
};
```

`CompetitiveLock<Atomic>(spin_limit, yield)`: `yield` is a `std::function<void()>` and defaults to `std::this_thread::yield`.

`lock()`: when the lock is available, take it right away without calling `yield`. When you can't get it, call `yield()` once after `spin_limit` consecutive failures, then start a new round of counting. `spin_limit == 0` means yielding on the first failure.

Lecture 16's analysis: as long as you spin for about as long as one context switch takes, the total cost is at most 2 times that of the optimal strategy.

## Stage 3 — Peterson's algorithm

```cpp
template <template <typename> class Atomic = std::atomic>
class PetersonLock {
public:
    PetersonLock();

    void lock(int self);
    void unlock(int self);
};
```

`PetersonLock<Atomic>`: supports only two threads, through `lock(self)` / `unlock(self)` (`self` is 0 or 1). Follow Lecture 17: to enter, first raise your flag to show that you want in, then give priority to the other thread, and wait only while the other thread also wants in and it is the other thread's turn.

Use only loads and stores; you must not use exchange or CAS. Use `seq_cst` for every memory order.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does Peterson's algorithm break on x86 without a fence? Which reordering (the only kind that TSO allows) causes it? (Lecture 17)
2. Compare the cache-coherence traffic of test-and-set and test-and-test-and-set when 16 cores contend for the lock.
3. Why does almost nobody use Peterson's algorithm in practice? What hardware guarantees does it need, and what does extending it to n threads cost? Are the memory-fence statements in Recitation 9 Problems 5 and 6 each true or false?
