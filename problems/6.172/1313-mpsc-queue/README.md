---
title: Single-Consumer Lock-Free Queue (MPSC)
unit: 13 · Data Synchronization (HW10)
source: 6.172 HW10 check-off 8 (single dequeuer)
pdf: materials/6.172/homework/hw10/handout.pdf
read: materials/6.172/lectures/lec17.pdf
kind: code
minutes: 60
sanitizer: tsan
---

# 1313 · Single-Consumer Lock-Free Queue

**Original problem:** [HW10: Data Synchronization](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/64f4db5020ca72ab205e458d40bbdfe3_MIT6_172F18hw10.pdf) · **Read first:** [Synchronization Without Locks](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/fc3b846f6e13e15e4982a92369ca3825_MIT6_172F18_lec17.pdf)

> **Why this comes up in practice:** logging, actor mailboxes, and the input side of an event loop almost always have several producers and one consumer. Drop the requirement that "several consumers fight over the same head", and dequeue can skip the most expensive CAS.

## Original problem

6.172 HW10 (Data Synchronization) is a written homework that compares a lock-based and a lock-free FIFO queue, both given as C-like pseudocode. Exercise 1312 restates the lock-free one, Michael and Scott's queue, whose links are pointers paired with modification counts and are updated with CAS. As in the rest of the homework, assume that CAS works on a whole pointer-and-count pair, that the compiler doesn't reorder instructions, that the node pool never runs out, and that every write is atomic.

- Check-off 8: show how to simplify the lock-free code when only one thread may dequeue. Write the pseudocode with comments, and explain why every possible execution still keeps FIFO order and why the simplified queue is still non-blocking.

## Your task

The homework's check-off 8: if only one thread ever dequeues, how can 1312's lock-free queue be simplified?

Reuse 1312's tagged words (`practice::NodePool` from `_shared/08_node_pool.hpp`, an index plus a version counter). `MpscQueue<Atomic>(capacity)`: only the tail and each node's `next` and value are `Atomic`.

You implement that simplification in C++, and Stage 3 lets the tests confirm that it really saves the CAS.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Sequential semantics

```cpp
template <template <typename> class Atomic = std::atomic>
class MpscQueue {
public:
    explicit MpscQueue(std::size_t capacity);

    bool enqueue(std::uint64_t value);
    std::optional<std::uint64_t> dequeue();  // single consumer thread only
};
```

- `enqueue(value)`: return `false` when the nodes run out.
- `dequeue()`: return `std::nullopt` when the queue is empty. **Only one thread ever calls it**, but it may run at the same time as several `enqueue` calls.

## Stage 2 — Concurrency

`enqueue` is the same as in 1312 (CAS `tail->next`, and help the tail forward when it lags). `dequeue` simplifies to:

- `head` is **the consumer's own field** and doesn't need to be `Atomic`.
- Read `head->next`; if it is empty, return `std::nullopt`.
- No other thread touches the head, so there is **no need to CAS head**.

There is one trap: before freeing the old dummy, make sure `tail` isn't still pointing at it (otherwise an enqueuer may still be using that node); so when the tail lags, the consumer must help push it forward too.

## Stage 3 — A verifiable simplification

In the common case (the tail isn't lagging), `dequeue` needs **not a single CAS**: the tests confirm this with an `Atomic` that counts CASes. With multiple producers, the final result and the FIFO property are checked by enumerating interleavings and by TSan tests with real threads.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. The second question of check-off 8: why is this version still non-blocking? When a producer stalls, what does the consumer see?
2. Why must you make sure the tail isn't lagging before freeing the old dummy? What goes wrong without this step?
3. If there is also only one producer (SPSC), which other CASes can you remove? What does the ring-buffer version look like?
