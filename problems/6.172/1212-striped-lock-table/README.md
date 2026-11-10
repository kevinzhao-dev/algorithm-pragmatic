---
title: Hash Table Lock Striping (striping and false sharing)
unit: 12 · Deterministic Execution (HW9)
source: 6.172 HW9 §5 Write-up 1–2 (hashlock, fair insert)
pdf: materials/6.172/homework/hw09/handout.pdf
read: materials/6.172/lectures/lec16.pdf
kind: code
minutes: 90
sanitizer: tsan
---

# 1212 · Hash Table Lock Striping

**Original problem:** [HW9: Deterministic Execution](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/ddcc1a4046b1893ca38211dc54be44fc_MIT6_172F18hw9.pdf) · **Read first:** [Nondeterministic Parallel Programming](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/e4fe97d2a250b57bb1e330fba8cf5992_MIT6_172F18_lec16.pdf)

> **Why this comes up in practice:** `ConcurrentHashMap`, Redis slots, and a database's latch manager all do the same thing: one big lock is too slow and one lock per row takes too much space, so the locks are split into stripes. Once they're split, the new problems are false sharing and the order in which locks are taken.

## Original problem

6.172 HW9 §5 (Hashlocking) runs the homework's open-addressing hash table on several threads (`make hashtable-mt` uses 10), where it fails without synchronization. A single lock for the whole table, as in `hashtable_insert_locked`, would be heavily contended. Splitting the lock helps, but one lock per table row costs too much memory and cache, so the handout settles on one lock per hash value, called a "hashlock". You switch `hashtable_fill` over to `hashtable_insert_fair`, a basic hashlock insert. Its locks are an array of 16 pthread mutexes indexed by the slot number modulo 16, and while it probes, it locks the next slot before unlocking the current one.

- Write-up 1: is the hashlock implementation in `hashlock.c` prone to false sharing? Why or why not?
- Write-up 2: what problem does the fairness scheme in `hashtable_insert_fair` introduce? Explain it, or demonstrate the behavior you see.

## Your task

Turn an open-addressing table (linear probing) into a multithreaded version, one step at a time. Keys are never 0, and **no two threads ever insert the same key at the same time**. Key `k`'s home slot is `k % capacity` (capacity is a power of 2), and slot `s` belongs to stripe `s % stripes` (stripes is also a power of 2, and ≤ capacity). `insert` returns `false` when the table is full, and inserting an existing key again overwrites its value.

Everything takes a template parameter `Mutex` (default `std::mutex`); the tests swap in `dsched::Mutex` from `_shared/08_dsched.hpp` to enumerate interleavings, and also run TSan with `std::mutex`.

Where the homework asks two written questions, here you build all four tables, including the one that deadlocks, and let the tests show what goes wrong.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — One big lock

```cpp
template <typename Mutex = std::mutex>
class CoarseTable {
public:
    explicit CoarseTable(std::size_t capacity);

    bool insert(std::uint64_t key, std::uint64_t value);
    std::optional<std::uint64_t> find(std::uint64_t key) const;
};
```

`CoarseTable(capacity)`: `insert` and `find` (`std::optional<std::uint64_t>`) both take the same lock.

## Stage 2 — Lock striping (Write-up 1)

```cpp
template <typename Mutex = std::mutex>
class StripedTable {
public:
    StripedTable(std::size_t capacity, std::size_t stripes);

    bool insert(std::uint64_t key, std::uint64_t value);
    std::optional<std::uint64_t> find(std::uint64_t key) const;
    const void* stripe_address(std::size_t stripe) const;
};
```

`StripedTable(capacity, stripes)`: each slot is protected by the lock of the stripe it belongs to. `insert` probes from the home slot and **holds only one lock at a time**: lock the slot's stripe, look at the slot, unlock it, then move on to the next slot.

Also provide `stripe_address(i)` (the address of the lock of stripe i). The stripe array must avoid false sharing: each stripe's lock must start at a 64-byte-aligned address, and the addresses of adjacent stripes must be at least 64 bytes apart. Padding alone is not enough, because the tests also check the alignment; declare the stripe type with `alignas(64)`. The homework asks: does your stripe array have false sharing? Why?

## Stage 3 — The homework's "fair" insert goes wrong (Write-up 2)

```cpp
template <typename Mutex = std::mutex>
class FairTable {
public:
    FairTable(std::size_t capacity, std::size_t stripes);

    bool insert(std::uint64_t key, std::uint64_t value);
    std::optional<std::uint64_t> find(std::uint64_t key) const;
};
```

`FairTable(capacity, stripes)`: the approach of the homework's `hashtable_insert_fair`. As it walks the probe path, it **acquires the next slot's lock before releasing the current slot's lock** (when the next slot belongs to a different stripe; within the same stripe, there is nothing to do). `find` works the same way.

You must **write it exactly to this spec**, and then watch the tests demonstrate why it is broken: they enumerate interleavings and find a schedule that really deadlocks. (This exercise deliberately asks you to implement the buggy version.)

## Stage 4 — Fix it with a global order

```cpp
template <typename Mutex = std::mutex>
class OrderedFairTable {
public:
    OrderedFairTable(std::size_t capacity, std::size_t stripes);

    bool insert(std::uint64_t key, std::uint64_t value);
    std::optional<std::uint64_t> find(std::uint64_t key) const;
};
```

`OrderedFairTable(capacity, stripes)`: walk forward the same way, but **keep holding the old lock while taking the new one only when the next slot's stripe number is larger than the current one**; as soon as the next slot's stripe number is smaller (the probe wrapped across the stripe boundary), release the old lock first and then take the new one. This makes the lock acquisition order globally monotonic, so no cycle can form.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Packing `pthread_mutex_t` tightly into an array causes false sharing. Why does that hurt lock striping in particular? How big is a cache line on Apple Silicon?
2. What is the minimum number of threads for the Stage 3 deadlock? Why might it not happen with the homework's 16 locks and 10 threads? Stage 2's approach has no deadlock at all; what does it cost?
3. What happens in Stage 2 when two threads insert the same key at the same time? How would you handle it? (Hint: once you give up hand-over-hand, a key that someone else inserts during your probe may land after a position you have already checked.)
