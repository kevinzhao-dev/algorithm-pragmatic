---
title: Loop Order in Matrix Multiplication
unit: 01 · C++ Fundamentals and Tools (HW1)
source: 6.172 HW1 §6 (Write-up 9–10), Lecture 1
pdf: materials/6.172/homework/hw01/handout.pdf
read: materials/6.172/lectures/lec01.pdf
kind: perf
minutes: 90
---

# 0113 · Loop Order in Matrix Multiplication

**Original problem:** [HW1: Basic Tools, C Primer](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/2724d8594cb413754669fc4e9c6ce7db_MIT6_172F18hw1.pdf) · **Read first:** [Introduction & Matrix Multiplication](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/d0c73dd51c79b95196a2e6faa824e1b4_MIT6_172F18_lec1.pdf)

> **Why this comes up in practice:** in the innermost loops of linear algebra, image processing, and embedding comparison, changing the nesting order without touching the algorithm can make the code ten times faster. It's the example the first lecture uses to shock you, and the one interviewers most often ask "why" about.

## Original problem

6.172 HW1 §6 times the C matrix-multiply program from §5. Timings on the shared course machines, or on a small VM that is also running your editor, are noisy, so the handout has you queue performance runs on a dedicated machine with the course's `awsrun` utility. Then:

- Grow all the matrices to 1000 × 1000 and time the program as it is. Its innermost loop reads `A` in order but strides across `B`.
- Exchange the `j` and `k` loops, which makes the innermost loop read consecutive elements of a row of `B` and update consecutive elements of a row of `C`; time it again and confirm the speedup.
- Time a debug build (`-O0`) and an optimized build (`-O3`), and confirm that the optimized one is faster.

The write-ups:

- **Write-up 9:** the execution times before and after the loop swap.
- **Write-up 10:** the execution times of the `-O0` debug build and of the `-O3` build.

Lecture 1 tells the same story at a larger scale: it multiplies 4096 × 4096 matrices, times all six loop orders (the order alone changes the running time by a factor of 18), explains the gap with cache lines and the row-major layout, and then measures Clang's optimization levels.

## Your task

This exercise rebuilds that experiment in C++: you write an RAII matrix, run the multiplication in all six loop orders, rank the orders with a small cache model, and let perf tests check the speedups inside a single executable instead of on a dedicated timing machine.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — RAII matrix

```cpp
class Matrix {
public:
    Matrix(std::size_t rows, std::size_t cols);

    std::size_t rows() const;
    std::size_t cols() const;

    double& operator()(std::size_t r, std::size_t c);
    double operator()(std::size_t r, std::size_t c) const;

    std::span<double> row(std::size_t r);
    std::span<const double> row(std::size_t r) const;

    friend bool operator==(const Matrix& a, const Matrix& b);
};

Matrix multiply(const Matrix& a, const Matrix& b);
```

`Matrix(rows, cols)` holds `double` elements stored contiguously in row-major order, and **sets every element to zero on construction** (one of HW1's bugs is a forgotten initialization). `operator()(r, c)` accesses an element, `row(r)` returns that row as a `std::span`, and `operator==` compares the shapes and the contents.

`multiply(a, b)`: return `C = A × B` (`a.rows()` × `b.cols()`). Throw `std::invalid_argument` when the dimensions don't match (`a.cols() != b.rows()`). Matrices with a dimension of 0 must work too.

## Stage 2 — Six loop orders

```cpp
enum class LoopOrder { IJK, IKJ, JIK, JKI, KIJ, KJI };

template <typename Body>
void for_each_index_triple(std::size_t rows, std::size_t cols, std::size_t depth,
                           LoopOrder order, Body&& body);   // body(i, j, k)

Matrix multiply_ordered(const Matrix& a, const Matrix& b, LoopOrder order);
```

`for_each_index_triple` calls `body(i, j, k)` in the nesting order that `order` names, with `i ∈ [0, rows)`, `j ∈ [0, cols)`, and `k ∈ [0, depth)`. For example, `IKJ` means `for i, for k, for j`.

Implement `multiply_ordered` with this function. All six orders must give the same result (the three extents correspond to `a.rows()`, `b.cols()`, and `a.cols()`). Like `multiply`, it throws `std::invalid_argument` when the dimensions don't match.

## Stage 3 — Explaining the difference with a cache model

```cpp
std::vector<LoopOrder> rank_by_cache_misses(std::size_t n, std::size_t line_bytes,
                                            std::size_t sets, std::size_t ways);
```

`std::vector<LoopOrder> rank_by_cache_misses(n, line_bytes, sets, ways)`: for an `n × n × n` multiplication, feed the address sequence into a `MiniCache` (`support/mini_cache.hpp`) and return the six orders **from the fewest misses to the most**.

Address model: a `double` takes 8 bytes; `A` starts at address 0, `B` at `1<<30`, and `C` at `2<<30`, all in row-major order. Each execution of the innermost `C[i][j] += A[i][k] * B[k][j]` accesses `C[i][j]`, `A[i][k]`, `B[k][j]`, and `C[i][j]`, in that order.

Once it runs, think it through: what do the two fastest orders have in common? What about the two slowest?

## Stage 4 — Measuring real time

The tests time `multiply_ordered` with `n = 512` and check that:

- `IKJ` is at least 4 times faster than `IJK`.
- `IKJ` is at least 4 times faster than the same logic with optimization turned off (the effect of `-O0`, reproduced inside the same executable by an `[[clang::optnone]]` baseline). This corresponds to HW1 Write-up 10.

Try passing `body` as a `std::function` yourself, and see how far the ratio drops (why?).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is IKJ so much faster than IJK? Is the cache alone enough to explain it? (Hint: can the innermost loop be vectorized?)
2. Stage 3's model works at the address level and has no hardware prefetcher. On a real machine, does JKI do worse or better than the model predicts?
3. If `n` grows so large that the matrices don't fit in L2, what would you do next? (Hint: blocking/cache-oblivious.)
