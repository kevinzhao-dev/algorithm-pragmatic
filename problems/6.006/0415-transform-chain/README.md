---
title: Live Updates to a Chained Matrix Product (augmented sequence)
unit: 04 · Trees & Heaps (PS4)
source: 6.006 PS4 Problem 4-5
pdf: materials/6.006/psets/ps4/questions.pdf
answer: materials/6.006/psets/ps4/solutions.pdf
read: materials/6.006/lectures/lec07.pdf, materials/6.006/recitations/rec07.pdf
kind: code
minutes: 55
---

# 0415 · Live Updates to a Chained Matrix Product

**Original problem:** [Problem Set 4 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/bff0aaeeb571a77083f10fd8e31f71d4_MIT6_006S20_ps4-questions.pdf) · **Read first:** [Lecture 7: Binary Trees, Part 2: AVL notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/a2c80596cf4a2b5fbc854afdd2f23dcb_MIT6_006S20_lec7.pdf), [Recitation 7 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/16d43dec3488717057d7daff2bac3400_MIT6_006S20_r07.pdf)

> **Why this comes up in practice:** forward kinematics for robotic arms and scene graphs, transformation chains on a GPU, and WebGL's matrix stack all multiply a long chain of non-commutative transformations, change just one of them, and need the product of the whole chain right away. Subtree augmentation is what cuts the O(n) recomputation down to O(log n).

## Original problem

6.006 PS4 Problem 4-5 models a robotic arm built from n + 1 rigid links joined in a chain by n rotating joints. Each link's orientation is given relative to the link before it: the change at joint k, between links k and k + 1, is a 4 × 4 transformation matrix M_k, so the whole arm is described by M = (M_0, …, M_{n−1}). The arm's full transformation is the ordered product M_0 · M_1 · … · M_{n−1}. Multiplying two 4 × 4 matrices takes O(1) time, but matrix multiplication is not commutative. Design a database whose operations meet these worst-case bounds:

- `initialize(M)`: build the database from the initial matrices in O(n) time.
- `update_joint(k, M)`: replace M_k with the matrix M in O(log n) time.
- `full_transformation()`: return the arm's current full transformation in O(1) time.

## Your task

A robotic arm consists of a chain of joints, each of which is a matrix `M`, and the transformation of the whole arm is the ordered product `M_0 · M_1 · … · M_{n-1}`. The matrix type `M` provides `M::identity()` and `operator*`, which is **associative but not commutative**; treat `operator*` as O(1).

The stages below give the interface and complexity of `TransformChain<M>` (`n` is the current number of joints). Stage 1 is the original problem's database, and Stages 2 and 3 go beyond it with segment products and with inserting and removing joints.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Updates and the full product

```cpp
template <typename M>
class TransformChain {
public:
    explicit TransformChain(std::vector<M> joints);
    std::size_t size() const;
    void update_joint(std::size_t index, M matrix);
    const M& full_transformation() const;
};
```

- `explicit TransformChain(std::vector<M> joints)`: O(n), with at most `3n` calls to `operator*`.
- `size()`.
- `update_joint(k, M)`: replace joint `k`, using O(log n) calls to `operator*` (at most `4⌈lg n⌉ + 8` in the tests); throw `std::out_of_range` if `k` is out of range.
- `full_transformation()`: return the current product of the whole chain in **O(1), without calling `operator*`**. When there are no joints, return `M::identity()`.

You may `#include "../0404-avl-sequence/solution.hpp"`, treat each matrix as a sequence item, and define an `Aug` whose `combine` is multiplication; you already did the hard part in 0404.

(In this stage the number and order of the joints never change, so a static structure is enough; only Stage 3 needs a dynamic version.)

## Stage 2 — Segment products

```cpp
// in class TransformChain<M>
M segment(std::size_t left, std::size_t right) const;
```

`segment(l, r)`: return `M_l · … · M_{r-1}` using O(log n) calls to `operator*` (at most `8⌈lg n⌉ + 16` in the tests). If `l == r`, return `identity`; if `l > r` or `r > n`, throw `std::out_of_range`.

`full_transformation` is cheaper than `segment(0, n)` because the root already stores the answer.

## Stage 3 — Reassembling the arm

```cpp
// in class TransformChain<M>
void insert_joint(std::size_t index, M matrix);
M remove_joint(std::size_t index);
```

- `insert_joint(k, M)`: insert a new joint at position `k` (`0 ≤ k ≤ n`), in O(log n).
- `remove_joint(k)`: remove and return joint `k`, in O(log n).

Both throw `std::out_of_range` when the position is out of range. One insert plus one remove may use at most `24⌈lg n⌉ + 48` calls to `operator*` in the tests. After this stage, `full_transformation` and `segment` must still be correct and still use only the numbers of multiplications given above.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. When the product isn't commutative, what happens if the calls to `combine(left, right)` come in the wrong order? How do your tests catch it?
2. If joints are never added or removed, what does a complete binary tree (an implicit array) offer compared with an AVL tree?
3. Real transformations use `double` and accumulate rounding error: numerically, which is more accurate, O(log n) updates or recomputing everything every time?

---

**Official solution:** [Problem Set 4 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/0462839ce975fea99f6a7b8adc8e1477_MIT6_006S20_ps4-solutions.pdf). Read it after you finish.
