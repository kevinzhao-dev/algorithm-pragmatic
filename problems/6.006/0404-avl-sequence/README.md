---
title: Sequence AVL (indexing and range folds)
unit: 04 · Trees & Heaps (PS4)
source: 6.006 Lecture 7, Recitation 7
pdf: materials/6.006/recitations/rec07.pdf
read: materials/6.006/lectures/lec06.pdf, materials/6.006/lectures/lec07.pdf
kind: code
minutes: 80
---

# 0404 · Sequence AVL

**Based on:** [Recitation 7 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/16d43dec3488717057d7daff2bac3400_MIT6_006S20_r07.pdf) · **Read first:** [Lecture 6: Binary Trees, Part 1 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/376714cc85c6c784d90eec9c575ec027_MIT6_006S20_lec6.pdf), [Lecture 7: Binary Trees, Part 2: AVL notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/a2c80596cf4a2b5fbc854afdd2f23dcb_MIT6_006S20_lec7.pdf)

> **Why this comes up in practice:** a text editor's rope, and any sequence that needs both insertion in the middle and random access, use this structure. An array's `insert_at` is O(n) and a linked list's `get_at` is O(n); this tree does both in O(log n).

## Your task

Use the `AvlTree` from 0402 to implement a **Sequence**: this time the traversal order is the order of the sequence and has nothing to do with keys. `#include "../0402-avl-tree/solution.hpp"` and reuse your engine; don't rewrite the balancing logic.

```cpp
template <typename T, typename Aug = NoAugment>
class AvlSequence;
```

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Construction and indexing

```cpp
template <typename T, typename Aug = NoAugment>
class AvlSequence {
public:
    using Tree = AvlTree<T, Aug>;
    using Node = typename Tree::Node;
    using Value = typename Aug::Value;

    AvlSequence() = default;
    explicit AvlSequence(std::vector<T> items);

    std::size_t size() const;
    bool empty() const;
    const Tree& tree() const;
    Node* node_at(std::size_t index) const;
    const T& at(std::size_t index) const;
    void set_at(std::size_t index, T value);
    std::vector<T> to_vector() const;
};
```

- `explicit AvlSequence(std::vector<T> items)`: build the tree in O(n) (with 0402's `build`).
- `node_at(i)`: the `i`-th node (counting from 0), in O(log n); throw `std::out_of_range` if `i >= size()`. This is the core of the other operations.
- `at(i)` / `set_at(i, v)`: O(log n); `set_at` must maintain the augmentation. Both throw `std::out_of_range` if `i >= size()`.
- `size()` / `empty()` / `to_vector()` / `tree()`.

## Stage 2 — Inserting and deleting in the middle

```cpp
// in class AvlSequence<T, Aug>
void insert_at(std::size_t index, T value);
T delete_at(std::size_t index);
```

- `insert_at(i, v)`: with `0 ≤ i ≤ size()`, put `v` at position `i` (the elements from the old position `i` onward move back by one); throw `std::out_of_range` if `i > size()`.
- `delete_at(i)`: remove and return the `i`-th element; throw `std::out_of_range` if `i >= size()`.

Both are O(log n).

## Stage 3 — Range fold

```cpp
// in class AvlSequence<T, Aug>
Value fold(std::size_t left, std::size_t right) const;
```

`fold(l, r)`: fold the `Aug` summaries over the range `[l, r)`, applying `combine` in sequence order, in O(log n). If `l == r`, return `identity()`; if `l > r` or `r > size()`, throw `std::out_of_range`.

**`combine` is not guaranteed to be commutative**, and the left-right order is the trap in this stage. Each node already stores the summary of its whole subtree, so you don't need to visit every element in the range: for a subtree that lies entirely inside the range, use `node->aug` directly, and at each level descend into at most two more branches. The number of calls to `combine` must be O(log n) (the tests allow `8 lg n + 16` per query).

Recitation 7's `flip(i)`/`count_ones_upto(i)` is exactly this mechanism (with `Aug` being the sum of the bits).

## Stage 4 — Real-world extension: text buffer

```cpp
class TextBuffer {
public:
    explicit TextBuffer(std::string_view text = "");

    std::size_t size() const;
    char at(std::size_t index) const;
    void insert(std::size_t index, char ch);
    void erase(std::size_t index);
    std::string substr(std::size_t left, std::size_t right) const;
    bool equal_ranges(std::size_t a, std::size_t b, std::size_t length) const;
};
```

`TextBuffer` (you can implement it with `AvlSequence<char, YourHashAug>`):

- `TextBuffer(text)`: built in O(n). `size()` and `at(i)`.
- `insert(i, ch)` and `erase(i)`: O(log n).
- `substr(l, r)`: the contents of `[l, r)`, in O(r − l + log n); throw `std::out_of_range` for an invalid range.
- `equal_ranges(a, b, length)`: whether `[a, a+length)` and `[b, b+length)` have the same contents, in **O(log n)**; throw `std::out_of_range` for an invalid range.

Hint: a polynomial hash is a monoid too. Store `(hash, base^len)` as the value, and `combine` is `(h_a · p_b + h_b, p_a · p_b)`, all modulo a large prime (for example `2^61 - 1`). `equal_ranges` compares the summaries of the two ranges; it's fine as long as the probability of a wrong answer is small enough.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does `fold` only need to visit O(log n) nodes? What is special about the nodes that `[l, r)` covers completely?
2. If range operations also had to support adding a number to a whole range, what would you change? (This is lazy propagation.)
3. When `equal_ranges` returns `true`, are the two ranges always actually equal? How do you reduce false positives? What does a false positive cost?
