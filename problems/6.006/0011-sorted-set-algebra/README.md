---
title: Sorted Set Intersection, Union, and Difference (posting lists)
unit: 00 · Warm-up (PS0)
source: 6.006 PS0 Problem 0-1
pdf: materials/6.006/psets/ps0/questions.pdf
answer: materials/6.006/psets/ps0/solutions.pdf
kind: code
minutes: 50
---

# 0011 · Sorted Set Intersection, Union, and Difference

**Original problem:** [Problem Set 0 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/60f0a029ab154553cad51b54cc25605b_MIT6_006S20_ps0-questions.pdf)

> **Why this comes up in practice:** a search engine's inverted index maps each term to a sorted list of document IDs, called a posting list. `a AND b` is the intersection of two lists, and `a NOT b` is their difference. A database merge join uses the same trick.

## Original problem

6.006 PS0 Problem 0-1 is a pencil-and-paper warm-up. It defines two sets in set-builder notation,

- A = { i + C(5, i) | i ∈ ℤ, 0 ≤ i ≤ 4 }
- B = { 3i | i ∈ {1, 2, 4, 5} }

and asks you to evaluate (a) A ∩ B, (b) |A ∪ B| and (c) |A − B|.

## Your task

This exercise turns that problem into code and keeps going: you write the three set operations, make them fast, and finish with a small search index. A set is always a **strictly increasing** sequence, and the element type is only guaranteed to support `<`.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Set-builder notation as code

```cpp
std::uint64_t binomial(std::uint64_t n, std::uint64_t k);

template <typename Domain, typename F>
std::vector<std::decay_t<std::invoke_result_t<F&, std::ranges::range_reference_t<const Domain>>>>
set_of(const Domain& domain, F f);
```

- `binomial(n, k)`: the binomial coefficient C(n, k). Return 0 when `k > n`. The result is guaranteed to fit in `uint64_t`, but intermediate values may not (try `binomial(67, 33)`).
- `set_of(domain, f)`: return `{ f(x) | x ∈ domain }`, sorted with duplicates removed. `domain` can be any range, and the result holds whatever type `f` returns.

Once both work, use them to build A and B from the original problem, and check your three answers against the official solution linked at the end.

## Stage 2 — Linear-time set operations

```cpp
template <typename T>
std::vector<T> intersect(std::span<const T> a, std::span<const T> b);

template <typename T>
std::size_t union_size(std::span<const T> a, std::span<const T> b);

template <typename T>
std::size_t difference_size(std::span<const T> a, std::span<const T> b);
```

`a` and `b` are both strictly increasing `std::span`s. Every operation must make **O(|a| + |b|)** comparisons in a single pass, with no nested loops:

- `intersect<T>(a, b)`: `a ∩ b`, strictly increasing.
- `union_size<T>(a, b)`: `|a ∪ b|`, without actually building the union.
- `difference_size<T>(a, b)`: `|a − b|`.

Edge cases: either side is empty, both sides are identical, and the two sets are disjoint.

## Stage 3 — Don't walk every element when the sizes are lopsided

```cpp
template <typename T>
std::vector<T> gallop_intersect(std::span<const T> a, std::span<const T> b);
```

`gallop_intersect<T>(a, b)`: same behavior as `intersect`, but it must make **O(m · log(n/m + 1))** comparisons, where `m` is the size of the smaller set and `n` the size of the larger one. This must hold whichever side is larger.

Hint: for each element of the small set, jump forward through the large set **from where the previous search stopped**, doubling the distance each time (1, 2, 4, …). Once you overshoot, finish with a binary search. This is called galloping, or exponential search, and 0213 uses it again.

## Stage 4 — Real-world extension: multi-term queries

```cpp
class PostingIndex {
public:
    void add(std::string term, std::vector<std::int64_t> doc_ids);
    std::vector<std::int64_t> query(std::span<const std::string> must,
                                    std::span<const std::string> must_not = {}) const;
};
```

`PostingIndex`:

- `add(term, doc_ids)`: `doc_ids` is strictly increasing. Calling `add` again with the same term replaces its old list.
- `query(must, must_not)`: return the IDs of the documents that are in the list of every `must` term and in no `must_not` term's list, strictly increasing.
  - If `must` is empty or contains an unknown term, return an empty set. Ignore unknown `must_not` terms.

In a typical AND query, one term is much rarer than the others. The query must walk only the shortest list and gallop through the rest. Even with many lists of length n, it must not degrade into a linear scan (a `PERF` test checks this).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. When is `gallop_intersect` slower than a plain merge? In a real system, how would you pick an algorithm based on the ratio of the two sizes?
2. Why does doing the multiplications and divisions in `binomial` in the right order guarantee that every intermediate result is an integer?
3. If a posting list doesn't fit in memory, or has to be compressed (delta + varint), does galloping still work? What role does a skip list play here?

---

**Official solution:** [Problem Set 0 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/cfc7d2d5bc3b9f4df35b5bdc7aa49284_MIT6_006S20_ps0-solutions.pdf). Read it after you finish.
