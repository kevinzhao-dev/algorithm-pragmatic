---
title: Two-Lock FIFO Queue (a head lock and a tail lock)
unit: 13 · Data Synchronization (HW10)
source: 6.172 HW10 check-off 1, 2, 7 (two-lock queue)
pdf: materials/6.172/homework/hw10/handout.pdf
read: materials/6.172/lectures/lec17.pdf
kind: code
minutes: 80
sanitizer: tsan
---

# 1311 · Two-Lock FIFO Queue

**Original problem:** [HW10: Data Synchronization](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/64f4db5020ca72ab205e458d40bbdfe3_MIT6_172F18hw10.pdf) · **Read first:** [Synchronization Without Locks](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/fc3b846f6e13e15e4982a92369ca3825_MIT6_172F18_lec17.pdf)

> **Why this comes up in practice:** a work queue's producers and consumers usually run on different threads, and a single lock makes them wait for each other. Split the lock into a head lock and a tail lock, and both ends can work at the same time; this is the starting point of almost every bounded queue.

## Original problem

6.172 HW10 (Data Synchronization) is a written homework that compares two FIFO queues given as C-like pseudocode: a lock-based queue (Figure 1) and a lock-free queue (Figures 2 and 3, restated in 1312). Both take their nodes from a pool allocated in advance, where `new_node()` grabs a free node and `free_node(node)` returns one, and both start with a dummy node that is never dequeued and only marks the queue as empty. In the lock-based queue, a node holds a value and a next pointer, and the queue holds a head pointer and a tail pointer, each with its own lock:

- Enqueue fills in a new node whose next is null, then, holding the tail lock, links it after the current tail and moves tail to it.
- Dequeue, holding the head lock, looks at the node after head. If there is none, the queue is empty and dequeue fails; otherwise it copies out that node's value, moves head to it, releases the lock, and frees the old dummy.

The handout asks you to assume that the compiler doesn't reorder instructions, that the pool never runs out, and that nodes don't cross cache lines, so every write is atomic. Three check-off questions concern this queue:

- Check-off 1: what constraints must enqueue and dequeue satisfy in a FIFO queue? (This one doesn't need the code.)
- Check-off 2: what do two locks gain over a single lock?
- Check-off 7: simplify the lock-based code for the case where only one thread may enqueue. Write the pseudocode with comments, and argue why every possible execution still keeps FIFO order.

## Your task

Michael & Scott's two-lock queue: a dummy node keeps head and tail pointing at different nodes (or at the same node when the queue is empty), `enqueue` touches only tail, and `dequeue` touches only head.

The nodes come from `practice::NodePool` in `_shared/08_node_pool.hpp` and are preallocated at initialization (the homework's `new_node` / `free_node`). The template parameters `Mutex` (default `std::mutex`) and `Atomic` (default `std::atomic`) let the tests replace the synchronization: they swap in `dsched::Mutex` / `dsched::Atomic` to enumerate interleavings, and also run TSan with real locks.

Instead of answering on paper, you implement the homework's lock-based queue in C++, and then the single-enqueuer simplification from check-off 7.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Sequential semantics (check-off 1)

```cpp
template <typename T, typename Mutex = std::mutex, template <typename> class Atomic = std::atomic>
class TwoLockQueue {
public:
    explicit TwoLockQueue(std::size_t capacity);

    bool enqueue(T value);
    std::optional<T> dequeue();
};
```

`TwoLockQueue<T, Mutex, Atomic>(capacity)`: holds at most `capacity` elements at once (the pool needs one extra node for the dummy).

- `enqueue(value)`: return whether it succeeded; return `false` when the nodes run out.
- `dequeue()`: return a `std::optional<T>`, or `std::nullopt` when the queue is empty.

`T` may be move-only (for example `std::unique_ptr`); you must not require it to be copyable.

## Stage 2 — Two locks (check-off 2)

`enqueue` takes only the tail lock and `dequeue` takes only the head lock, so both sides can be inside their own critical sections at the same time.

**Note:** when the queue is empty, tail and head point to the same node; the enqueuer writes that node's `next` while the dequeuer reads its `next`. That is a real race, so `next` must be an `Atomic`. The homework's pseudocode treats it as a plain pointer because it assumes there that writes are atomic; in C++ you must say so explicitly.

The FIFO constraint (check-off 1): values that one producer enqueues in order must be dequeued in the same order, and every value is taken out exactly once.

## Stage 3 — Only one enqueuer (check-off 7)

```cpp
template <typename T, typename Mutex = std::mutex, template <typename> class Atomic = std::atomic>
class SingleEnqueuerQueue {
public:
    explicit SingleEnqueuerQueue(std::size_t capacity);

    bool enqueue(T value);
    std::optional<T> dequeue();
};
```

`SingleEnqueuerQueue<T, Mutex, Atomic>(capacity)`: only one thread is guaranteed to call `enqueue`, so you can drop the tail lock, and `enqueue` takes no lock at all. `dequeue` stays the same (there may still be several consumers).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is a dummy node needed? Without the dummy, what goes wrong when the queue is empty? (The homework's Figure 1, before check-off 3)
2. Check-off 2: where is the throughput ceiling of the two-lock version? Under what workload is it about the same as a single lock?
3. In Stage 3, why can the enqueuer take no lock at all? If there is also only one consumer, does `dequeue` still need the head lock?
