---
title: Best Quadrant Slice (max-prefix augmentation)
unit: 04 · Trees & Heaps (PS4)
source: 6.006 PS4 Problem 4-6
pdf: materials/6.006/psets/ps4/questions.pdf
answer: materials/6.006/psets/ps4/solutions.pdf
read: materials/6.006/lectures/lec07.pdf, materials/6.006/recitations/rec07.pdf
kind: code
minutes: 90
---

# 0416 · Best Quadrant Slice

**Original problem:** [Problem Set 4 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/bff0aaeeb571a77083f10fd8e31f71d4_MIT6_006S20_ps4-questions.pdf) · **Read first:** [Lecture 7: Binary Trees, Part 2: AVL notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/a2c80596cf4a2b5fbc854afdd2f23dcb_MIT6_006S20_lec7.pdf), [Recitation 7 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/16d43dec3488717057d7daff2bac3400_MIT6_006S20_r07.pdf)

> **Why this comes up in practice:** this is how you pick two thresholds that maximize expected revenue (for example, showing an ad to users with age ≤ X and spending ≤ Y), or find the quadrant of monitoring data where the sum of some signed metric is largest. What brings the scan down from O(n²) to O(n log n) is maintaining a max-prefix on a balanced tree.

## Original problem

6.006 PS4 Problem 4-6 is about cutting a pizza. The pizza is a disc of radius z with n toppings labeled 0 to n − 1, and z fits in one machine word, so arithmetic on O(1) such integers takes O(1) time. Topping i sits at integer coordinates (x_i, y_i) with x_i, y_i in R = {−z, …, z}, all coordinates are distinct, and the topping has an integer tastiness t_i ∈ R, which may be negative. Choosing a point (x′, y′) and cutting straight down and straight left from it gives a slice: the part of the pizza where x ≤ x′ and y ≤ y′. The slice's tastiness is the sum of t_i over all toppings with x_i ≤ x′ and y_i ≤ y′, and the goal is a tastiest slice, one of maximum tastiness. Assume that some slice has positive tastiness.

- (a) Show that if a point (x′, y′) yields a slice of tastiness t ≠ 0, then some point (x_i, y_j) with i, j ∈ {0, …, n − 1} yields a slice of the same tastiness t, so a tastiest slice can always be cut at a point lined up with toppings in both directions.
- (b) Show how to change a Set AVL tree so that it stores key-value items (each item x has a value x.val besides the key x.key that orders the tree), supports a new operation max_prefix() that returns in worst-case O(1) time a pair (k*, prefix(k*)), where k* is a stored key that maximizes prefix(k), the sum of x.val over all items with x.key ≤ k, and keeps the running times of all the other Set AVL operations.
- (c) Treating (b) as a black box, give a worst-case O(n log n)-time algorithm that returns a triple (x, y, t) such that the point (x, y) yields a slice of maximum tastiness t.
- (d) Implement (c), including the data structure from (b), in the Python template as `tastiest_slice(toppings)`.

## Your task

There are `n` points in the plane, each with a signed value (`Topping { x, y, t }`). Pick a cut point `(X, Y)`; the value of the slice is the sum of `t` over all points with `x ≤ X` and `y ≤ Y` (a slice that contains no points has value 0). Find a slice whose value is the maximum and is positive.

Stage 1 is the augmented tree from part (b), Stage 2 is the algorithm from parts (c) and (d), and Stage 3 drops the original's assumption that all coordinates are distinct.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — An AVL tree with a max-prefix augmentation

```cpp
class PrefixTree {
public:
    PrefixTree();
    bool insert(std::int64_t key, std::int64_t value);
    bool erase(std::int64_t key);
    std::size_t size() const;
    std::optional<std::int64_t> value_of(std::int64_t key) const;
    std::optional<std::pair<std::int64_t, std::int64_t>> max_prefix() const;
};
```

Add an augmentation to 0403's `AvlMap` or 0402's `AvlTree` (you should `#include "../0403-avl-map/solution.hpp"`).

`PrefixTree` maintains a set of `key → value` pairs (both are `int64_t`), ordered by key:

- `insert(key, value)`: return `true` for a new key; if the key already exists, overwrite its value and return `false`. O(log n).
- `erase(key)`: return `true` if it removed a key. O(log n).
- `value_of(key)` and `size()`.
- `max_prefix()`: **O(1)**. Define `prefix(k)` as the sum of the values of all keys `key ≤ k`. Return `(k*, prefix(k*))`, where `k*` is a key **present in the tree** that maximizes `prefix(k*)`. Return `std::nullopt` if the tree is empty.

Hint: each subtree's summary must store the total and the best prefix's value and key. This is a monoid (think about how to compute `combine(a, b)`, especially when the best prefix lies in `b`). When all the values are negative, `prefix` can be negative too; then `k*` is the key whose prefix is the least negative.

## Stage 2 — The best slice

```cpp
struct Topping {
    std::int64_t x;
    std::int64_t y;
    std::int64_t t;
};

struct Slice {
    std::int64_t x;
    std::int64_t y;
    std::int64_t t;
    friend bool operator==(const Slice&, const Slice&) = default;
};

inline std::optional<Slice> tastiest_slice(std::span<const Topping> toppings);
```

`tastiest_slice(toppings)`: assume that all points have distinct `x` values and distinct `y` values. Return any `Slice{X, Y, t}` where `X` is some point's `x`, `Y` is some point's `y`, `t` is the value of that slice, and `t` is the maximum over all slices.

If no slice has a positive value, return `std::nullopt`.

**O(n log n)**. Sort by `x`, then add the points to the Stage 1 tree in that order (the key is `y` and the value is `t`), reading `max_prefix()` after each one.

## Stage 3 — Repeated coordinates

```cpp
// in class PrefixTree
void add(std::int64_t key, std::int64_t delta);
```

The same `x` or the same `y` may appear more than once (each `Topping` counts on its own, even when two are identical; it's the coordinates that repeat):

- Add `add(key, delta)`: if the key doesn't exist, it is the same as `insert(key, delta)`; otherwise add `delta` to its value. O(log n).
- You may read `max_prefix()` only **after all the points** with the same `x` have been added; otherwise you would count a slice that contains only some of the points sharing that `x`. Points with the same `y` must be added to the same key.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why can the best slice always be taken at some point's `x` and some point's `y`?
2. Why is sorting by `x` enough for the scan, instead of enumerating every `(x, y)` pair? How much does the complexity differ?
3. If points keep being added and deleted and you must maintain the best slice at all times, does this approach still work? What's missing?

---

**Official solution:** [Problem Set 4 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/0462839ce975fea99f6a7b8adc8e1477_MIT6_006S20_ps4-solutions.pdf). Read it after you finish.
