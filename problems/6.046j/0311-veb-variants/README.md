---
title: Two vEB Variants (policy-based design)
unit: 03 · van Emde Boas (PS3)
source: 6.046J PS3 Problem 3-1
pdf: materials/6.046j/psets/ps03/questions.pdf
answer: materials/6.046j/psets/ps03/solutions.pdf
read: materials/6.046j/lectures/lec04.pdf
kind: code
minutes: 150
---

# 0311 · Two vEB Variants

**Original problem:** [Problem Set 3](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/c6f512681e64389b446a981c7d963323_MIT6_046JS15_pset3.pdf) · **Read first:** [Divide & Conquer: Van Emde Boas Trees](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/49c8fa24dffce58052c90d46ac800387_MIT6_046JS15_lec04.pdf)

> **Why this comes up in practice:** you often need to try several splitting strategies and trade-offs for the same data structure. Pulling the part that varies out into a policy, which the compiler swaps in at zero cost, is cleaner than copying the code or adding runtime branches.

## Original problem

6.046J PS3 Problem 3-1 ("Variants on van Emde Boas") takes the vEB structure from Lecture 4 (also in CLRS Section 20.3) and changes it in two ways. For each variant, explain how the pseudocode (from the lecture or the textbook) must change, and work out what INSERT, DELETE, and SUCCESSOR now cost relative to the original structure.

- (a) Use u^{1/3} clusters of size u^{2/3}, instead of u^{1/2} clusters of size u^{1/2}.
- (b) With the original u^{1/2} × u^{1/2} split, store the maximum, like the minimum, only in the node's existing max field and never in a recursive substructure.

## Your task

Rewrite the structure from [`0301`](../0301-van-emde-boas/README.md) as `template <typename Split, bool ExcludeMax> class Veb`. The interface is the same as in 0301 (`size`, `empty`, `min`, `max`, `contains`, `insert`, `erase`, `successor`, `predecessor`, `node_count`, `visits`), and so are the range rules.

`Split::high_bits(bits)` decides how many high bits one level splits off (for `bits ≥ 2` it returns a value in `[1, bits-1]`); the low bits are the rest. `visits()` is defined as in 0301, and each operation enters at most `2·L + 2` nodes, where `L` is the number of levels when the recursion always follows the larger part.

The goal of this exercise is to **rewrite it as one template**, not to write three copies inside `Veb`. Use `if constexpr (ExcludeMax)` to separate the cases.

Stage 1 is part (a), Stage 2 is part (b), and Stage 3 combines them. Instead of describing the pseudocode changes and analyzing them on paper, you implement both variants, and the tests measure the costs through `visits()` and `node_count()`.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Splitting into thirds

```cpp
// How many high-order bits become the cluster number when a level splits `bits` bits.
struct SqrtSplit {
    static constexpr int high_bits(int bits) { return bits - bits / 2; }  // provided
};

struct CubeRootSplit {
    static constexpr int high_bits(int bits) { return std::max(1, bits / 3); }  // provided
};

template <typename Split, bool ExcludeMax>
class Veb {
public:
    explicit Veb(int universe_bits);

    std::size_t size() const;
    bool empty() const;
    std::optional<std::uint64_t> min() const;
    std::optional<std::uint64_t> max() const;
    bool contains(std::uint64_t x) const;
    bool insert(std::uint64_t x);
    bool erase(std::uint64_t x);
    std::optional<std::uint64_t> successor(std::uint64_t x) const;
    std::optional<std::uint64_t> predecessor(std::uint64_t x) const;
    std::size_t node_count() const;

    // Cost model used by the tests: the number of vEB nodes entered so far.
    std::uint64_t visits() const;
};
```

`Veb<CubeRootSplit, false>`: `u^{1/3}` clusters, each of size `u^{2/3}`. As in 0301, a summary or cluster exists only while it holds something: a single key needs only the root (`node_count() == 1`), and a second key already adds a summary and a cluster (`node_count() == 3`).

`CubeRootSplit::high_bits` already handles `bits == 2` for you; otherwise the high part would have 0 bits and the recursion would never end. The tests also confirm that the recursion really is deeper than with the original `SqrtSplit`, that is, that you really split according to the policy.

## Stage 2 — Keeping the maximum out of the clusters too

`Veb<SqrtSplit, true>`: besides `min`, `max` is also stored only on the node and never in any cluster.

- With one element, `min == max`; with exactly two elements, every cluster is empty and `node_count() == 1`. **Only the third element enters a cluster.**
- When `insert` gets a key larger than `max`, the old `max` has to be pushed down into a cluster, symmetric to `min`.
- When `successor` finds no next cluster, the answer may be `max`; this is the step people miss most often. `predecessor` is symmetric.
- `erase` must handle the special cases "exactly two elements" and "empty summary", and when it deletes `max`, it must pull the new `max` up from the last cluster.

## Stage 3 — Both variants together, and any policy

`Veb<CubeRootSplit, true>` must work, and so must any policy that follows the `high_bits` contract (the tests define their own policy that splits off a fixed 4 high bits). Don't assume `Split` comes in only these two kinds, and don't write the "max stays out of the clusters" logic in a way that holds for only one particular policy.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. The original problem asks how the complexity of `INSERT`, `DELETE`, and `SUCCESSOR` changes. For the `u^{1/3}` split, what is the recurrence? Starting from `log₂ log₂ u`, what does the number of levels become? How much does the constant change?
2. Excluding `max` as well doesn't change the asymptotic complexity at all, so what do you gain? Which wins: the nodes you save, or the extra branches you write?
3. What are the trade-offs between `if constexpr` and virtual functions (the strategy pattern)? Why is the former the better fit here?

---

**Official solution:** [Solutions to Problem Set 3](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/3e83fa6454e5ebe6bf1bcb1b4a122f6d_MIT6_046JS15_pset3sols.pdf). Read it after you finish.
