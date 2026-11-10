---
title: Parallel Floyd-Warshall and Transitive Closure
unit: 06 · Work/Span Theory (HW5)
source: 6.172 HW5 Write-up 6 (CLRS 27.2-6)
pdf: materials/6.172/homework/hw05/handout.pdf
read: materials/6.172/lectures/lec08.pdf
kind: code
sanitizer: tsan
minutes: 110
---

# 0616 · Parallel Floyd-Warshall and Transitive Closure

**Original problem:** [HW5: Theory of Performance Engineering](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/cd57dbdecbce3bd4da39d620c1e61d14_MIT6_172F18hw5.pdf) · **Read first:** [Analysis of Multithreaded Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/5655e50541f85421cc47b7c85d8e5de3_MIT6_172F18_lec8.pdf)

> **Why this comes up in practice:** all-pairs distances in route planning and the transitive closure of module dependencies ("which files must be rebuilt if I change this header?") are both this triple loop. The hard part isn't the analysis; it's seeing "what won't change in this round," which is exactly what makes it parallelizable.

## Original problem

6.172 HW5 Write-up 6 is CLRS Exercise 27.2-6: give an efficient multithreaded implementation of the Floyd-Warshall algorithm (CLRS §25.2), which computes the shortest-path distances between all pairs of vertices in an edge-weighted directed graph, and analyze it. Floyd-Warshall runs one round per vertex k; in round k, every pair (i, j) takes the route through k whenever d[i][k] + d[k][j] is shorter than its current d[i][j]. HW5 is a written problem set, so the original asks for pseudocode and an analysis of the work, span, and parallelism, not a program.

## Your task

As before, use `template <typename Exec>` (`_shared/01_exec.hpp`): `exec::Meter` computes work/span, and with `exec::ForkJoin`, TSan guards against races. `dist` is an `n×n` row-major array; a cell with no edge holds `kNoEdge`, and the diagonal is 0.

Beyond the parallel algorithm that the exercise asks for, you also keep next-hop pointers so that paths can be rebuilt, and you finish with a bitset transitive closure that has to be fast.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Parallelizing the triple loop

```cpp
inline constexpr std::int64_t kNoEdge = std::numeric_limits<std::int64_t>::max() / 4;

template <typename Exec>
void floyd_warshall(Exec& exec, std::span<std::int64_t> dist, std::size_t n);
```

`floyd_warshall(exec, dist, n)`: compute all-pairs shortest distances in place. There may be negative edges but no negative cycles; skip any addition that involves `kNoEdge` (don't let it overflow or turn into something that "looks reachable").

The outermost loop over `k` must stay serial (round `k` uses the results of round `k-1`). The inner two loops must use `parallel_for`. The key observation: during round `k`, row `k` and column `k` **don't change** (`d[k][k] = 0`), so reading `d[i][k]` and `d[k][j]` while writing `d[i][j]` is safe, provided you really **don't write** the cells where `i == k` or `j == k`. Otherwise TSan catches it on the spot (even if the value written back is the same, it's still a data race).

Accounting: `exec.charge(1)` for every `(i, j)` considered with `i != k` and `j != k` (whether or not it gets updated), so the work is exactly `n(n-1)²` plus the number of forks. The span must be `O(n lg n)`: at most `n·(2⌈lg n⌉ + 6)`.

## Stage 2 — Paths

```cpp
template <typename Exec>
void floyd_warshall_paths(Exec& exec, std::span<std::int64_t> dist, std::span<std::int64_t> next,
                          std::size_t n);

std::vector<std::size_t> reconstruct_path(std::span<const std::int64_t> next, std::size_t n,
                                          std::size_t s, std::size_t t);

bool has_negative_cycle(std::span<const std::int64_t> dist, std::size_t n);
```

`floyd_warshall_paths(exec, dist, next, n)`: `next` has the same size, and the function initializes it from `dist` itself (`next[i][j] = j` where there's an edge, `-1` otherwise; `-1` on the diagonal). Afterwards, whenever you update `d[i][j]`, also set `next[i][j]` to `next[i][k]`, and again don't write the cells where `i == k` or `j == k`.

- `reconstruct_path(next, n, s, t)`: return the sequence of vertices from `s` to `t` (both ends included); return an empty sequence when `t` is unreachable, and `{s}` when `s == t`.
- `has_negative_cycle(dist, n)`: call it after `floyd_warshall`; any negative diagonal entry means there's a negative cycle. (For this check the tests do run `floyd_warshall` on a graph that has a negative cycle.)

## Stage 3 — Real-world extension: Bitset transitive closure

```cpp
template <typename Exec>
void transitive_closure(Exec& exec, std::span<std::uint64_t> rows, std::size_t n);
```

`transitive_closure(exec, rows, n)`: `rows` is a bit matrix of `n` rows, each made of `words = (n + 63) / 64` `uint64_t` words, where bit `j % 64` of `rows[i * words + (j / 64)]` means `i → j`. Turn it into the transitive closure (`i` can reach `j` along at least one edge). In round `k`, you only need to OR row `k` into "every other row whose bit `k` is 1"; the inner loop can run in parallel (again, don't touch row `k`).

Bitsets are tens of times faster than a cell-by-cell boolean array: the test requires at least a **10×** speedup over a cell-by-cell baseline in the same executable, measured with a single worker.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Give the work and span of Stage 1, and explain why `k` can't be parallel (HW5 Write-up 6). If the inner two `(i, j)` loops went back to being serial, what would the span become?
2. Why is the `d[k][k] = 0` property from Stage 1 a necessary precondition? What happens when there's a negative cycle?
3. In the bitset version's complexity `O(n³/w)`, what does `w` stand for? When should a sparse input graph use `n` runs of BFS instead?
