---
title: Quicksort Analysis (checking high-probability bounds by experiment)
unit: 04 · Amortization and Randomization (PS4)
source: 6.046J PS4 Problem 4-2
pdf: materials/6.046j/psets/ps04/questions.pdf
answer: materials/6.046j/psets/ps04/solutions.pdf
read: materials/6.046j/lectures/lec06.pdf, materials/6.046j/recitations/rec04.pdf
kind: code
minutes: 100
---

# 0412 · Quicksort Analysis

**Original problem:** [Problem Set 4](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/5eb7c1134350c89889dcaf7ec3fd609a_MIT6_046JS15_pset4.pdf) · **Read first:** [Randomization: Matrix Multiply, Quicksort](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/cb55cb123a557eed0738a1187a452c24_MIT6_046JS15_lec06.pdf), [Randomization: Randomized Median](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/08020963ae819d4c6dcf177983bb7c6b_MIT6_046JS15_Recitation4.pdf)

> **Why this comes up in practice:** "O(n log n) on average" isn't reassuring enough, and online systems care more about the tail: how likely is it that the cost exceeds some budget? This exercise turns a high-probability bound into an experiment you can measure, and then uses it to derive the safeguard in introsort.

## Original problem

6.046J PS4 Problem 4-2 ("Quicksort Analysis") bounds the running time of QUICKSORT with failure probabilities rather than expectations. The array to sort is A[1..n], x_i denotes the element that starts out in A[i], and all the x_i are distinct. The problem builds on a claim from lecture: for any real constant c > 1 and any positive integer α, 3(α + c) lg n flips of a fair coin come up heads at least c lg n times with probability at least 1 − 1/n^α.

- (a) Fix an element x_i and a recursive call on a subarray A[p..p+m−1] of size m ≥ 2 that contains it. Prove that with probability at least 1/2, either this call picks x_i as its pivot, or the next recursive call that contains x_i works on a subarray of size at most (3/4)m.
- (b) Prove that with probability at least 1 − 1/n², x_i is compared with pivots at most d lg n times in total, for a constant d that you state explicitly.
- (c) Use part (b), together with the union bound, to prove that with probability at least 1 − 1/n, QUICKSORT makes at most d′ n lg n comparisons on the whole input, for an explicit constant d′.
- (d) Extend the argument so that, for every positive integer α, the number of comparisons obeys a bound with probability 1 − 1/n^α, rather than only for α = 1.

## Your task

The original problem is a proof; this exercise turns each step of the argument into a check you can run. The proofs themselves move to the interview follow-ups.

Stage 1 checks the lemma of part (a), Stage 2 instruments quicksort so that the comparisons in parts (b) and (c) can be counted, Stage 3 measures the bounds of part (d), and Stage 4 adds a safeguard that the original doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Good pivots

```cpp
std::size_t good_pivot_count(std::size_t m, std::size_t rank);
```

`good_pivot_count(m, rank)`: in a subarray of size `m`, element `x` has rank `rank` (0-based), and the pivot lands uniformly on one of the `m` positions. Return how many choices of pivot are "good": the pivot is `x` itself, or the subarray that contains `x` next has size `≤ 3m/4` (compare with integers, `4·size ≤ 3m`; don't use floating point).

The tests enumerate every `m ≤ 800` and every rank to verify the lemma that "at least half of the pivots are good".

## Stage 2 — Quicksort with a trace

```cpp
struct QuicksortTrace {
    std::vector<std::uint32_t> pivot_comparisons;
    std::uint64_t total_comparisons = 0;
};

template <typename T, std::uniform_random_bit_generator Rng>
QuicksortTrace traced_quicksort(std::span<T> values, Rng& rng);
```

`traced_quicksort(values, rng)`: the elements of `values` are distinct. Sort them and return a `QuicksortTrace`:

- `pivot_comparisons[i]`: how many times in total the element whose **starting position** is `i` was compared with a pivot (the call in which it is the pivot itself doesn't count). Elements move around during sorting, so remember where each one started.
- `total_comparisons`: the total number of comparisons, which equals the sum of the above.

Rules: in each partition the pivot is chosen uniformly at random, and every other element is **compared with the pivot exactly once**. The tests use `Counted` to confirm that `total_comparisons` equals the real number of comparisons exactly, and they also check that each element's average number of comparisons matches `H_i + H_{n+1-i} - 2`.

## Stage 3 — Measuring the high-probability bound

```cpp
struct BoundReport {
    int trials = 0;
    int element_violations = 0;
    int total_violations = 0;
    double mean_total = 0.0;
};

double element_bound(double n, int alpha);
double total_bound(double n, int alpha);

template <std::uniform_random_bit_generator Rng>
BoundReport measure_bound(std::size_t n, int trials, int alpha, Rng& rng);
```

Let `c = log_{4/3} 2`.

- `element_bound(n, α)`: `3(α + 1 + c) · lg n`. A single element is compared at most this many times with probability at least `1 − n^{−(α+1)}`.
- `total_bound(n, α)`: `n · element_bound(n, α)`. The total number of comparisons is at most this value with probability at least `1 − n^{−α}` (by the union bound).

Note: the official solution's `3(α + c)` is off by a `+1` in the union bound. Follow the version above here; the reason why belongs in the interview follow-ups.

`measure_bound(n, trials, alpha, rng)`: run `trials` times (with a fresh shuffle each time) and return a `BoundReport`: how many runs there were, how many exceeded the element bound, how many exceeded the total bound, and the mean total number of comparisons.

## Stage 4 — Real-world extension: introsort's safeguard

```cpp
template <typename T, std::uniform_random_bit_generator Rng>
void guarded_sort(std::span<T> values, Rng& rng);
```

If the random number generator breaks, or an adversary can predict the pivots, Stage 2 becomes Θ(n²). `guarded_sort(values, rng)`:

- Check the depth before partitioning; once the recursion depth exceeds `2⌈lg n⌉`, switch to heapsort for that range (`std::make_heap` and `std::sort_heap` are enough).
- `values` may contain duplicates. You don't need to keep a trace.
- Even if `rng` always returns 0, the number of comparisons must be `O(n log n)`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Prove the Stage 1 lemma: when the pivot lands in the middle range, why does neither side exceed `3m/4`?
2. Derive the element bound from the lemma: what is a "successful call"? Why do you need `c lg n` successes? Why does that take `3(α + 1 + c) lg n` coin flips?
3. Why is the official solution missing the `+1`? Write out every step of the union bound.
4. Why choose `2 lg n` as the depth limit? What happens if you choose a larger or a smaller one? (Compare with `std::sort`.)

---

**Official solution:** [Solutions to Problem Set 4](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/8cc8292f1b04e9adc43546295b2ecae3_MIT6_046JS15_pset4sols.pdf). Read it after you finish.
