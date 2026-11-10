---
title: Skip List Finger Search and Rank
unit: 05 · Skip Lists, Hashing, and DP (PS5)
source: 6.046J PS5 Problem 5-1
pdf: materials/6.046j/psets/ps05/questions.pdf
answer: materials/6.046j/psets/ps05/solutions.pdf
read: materials/6.046j/lectures/lec07.pdf
kind: code
minutes: 210
---

# 0511 · Skip List Finger Search and Rank

**Original problem:** [Problem Set 5](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/45952953cb13d077bd35d4397d67fa7c_MIT6_046JS15_pset5.pdf) · **Read first:** [Randomization: Skip Lists](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/14fe095781943ff9bd485e6e263b48f7_MIT6_046JS15_lec07.pdf)

> **Why this comes up in practice:** queries often have locality: turning to the next page, looking at the neighboring rank, stepping a few records forward in a time series. A search from the head costs O(log n), but when you already hold a nearby node, the cost should depend only on the distance.

## Original problem

6.046J PS5 Problem 5-1 ("New Operations for Skip Lists") is about using skip lists, possibly augmented, for queries whose answer lies near a node you already hold. FINGER-SEARCH(x, k) receives a node x and a key k, and returns the node y that holds k (you may assume k is present); it should be fast when x and y are close together.

- (a) Write pseudocode for FINGER-SEARCH(x, k) on a skip list with distinct keys, where x is a node of the level-0 list and the answer y is returned from the level-0 list. With m = 1 + |rank(x.key) − rank(k)|, where rank is a key's index in the sorted order of the set at the time of the call, it must take O(lg m) steps with high probability with respect to m: for every positive integer α, it finishes in O(lg m) time with probability at least 1 − 1/m^α, with a constant that may depend on α. Prove this bound. You may assume that every level begins with −∞ and ends with +∞.
- (b) RANK-SEARCH(x, r) receives a node x and a positive integer r, and returns the node whose key has rank rank(x) + r (you may assume it exists). Decide what extra information the nodes store so that rank search can be fast, and show that maintaining it keeps SEARCH, INSERT, and DELETE within their usual high-probability bounds.
- (c) Write pseudocode for RANK-SEARCH(x, r) under the same assumptions as part (a). It must take O(lg m) steps with high probability, in the same sense, for m = r + 1. Prove this bound.

## Your task

Extend the skip list from [`0501`](../0501-skip-list/README.md) into `IndexedSkipList<Key>`: every level is a doubly linked list, and every link records how many level-0 nodes it spans. The inside of `Node` is up to you; it only has to expose `key()`. A node pointer must stay valid until that node is deleted.

`m = 1 + |rank(x) − rank(target)|`. The queries in Stage 1 and Stage 3 must both take O(lg m), independent of `n`.

Stage 1 is part (a), Stage 2 is part (b), and Stage 3 is part (c). Unlike the original, searches must handle keys that are absent and jumps in either direction, and Stage 4 builds a sliding median on top.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Cost model

`steps()`: the total number of links followed by `finger_search`, `rank`, `at`, and `rank_search`, where moving to another level also counts as one step. `insert`, `erase`, and `find` don't need to be counted.

## Stage 1 — Doubly linked levels and finger search

```cpp
template <typename Key>
class IndexedSkipList {
public:
    class Node {
    public:
        const Key& key() const;
    };

    explicit IndexedSkipList(std::uint64_t seed = 6046);

    ~IndexedSkipList();  // free every node, iteratively

    IndexedSkipList(const IndexedSkipList&) = delete;
    IndexedSkipList& operator=(const IndexedSkipList&) = delete;

    std::size_t size() const;
    bool empty() const;
    Node* first() const;
    Node* next(const Node* node) const;
    Node* prev(const Node* node) const;
    Node* find(const Key& key) const;
    Node* insert(Key key);
    bool erase(const Key& key);
    Node* finger_search(Node* from, const Key& key) const;

    // Cost model used by the tests: links followed plus level changes.
    std::uint64_t steps() const;
};
```

The basic operations: `insert(key)` returns the new node (or `nullptr` if the key already exists), plus `erase(key)`, `find(key)`, `first()`, `next(node)`, `prev(node)`, `size()`, and `empty()`. Walking off either end returns `nullptr`. The destructor must not recurse.

`finger_search(from, key)`: search for `key` starting from `from`. **It must support both directions, left and right**, and return `nullptr` if the key is absent.

It works in two phases: first climb up while moving toward the target, until going any higher would overshoot it; then move toward the target while descending one level at a time. The tests count comparisons with `Counted`.

## Stage 2 — Widths for ranks

```cpp
// in class IndexedSkipList<Key>
std::size_t rank(const Node* node) const;
Node* at(std::size_t index) const;
```

Every link also stores a width (how many level-0 nodes the link spans). Both `insert` and `erase` must keep the widths correct without changing their complexity.

- `rank(node)`: the node's 0-based rank, in O(log n).
- `at(index)`: return the node at position `index`, or `nullptr` if it is out of range, in O(log n).

## Stage 3 — Rank search

```cpp
// in class IndexedSkipList<Key>
Node* rank_search(Node* from, std::ptrdiff_t delta) const;
```

`rank_search(from, delta)`: return the node whose rank is `rank(from) + delta`. `delta` may be positive or negative; `delta == 0` returns `from`, and a rank out of range returns `nullptr`. **O(lg m)** with `m = |delta| + 1`, so you can't call `rank` and then `at`.

## Stage 4 — Real-world extension: sliding median

```cpp
class SlidingMedian {
public:
    explicit SlidingMedian(std::uint64_t seed = 6046);

    std::size_t size() const;
    std::optional<std::int64_t> median() const;
    void push(std::int64_t value);
    void pop();
    std::uint64_t steps() const;
};
```

`SlidingMedian(seed)`: maintain a window of values in order of arrival.

- `push(value)`: add a new value; duplicates are allowed.
- `pop()`: remove the oldest value; throw `std::out_of_range` when the window is empty.
- `median()`: return the value of rank `(n−1)/2` (the lower of the two middle values when the count is even), or `std::nullopt` when the window is empty.
- `size()`, `steps()`: `steps()` returns the total steps taken by the internal list.

Each update moves the median by at most one position, so hold on to the current median's node and move it with `rank_search`. The average number of steps per update must be constant; calling `at` from the head every time is caught by the tests.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is the height reached in the climbing phase of finger search bounded by O(lg m)? How does this differ from the usual O(lg n) analysis of a skip list? (Hint: there are only `m` nodes in the range.)
2. How are the widths updated on insertion? Why is the search path alone enough on deletion?
3. Can a B-tree support the same finger search? If you need concurrency, which fits better, a skip list or a B-tree?

---

**Official solution:** [Solutions to Problem Set 5](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/bbcc32bdd480116ca12b065cf6d6c454_MIT6_046JS15_pset5sols.pdf). Read it after you finish.
