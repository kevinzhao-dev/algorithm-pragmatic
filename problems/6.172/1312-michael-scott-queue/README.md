---
title: Michael-Scott Lock-Free Queue
unit: 13 · Data Synchronization (HW10)
source: 6.172 HW10 check-off 3, 4, 5, 6, 9 (lock-free queue)
pdf: materials/6.172/homework/hw10/handout.pdf
read: materials/6.172/lectures/lec17.pdf
kind: code
minutes: 120
sanitizer: tsan
---

# 1312 · Michael-Scott Lock-Free Queue

**Original problem:** [HW10: Data Synchronization](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/64f4db5020ca72ab205e458d40bbdfe3_MIT6_172F18hw10.pdf) · **Read first:** [Synchronization Without Locks](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/fc3b846f6e13e15e4982a92369ca3825_MIT6_172F18_lec17.pdf)

> **Why this comes up in practice:** the JDK's `ConcurrentLinkedQueue` is this algorithm. It has a trick well worth remembering: tail may lag behind for a while, but any other thread that notices the lag **helps** push it to the right place. That is what lock-free means: no single thread can hold everyone else up.

## Original problem

6.172 HW10 (Data Synchronization) is a written homework that compares two FIFO queues given as C-like pseudocode; this exercise covers the lock-free one, Michael and Scott's queue (Figures 2 and 3). As in the lock-based queue (restated in 1311), nodes come from a pool allocated in advance, and the first node is a dummy that is never dequeued and only marks an empty queue. Here every link (head, tail, and each node's next) is a `pointer_t`: a node pointer paired with a modification count. CAS atomically replaces the contents of an address with a new value if they still equal an expected old value, and reports whether it did; every CAS in the queue also increments the count. Stage 2 below walks through the enqueue and dequeue steps. The handout asks you to assume that CAS works on a whole `pointer_t`, that the compiler doesn't reorder instructions, that the pool never runs out, and that nodes don't cross cache lines, so every write is atomic.

- Check-off 3: annotate every line of the lock-free code with a brief comment of at most 10 words, in the style of the comments on the lock-based code.
- Check-off 4: explain how a new node gets inserted. How many successful CASes does each node need? What happens if the final CAS, the one that swings tail to the new node, fails? How far can tail lag behind? Is the program correct without that CAS?
- Check-off 5, about dequeue:
  - (a) After reading head, tail, and head's next, dequeue checks that head hasn't changed. Why is that check needed?
  - (b) The value is read from the next node before the CAS that moves head. Why does that order matter, and what can happen if the two are swapped?
  - (c) What happens if that CAS on head fails?
- Check-off 6: which implementation do you expect to run faster, the lock-based one or the lock-free one? Justify it by weighing what the synchronization primitives cost, how much contention there is, and the overhead of synchronizing.
- Check-off 9: explain how the count deals with the ABA problem discussed in recitation.

## Your task

Write the algorithm from the homework's Figures 2 and 3 in C++. The nodes come from `practice::NodePool` in `_shared/08_node_pool.hpp`, and again an **index plus a version counter** is packed into one `Atomic<std::uint64_t>` (the low 32 bits hold the index, the high 32 bits the counter): `head`, `tail`, and every node's `next` are all tagged words of this kind. The homework's `pointer_t` is exactly this.

Values are `std::uint64_t`, stored in the node's `Atomic<std::uint64_t>` (the homework's check-off 5(b) points out that dequeue reads the value **before** the CAS, when someone else may already have recycled and reused the node, so the value must be atomic). `MsQueue<Atomic>(capacity)`: `capacity` elements plus one dummy.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Sequential semantics

```cpp
template <template <typename> class Atomic = std::atomic>
class MsQueue {
public:
    explicit MsQueue(std::size_t capacity);

    bool enqueue(std::uint64_t value);
    std::optional<std::uint64_t> dequeue();
};
```

- `enqueue(value)`: return `false` when the nodes run out.
- `dequeue()`: return `std::nullopt` when the queue is empty.

A new node's `next` must be reset to empty, but **keep and increment** its old version counter (nodes get reused).

## Stage 2 — Concurrency and helping (check-off 3, 4, 5)

Write out the homework's complete procedure:

- `enqueue`: read tail and `tail->next`, and confirm that tail hasn't changed. If `next` is empty, CAS `tail->next` to link in the new node; otherwise tail is lagging, so CAS it forward to help. Once the node is linked, CAS tail over to the new node.
- `dequeue`: read head, tail, and `head->next`, and confirm that head hasn't changed. If head and tail are the same: an empty `next` means the queue is empty, so return `std::nullopt`; otherwise tail is lagging, so help move it. If they differ: read the value **first**, then CAS head to `next`.

Because of helping, a thread stuck at "node linked, tail not yet moved" doesn't block the other threads. The tests enumerate this situation: there must be no deadlock and no endless retrying.

## Stage 3 — ABA (check-off 9)

What the version counter does: after a node is recycled and reused, its index may equal an old value, but the version differs, so a stale CAS fails. The tests enumerate a stale `dequeue` scenario in which another thread cycles head back around to the same index in the meantime.

## Stage 4 — Running it for real

Several producers and consumers: nothing may be lost or duplicated, and each producer's values stay in order. Everything must pass TSan.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Check-off 4: why does enqueue need two CASes? What happens if the second CAS fails (or the thread is killed before it)? How far can tail lag behind at most?
2. Check-off 5: why does dequeue read the value before the CAS? What happens if you reverse the order?
3. Check-off 6: is the lock-free version always faster than the two-lock one? Explain in terms of CAS retries, cache-line contention, and how heavy the contention is.
