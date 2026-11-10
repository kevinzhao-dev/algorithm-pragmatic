---
title: Hash Table That Ignores Insertion Order
unit: 12 · Deterministic Execution (HW9)
source: 6.172 HW9 §7 Write-up 4 (deterministic hashtable)
pdf: materials/6.172/homework/hw09/handout.pdf
read: materials/6.172/lectures/lec16.pdf
kind: code
minutes: 70
sanitizer: tsan
---

# 1214 · Hash Table That Ignores Insertion Order

**Original problem:** [HW9: Deterministic Execution](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/ddcc1a4046b1893ca38211dc54be44fc_MIT6_172F18hw9.pdf) · **Read first:** [Nondeterministic Parallel Programming](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/e4fe97d2a250b57bb1e330fba8cf5992_MIT6_172F18_lec16.pdf)

> **Why this comes up in practice:** for a parallel program to be reproducible, a data structure's final state can't depend on who inserted first. The same property lets snapshots be compared byte for byte and makes builds reproducible; a history-independent structure can even avoid leaking the order of operations through its memory layout.

## Original problem

6.172 HW9 §7 (Deterministic Hashtable) closes the homework with a single design question and no code to write.

- Write-up 4: how would you implement a deterministic hash table, that is, one whose final state is the same no matter in which order the insertions happen?

## Your task

The homework's Write-up 4 only asks "how would you do it"; here you build it. The approach is **prioritized linear probing**: the key itself is the priority, and larger values win.

`DeterministicTable<Atomic>(capacity)`: capacity is a power of 2, keys are never 0, the home slot is `key % capacity`, and the caller guarantees that there are fewer keys than capacity. Declare every shared field with the template parameter `Atomic` (default `std::atomic`).

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Ordering by priority

```cpp
template <template <typename> class Atomic = std::atomic>
class DeterministicTable {
public:
    explicit DeterministicTable(std::size_t capacity);

    void insert(std::uint64_t key);
    std::vector<std::uint64_t> layout() const;
    std::pair<bool, std::size_t> lookup(std::uint64_t key) const;
    bool contains(std::uint64_t key) const;
};
```

`insert(key)`: probe from the home slot; at each slot:

- Empty: put the key there and stop.
- The same key: do nothing and stop.
- The key there is **smaller than the one in hand**: swap the two, carry the key that was swapped out instead, and continue from the **next slot**.
- The key there is larger than the one in hand: move on to the next slot.

`layout()` returns the whole slot array (0 for an empty slot). Whatever the insertion order, the same set of keys must produce exactly the same `layout()`.

`lookup(key)` returns a `std::pair<bool, std::size_t>`: whether the key was found, and how many slots were examined. Lookups in this structure can stop early: seeing an empty slot, or a key smaller than `key`, means `key` isn't present (think about why). `contains(key)` returns only the first element.

## Stage 2 — Concurrent inserts

Change `insert` to use CAS: at each step, read the slot first; when you decide to place or swap, do it with a CAS, and if the CAS fails, reread the same slot. No locks are needed.

Within an insert-only phase, the final `layout()` must be exactly the same as the serial version's, **however the threads interleave**. The tests enumerate interleavings. The same key may be inserted by several threads at once, and the result must still hold only one copy.

Lookups are only called during phases with no inserts.

## Stage 3 — Building the table in parallel

```cpp
// in class DeterministicTable<Atomic>
void insert_all(std::span<const std::uint64_t> keys);
```

`insert_all(keys)`: insert a batch of keys in parallel with `practice::parallel_for` from `harness/forkjoin.hpp`. Whatever the number of workers, `layout()` must match the serial version's.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why can `lookup` stop when it sees a key smaller than its own? Why does that condition still hold when keys get swapped?
2. Write-up 4: another approach is to insert everything and then sort. Compared with this exercise's approach, what are the trade-offs of each?
3. Why can this structure only be phase-concurrent? If inserts and lookups ran at the same time, what might a lookup see?
