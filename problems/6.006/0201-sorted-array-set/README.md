---
title: Sorted Array Set (binary search and order queries)
unit: 02 · Sorting & Recurrences (PS2)
source: 6.006 Lecture 3, Recitation 3
pdf: materials/6.006/recitations/rec03.pdf
read: materials/6.006/lectures/lec03.pdf
kind: code
minutes: 75
---

# 0201 · Sorted Array Set

**Based on:** [Recitation 3 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/1869dbf640ded6b31f1bd369d2001ef5_MIT6_006S20_r03.pdf) · **Read first:** [Lecture 3: Sorting notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6d1ae5278d02bbecb5c4428928b24194_MIT6_006S20_lec3.pdf)

> **Why this comes up in practice:** a read-only index (an SSTable's index block, the lookup table of a static config file) is often just a sorted array plus binary search: it uses less memory than a tree and is friendlier to the cache. Getting binary search's boundaries wrong is also the most common way to crash and burn in an interview.

## Your task

The Set interface from Lecture 3: items have unique keys, and you can look an item up by key and also ask for "the next one after k." Implement it with an array sorted by key, so that every lookup is O(log n). **Do not use `std::map`, `std::set`, or `std::lower_bound`**: write the binary search yourself. For the sorting itself, this exercise uses the standard library (`std::stable_sort`); you write sorts by hand in 0202 and 0203.

`Key` is only guaranteed to have `<` and `==`. The tests count comparisons with `Counted` to confirm that you really are doing binary search.

You write the two binary searches first, then the set's static and dynamic operations, and finish with the range and as-of queries that a time-series index needs.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Binary Search

```cpp
template <typename T>
std::size_t lower_bound_index(std::span<const T> sorted, const T& key);

template <typename T>
std::size_t upper_bound_index(std::span<const T> sorted, const T& key);
```

`sorted` is a `std::span<const T>`, already sorted in increasing order, and it **may contain duplicates**:

- `lower_bound_index<T>(sorted, key)`: the first `i` with `sorted[i] >= key` (return `size()` when every element is less than key).
- `upper_bound_index<T>(sorted, key)`: the first `i` with `sorted[i] > key` (return `size()` when no element is greater than key).

Use at most 2·⌈log₂(n+1)⌉ + 2 comparisons. Edge cases: an empty array, a key smaller / larger than everything, and a whole run of elements equal to key.

## Stage 2 — Static operations

```cpp
template <typename Key, typename Value>
class SortedArraySet {
public:
    using Item = std::pair<Key, Value>;

    SortedArraySet();
    explicit SortedArraySet(std::vector<Item> items);

    std::size_t size() const;

    const Item* find(const Key& key) const;
    const Item* find_min() const;
    const Item* find_max() const;
    const Item* find_next(const Key& key) const;
    const Item* find_prev(const Key& key) const;

    std::span<const Item> items() const;
};
```

`SortedArraySet<Key, Value>`, where `Item` is `std::pair<Key, Value>`:

| Operation | Behavior | Comparisons |
| --- | --- | --- |
| `SortedArraySet()` / `explicit SortedArraySet(vector<Item>)` | Empty / build; for equal keys, **the later occurrence replaces the earlier one** | O(n log n) |
| `size()` | | |
| `find(k)` | Return a `const Item*` pointing at the item, or `nullptr` if it doesn't exist | O(log n) |
| `find_min()` / `find_max()` | The item with the smallest / largest key, or `nullptr` when the set is empty | O(1) |
| `find_next(k)` | The item with the smallest key **strictly greater than** `k` | O(log n) |
| `find_prev(k)` | The item with the largest key **strictly less than** `k` | O(log n) |
| `items()` | A `std::span<const Item>`, in increasing key order | O(1) |

Note that `k` need not be in the set. A returned pointer stays valid until the next modification.

## Stage 3 — Dynamic operations

```cpp
// in class SortedArraySet<Key, Value>
bool insert(Item item);
std::optional<Item> erase(const Key& key);
```

- `insert(item)`: if the key already exists, **replace** its value and return `false`; otherwise add the item and return `true`. O(log n) comparisons (to find the position); O(n) element moves are fine.
- `erase(k)`: remove and return that item (`std::optional<Item>`), or return `std::nullopt` if it doesn't exist. O(log n) comparisons.

## Stage 4 — Real-world extension: a time-series index

```cpp
// in class SortedArraySet<Key, Value>
std::span<const Item> range(const Key& lo, const Key& hi) const;
const Item* as_of(const Key& t) const;
```

Time-series databases (and snapshot reads in an LSM tree) often use a sorted-array index:

- `range(lo, hi)`: return a `std::span<const Item>` covering every item with `lo ≤ key < hi`. What you return is **a view of a slice of the internal array** (zero-copy), using O(log n) comparisons.
- `as_of(t)`: among the items with `key ≤ t`, the one with the largest key, or `nullptr` if there is none ("the latest version visible at time t").
- When `lo > hi`, `range` returns an empty span.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is `lower_bound` least error-prone when written with `low < high` and the half-open interval `[low, high)`? Why should `mid` be computed as `low + (high - low) / 2`?
2. This Set's `insert` is O(n) while `find` is O(log n): what read/write ratio suits it? Compared with a hash table or a balanced tree, when would you choose it?
3. If you call `insert` after returning a pointer or a span, why might those pointers become invalid? What are `std::vector`'s iterator invalidation rules?
