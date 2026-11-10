---
title: Matrix Multiply and Freivalds Verification
unit: 04 · Amortization and Randomization (PS4)
source: 6.046J Recitation 1, Lecture 6
pdf: materials/6.046j/lectures/lec06.pdf
read: materials/6.046j/recitations/rec01.pdf, materials/6.046j/lectures/lec06.pdf
kind: code
minutes: 90
---

# 0404 · Matrix Multiply and Freivalds Verification

**Based on:** [Randomization: Matrix Multiply, Quicksort](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/cb55cb123a557eed0738a1187a452c24_MIT6_046JS15_lec06.pdf) · **Read first:** [Divide & Conquer: Smarter Interval Scheduling, Master Theorem, Strassen’s Algorithm](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/7c15f31b5a1aa1991313d1b4544cc137_MIT6_046JS15_Recitation1.pdf), [Randomization: Matrix Multiply, Quicksort](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/cb55cb123a557eed0738a1187a452c24_MIT6_046JS15_lec06.pdf)

> **Why this comes up in practice:** after you hand an expensive matrix computation to a GPU or an untrusted worker, how do you cheaply confirm that hardware faults or bugs didn't corrupt the result? Computing it again defeats the purpose; Freivalds checks O(n³) work in O(n²).

## Your task

`Matrix<Ring>` is an `n × n` square matrix that stores its elements with value semantics. `Ring` is a ring that supports `+`, `-`, `*`, and `==`: `Ring{}` is 0 and `Ring(1)` is 1. The tests mostly use `std::uint64_t` (which naturally works modulo 2⁶⁴).

You build the matrix type with the cubic product, then Strassen's algorithm, and finally Freivalds' randomized check of a claimed product.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Matrix and the direct product

```cpp
template <typename Ring>
class Matrix {
public:
    explicit Matrix(std::size_t n);

    std::size_t size() const;
    Ring& operator()(std::size_t i, std::size_t j);
    const Ring& operator()(std::size_t i, std::size_t j) const;

    friend bool operator==(const Matrix&, const Matrix&);
};

template <typename Ring>
Matrix<Ring> multiply_naive(const Matrix<Ring>& a, const Matrix<Ring>& b);
```

`Matrix(n)` creates an all-0 matrix (`n` may be 0), with `size()`, `m(i, j)` (0-based), and `operator==`, which returns `false` for matrices of different sizes instead of throwing. `multiply_naive(a, b)` performs n³ multiplications. Throw `std::invalid_argument` when the sizes don't match.

## Stage 2 — Strassen

```cpp
template <typename Ring>
Matrix<Ring> multiply_strassen(const Matrix<Ring>& a, const Matrix<Ring>& b, std::size_t cutoff = 64);
```

`multiply_strassen(a, b, cutoff)`: switch to `multiply_naive` when `n ≤ cutoff`. When `n` is not a power of 2, pad with 0s up to the next power of 2, and trim the result back afterwards. Throw `std::invalid_argument` when `cutoff == 0` or when the sizes don't match.

- Each level of recursion may make only 7 subproblem multiplications; that is, with `n = 2^k` and `cutoff = 1`, the total number of multiplications is **exactly** `7^k`.
- The next level's matrices are smaller than this level's: Strassen's seven subproblems must recurse on submatrices, rather than computing 8 quarter-size products yourself.

## Stage 3 — Freivalds verification

```cpp
template <typename Ring, std::uniform_random_bit_generator Rng>
bool verify_product(const Matrix<Ring>& a, const Matrix<Ring>& b, const Matrix<Ring>& c, Rng& rng,
                    int rounds = 32);
```

`verify_product(a, b, c, rng, rounds)`: decide whether `a × b` equals `c`. Throw `std::invalid_argument` unless all three matrices have the same size.

- If the equation holds, **always** return `true`.
- If it doesn't, each round returns `false` with probability at least 1/2, so the probability of returning `true` is at most `2^{-rounds}`.
- Each round draws a random vector `r ∈ {0,1}^n` and compares `A(Br)` with `Cr`. Each round may do only 3 matrix-vector products, for a total of `≤ 3·rounds·n²` multiplications.

Lecture 6 does the analysis mod 2, while this exercise uses the wrap-around of `std::uint64_t`. The analysis doesn't need to change, because it only uses the fact that `Dr` and `D(r + e_j)` can't both be 0.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why do 7 recursive calls beat 8? How do you solve `T(n) = 7T(n/2) + Θ(n²)`? Why does every practical implementation set a cutoff?
2. Is Freivalds Monte Carlo or Las Vegas? If each component of `r` were drawn uniformly from a larger range, how would the error probability change?
3. Where else does this "verifying is cheaper than computing" idea apply? (Think of Schwartz–Zippel, or the checks you saw in 0201.)
