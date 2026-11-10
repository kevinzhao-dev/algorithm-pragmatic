---
title: Lockless Hash Table Insert (two words, one CAS)
unit: 12 · Deterministic Execution (HW9)
source: 6.172 HW9 §6 Write-up 3 (lockless hashtable)
pdf: materials/6.172/homework/hw09/handout.pdf
read: materials/6.172/lectures/lec17.pdf
kind: code
minutes: 80
sanitizer: tsan
---

# 1213 · Lockless Hash Table Insert

**Original problem:** [HW9: Deterministic Execution](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/ddcc1a4046b1893ca38211dc54be44fc_MIT6_172F18hw9.pdf) · **Read first:** [Synchronization Without Locks](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/fc3b846f6e13e15e4982a92369ca3825_MIT6_172F18_lec17.pdf)

> **Why this comes up in practice:** hardware usually guarantees CAS on only a single word, but a record often has two fields. "Claim the slot with a CAS first, then publish the rest" is the common skeleton of lock-free hash maps, ring buffers, and logging systems, and the hard part is always what a reader sees between the two steps.

## Original problem

6.172 HW9 §6 (Lockless Hashtable) drops the locks from the homework's hash table altogether. Each entry is a pair of 64-bit words: a pointer, which serves as the key, and a size. Modern x86 processors can compare-and-swap 16 bytes at once (`CMPXCHG16B`), but early 64-bit processors could not, so the homework limits itself to a CAS on one 64-bit word, which means it can't write a whole entry atomically. Sizes are always positive, and the homework's `common.h` provides the 64-bit CAS wrapper.

- Write-up 3: implement `hashtable_insert_lockless` with `InterlockedCompareExchange64` (and switch `hashtable_fill` over to it), and check it with `make hashtable-mt1000`. What has to change in `hashtable_lookup` so that the insert can get by with a single 8-byte compare-and-swap?

## Your task

An entry has two 64-bit fields, key and value, but one CAS can change only one word. Keys are never 0 and values are never 0; the home slot is `key % capacity` (capacity is a power of 2); probing is linear, and nothing is ever deleted.

Declare every shared field with the template parameter `Atomic` (default `std::atomic`): the tests swap in `dsched::Atomic` from `_shared/08_dsched.hpp` to enumerate interleavings, and also run TSan with a real `std::atomic`.

You build the homework's lockless insert, together with a lookup that avoids its false negatives, and then go one step further in Stage 3 by publishing a value that isn't atomic at all.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Single-threaded semantics

```cpp
template <template <typename> class Atomic = std::atomic>
class LocklessTable {
public:
    explicit LocklessTable(std::size_t capacity);

    bool insert(std::uint64_t key, std::uint64_t value);
    std::optional<std::uint64_t> find(std::uint64_t key) const;
};
```

`LocklessTable<Atomic>(capacity)`:

- `insert(key, value)`: return `false` when the table is full; if the key already exists, overwrite its value.
- `find(key)`: return the value, or `std::nullopt` if the key isn't there.

## Stage 2 — Concurrent inserts

Make Stage 1 lock-free:

- `insert`: **claim** an empty slot by writing the key into it with a CAS, and only after that succeeds, write the value (between the two steps, other threads can see the key but not the value). If the CAS fails, check whether the slot holds your own key; otherwise keep probing.
- `find`: if the key matches but the value is still 0, the write hasn't finished yet; return `std::nullopt`.

**There must be no false negatives:** once an `insert` has returned, **every** later `find` must find it, even if another slot on the probe path is stuck in the "claimed, but value not yet written" state. The homework's `hashtable_lookup` gives up when it reaches a slot whose size is 0, and that code has exactly this problem; your version must avoid it.

No key is ever inserted by two threads at the same time.

## Stage 3 — Publishing non-atomic content

```cpp
template <template <typename> class Atomic = std::atomic>
class PublishedTable {
public:
    explicit PublishedTable(std::size_t capacity);

    bool insert(std::uint64_t key, std::string text);
    std::optional<std::string> find(std::uint64_t key) const;
};
```

`PublishedTable<Atomic>(capacity)`: the value is now a `std::string` that **is not atomic**.

- `insert(key, text)`: claim the slot with a CAS, move the string into the slot, then publish it with an atomic ready flag. If the key already exists, return `false` and leave the existing content alone; also return `false` when the table is full.
- `find(key)`: read the string only once it is ready, and return a copy of it; otherwise return `std::nullopt`.

You choose the memory orders yourself. If the publishing side and the reading side don't form a happens-before relationship, reading the `std::string` is a data race. TSan only sees the accesses that actually happen in this run, so passing the tests doesn't mean your memory orders are right; think it through yourself.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. What memory order does Stage 2's CAS need, and what does the store of the value need? What breaks if you make the value store relaxed? (Homework Write-up 3)
2. Why is the path where the homework's `hashtable_lookup` returns NULL a false negative? To fix it, what are the trade-offs between continuing to probe and waiting?
3. If you had to support delete, what problems would this design run into? How do tombstones and Robin Hood hashing each handle it?
