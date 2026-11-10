---
title: Batch Operations on the Sequence Interface
unit: 01 · Sequences (PS1)
source: 6.006 PS1 Problem 1-2
pdf: materials/6.006/psets/ps1/questions.pdf
answer: materials/6.006/psets/ps1/solutions.pdf
read: materials/6.006/lectures/lec02.pdf
kind: code
minutes: 50
---

# 0112 · Batch Operations on a Sequence

**Original problem:** [Problem Set 1 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/dc56bd1995baf20cfcdd3fbabdd2dbc9_MIT6_006S20_ps1-questions.pdf) · **Read first:** [Lecture 2: Data Structures notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/79a07dc1cb47d76dae2ffedc701e3d2b_MIT6_006S20_lec2.pdf)

> **Why this comes up in practice:** many data structures (a sequence built on a balanced tree, a rope, the document model of a collaborative editor) offer only `insert_at` / `delete_at`, so you have to assemble every other batch operation yourself, and its cost must be proportional to "how much you move," not to the length of the whole sequence.

## Original problem

6.006 PS1 Problem 1-2 hands you a data structure D that supports the Sequence operations `D.build(X)` in O(n) time, and `D.insert_at(i, x)` and `D.delete_at(i)` in O(log n) time each, where n is the number of items stored in D when the operation runs; `delete_at` returns the item it removes. Using only these lower-level operations, describe algorithms for two higher-level ones, each running in O(k log n) time:

- (a) `reverse(D, i, k)`: flip the order of the k consecutive items that begin at index i, that is, indices i through i + k − 1.
- (b) `move(D, i, k, j)`: take the k consecutive items that begin at index i and relocate them, keeping their order, so that they sit just before the item at index j. You may assume that i ≤ j < i + k does not hold.

The answer is written; there is no code.

## Your task

The problem set gives you a Sequence on which only `insert_at` and `delete_at` are available, and asks you to build range reversal and range moves out of those two operations: when each operation costs O(log n), the total cost must be O(k log n). Here, "each operation costs O(log n)" is replaced by **counting the calls directly**: the tests pass in a counting sequence, and your functions may call only these three of its operations:

```cpp
seq.size();
seq.insert_at(i, std::move(item));   // 0 ≤ i ≤ seq.size(); inserts before index i
auto item = seq.delete_at(i);        // deletes and returns it; 0 ≤ i < seq.size()
```

Every function is a template (`Seq` is only guaranteed to have these three operations), and elements are only guaranteed to be movable (`std::unique_ptr` must work too). When the arguments are invalid, throw `std::invalid_argument` and leave the sequence unchanged.

Stage 3 adds a range rotation built on `move_range`, which the original problem doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — `reverse_range(seq, i, k)`

```cpp
template <typename Seq>
void reverse_range(Seq& seq, std::size_t i, std::size_t k);
```

Reverse the `k` consecutive elements that start at index `i`.

- Requires `i + k ≤ seq.size()`. When `k ≤ 1`, there is nothing to do.
- The **total number of `insert_at` + `delete_at` calls must be ≤ 2k**.

## Stage 2 — `move_range(seq, i, k, j)`

```cpp
template <typename Seq>
void move_range(Seq& seq, std::size_t i, std::size_t k, std::size_t j);
```

Move the `k` elements that start at index `i`, in their original order, to just in front of "the element currently at index `j`." `j` is an index from **before the move**.

- Requires `i + k ≤ seq.size()` and `j ≤ seq.size()` (`j == seq.size()` means moving to the end), and `j` must not fall inside the range (`i ≤ j < i + k` must be false). `k == 0` does nothing.
- The total number of calls must be **≤ 2k**. The hard part is that the indices shift after every delete, so work out how `j` has to be adjusted when moving left and when moving right.

## Stage 3 — `rotate_left(seq, i, k, m)`

```cpp
template <typename Seq>
void rotate_left(Seq& seq, std::size_t i, std::size_t k, std::size_t m);
```

Rotate the `k` elements that start at `i` left by `m` positions (the first `m` elements of the range move to the end of the range; `0 ≤ m ≤ k`, otherwise throw `std::invalid_argument`).

Build it from `move_range`, and **pick the cheaper direction**: the total number of calls must be ≤ 2·min(m, k − m).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Suppose the sequence's `insert_at` and `delete_at` are O(log n) (an AVL tree, for example). Why can `reverse_range` reach O(k log n) without any special balanced-tree operations? If `k` is close to `n`, is there a faster way?
2. Why do the index shifts in `move_range` differ between the `j > i` case and the `j < i` case? Explain your loop with an invariant.
3. If the sequence supported O(log n) split / concatenate (a treap, for example), how could you rework `move_range`, and what would it cost?

---

**Official solution:** [Problem Set 1 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/10e4e3444c1bde4c662a47a8f14dadd3_MIT6_006S20_ps1-solutions.pdf). Read it after you finish.
