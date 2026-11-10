---
title: Lock-Free Stack and the ABA Problem
unit: 13 · Data Synchronization (HW10)
source: 6.172 Lecture 17 Lock-Free Stack, ABA
pdf: materials/6.172/lectures/lec17.pdf
kind: code
minutes: 90
sanitizer: tsan
---

# 1302 · Lock-Free Stack and the ABA Problem

**Based on:** [Synchronization Without Locks](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/fc3b846f6e13e15e4982a92369ca3825_MIT6_172F18_lec17.pdf)

> **Why this comes up in practice:** an allocator's free list and a task pool both look like this: one CAS swaps out the head. It seems like the simplest lock-free structure, but once nodes get reused, you run into ABA: head has the same value as when you read it, but the world has changed.

## Your task

You build a stack out of preallocated nodes. The nodes come from `practice::NodePool` in `_shared/08_node_pool.hpp` (`allocate()` returns an index, or `kNilIndex` once the nodes run out; `release(i)` gives one back; nodes released first are reused first, so a node soon comes back in a different position). Nodes are referred to by **index** rather than by pointer, so that an index and a version counter fit in the same 64-bit word and can be updated together with one CAS.

Declare every shared field with the template parameter `Atomic` (default `std::atomic`). The tests swap in `dsched::Atomic` to enumerate interleavings, and also run TSan with real atomics.

`LockFreeStack<Atomic>(capacity)`: the values are `std::uint64_t`.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Sequential semantics

```cpp
template <template <typename> class Atomic = std::atomic>
class LockFreeStack {
public:
    explicit LockFreeStack(std::size_t capacity);

    bool push(std::uint64_t value);
    std::optional<std::uint64_t> pop();
};
```

- `push(value)`: return whether it succeeded (return `false` when the nodes run out).
- `pop()`: return a `std::optional<std::uint64_t>`, or `std::nullopt` when the stack is empty.

## Stage 2 — Concurrency

Make `head` an `Atomic<std::uint64_t>`: `push` and `pop` both swap the head with a CAS and retry on failure. A node's `next` must be an `Atomic` (another thread may be reading a node that has already been moved). `value` is written at push time and read after a successful pop, so it can be a plain field, but the two sides must establish happens-before.

The tests enumerate a few small scenarios: every value that is pushed must end up popped exactly once (or still be in the stack).

## Stage 3 — ABA

Within the head's 64 bits, the low 32 bits hold the index and the high 32 bits hold a **version counter**, and every successful CAS increments the version.

The tests enumerate this scenario (with a pool of only 2 nodes): thread A starts a `pop` and is preempted after reading the head and `next` but before its CAS; thread B pops both nodes, then pushes a new value, and the reused node happens to be the old head, but its `next` is now different. Without a version counter, A's CAS succeeds and links a node that has already been recycled back into the stack.

You should first run it once with the version counter removed, to see the tests catch the bug with your own eyes.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Besides a version counter, what other ways are there to solve ABA? What are the trade-offs of hazard pointers and epoch-based reclamation?
2. Can a 32-bit version counter really never wrap around? When do you need to worry about it?
3. Why is this stack lock-free but not wait-free? How does the cost of pop retries change under heavy contention?
