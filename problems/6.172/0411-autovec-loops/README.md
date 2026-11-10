---
title: Getting the Compiler to Vectorize for You
unit: 04 · Vectorization (HW3)
source: 6.172 HW3 (§2 Example 1–4, §3.1–3.2, Write-up 1–15), Recitation R3
pdf: materials/6.172/homework/hw03/handout.pdf
answer: materials/6.172/recitations/r3/solutions.pdf
read: materials/6.172/homework/hw03/handout.pdf, materials/6.172/recitations/r3/problems.pdf
kind: perf
minutes: 180
---

# 0411 · Getting the Compiler to Vectorize for You

**Original problem:** [HW3: Vectorization](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/072651a8229a63376d5720c9a500ae45_MIT6_172F18hw3.pdf) · **Read first:** [HW3: Vectorization](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/072651a8229a63376d5720c9a500ae45_MIT6_172F18hw3.pdf), [R3: Cilk and Reducers Problems](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/35d02145726032b5031045935a9f9659_MIT6_172F18_rec3.pdf)

> **Why this comes up in practice:** embedding comparison, image processing, and column-store scans almost all apply the same operation to a long run of numbers. The compiler will turn that into NEON for you, but if the loop's shape is even slightly off, it quietly gives up. This exercise practices "how to get the compiler to do this for you, and how to confirm that it really did."

## Original problem

6.172 HW3 is a lab on Clang's auto-vectorizer for x86 (SSE and AVX2). It works through small C files, Makefile switches (`VECTORIZE=1`, `AVX2=1`, `ASSEMBLE=1`), and timings on the course's `awsrun` machines.

§2 reads the vectorizer's report and the generated assembly for four tiny loops over 2^16 elements:

- Example 1 adds one `uint8_t` array into another, element by element. Clang vectorizes it but guards the vector loop with a runtime overlap check. Declaring the pointers `restrict` removes the check, `__builtin_assume_aligned` turns the unaligned moves into aligned ones, and then the example is rebuilt with AVX2.
- Example 2 takes the elementwise max of two arrays, written as an `if` that stores conditionally. It doesn't vectorize well until it's rewritten as an unconditional assignment of a conditional expression, which produces `pmaxub`.
- Example 3 copies each element from the next position of the other array, and its assembly uses no vector registers.
- Example 4 sums an array of `double`s. It isn't vectorized, because floating-point addition isn't associative and Clang may not reorder it, until `-ffast-math` is added.

§3.1 times a data-parallel loop in `loop.c` that combines two `uint32_t` arrays elementwise into a third (addition by default, with a fixed `N = 1024` and an outer loop that repeats it for timing): unvectorized, vectorized, and with AVX2. It then covers the report flags (`-Rpass=loop-vectorize`, `-Rpass-missed=loop-vectorize`, `-Rpass-analysis=loop-vectorize`), reading the assembly, other operators through the `__OP__` macro, other element types through `__TYPE__`, and multiplication on `uint64_t`. §3.2 covers common patterns: a loop bound known only at run time, a stride of 2, and a sum reduction.

The write-ups:

- **Write-up 1:** why Example 1's assembly starts its index at −2^16 and counts up, instead of starting at 0 with small positive offsets.
- **Write-up 2:** the AVX2 build of Example 1 still uses unaligned moves; fix the code so that it uses aligned ones.
- **Write-up 3:** a theory for why the two versions of Example 2 compile to such different assembly.
- **Write-up 4:** why Example 3's assembly has no vector instructions, and whether vectorizing it would make it faster.
- **Write-up 5:** confirm that Example 4 vectorizes with `-ffast-math`, and explain why the program prints a different result with and without the flag.
- **Write-up 6:** the speedup from vectorization and the extra speedup from AVX2, and what they imply about the bit width of the default and of the AVX2 vector registers.
- **Write-up 7:** the instruction that performs the vector add, with and without `AVX2=1`.
- **Write-up 8:** after trying other operators, which ones don't vectorize with AVX2, and how the assembly for `<<` without AVX2 differs from the AVX2 version.
- **Write-up 9:** the speedups for `uint64_t`, `uint32_t`, `uint16_t`, and `uint8_t` elements.
- **Write-up 10:** what happens to the AVX2 speedup for `*` on `uint64_t`, and on a narrower type such as `uint8_t`.
- **Write-up 11:** where perf says the time goes in the AVX2 `uint64_t` multiply, and how the vector add's share of the time compares with the vector multiply's.
- **Write-up 12:** with `N` read from the command line, how the speedups change compared with `N = 1024`, and why.
- **Write-up 13:** whether Clang vectorizes the loop with a stride of 2, and why it might choose not to.
- **Write-up 14:** force it with Clang's `vectorize` loop pragma, report the speedups, what `vectorize_width` of 2 does, and the best pragma setting you find.
- **Write-up 15:** how the compiler vectorizes the sum reduction, explained from the assembly.

Recitation R3 adds two questions:

- (1) A loop stores `k` times each element of an array `B` into an array `A` (2^30 elements; the values are nonnegative, the two arrays don't alias, and `k` is known only at run time). Ben rewrites it with pointers and a sentinel: he stores a negative value in the last element of `A`, loops while the current element of `A` is nonnegative, and handles the last element after the loop. Under GCC `-O3`, the new version is several times slower because it no longer vectorizes. Why?
- (2) Can a function that computes the row sums of a row-major 2048 × 3072 `int` matrix, one row at a time, use the vector hardware effectively? If so, where and how; if not, why not?

## Your task

Every stage compares your function with a version in the same executable that is forced to stay scalar with `#pragma clang loop vectorize(disable) interleave(disable)` (Release, `-O2`).

**Read the compiler's decisions** (NEON replaces AVX2, with 128-bit width): `practice.cmake` already turns on `-Rpass=loop-vectorize` and the related reports. After you've run the tests once:

```bash
touch problems/6.172/0411-autovec-loops/tests/test.cpp
cmake --build build/release --target t_6_172_0411_autovec_loops 2>&1 | grep -E 'remark|error'
```

Read the remark for each loop in `solution.hpp`, then look at the actual instructions with `-S` or `sample`. **A remark that says vectorized doesn't mean the vectorization is effective** (you'll run into this in Stage 3).

The functions are already marked `[[gnu::noinline]]`. Don't remove it, or inlining will distort the comparison.

Instead of reading x86 assembly for the handout's C files, you write each loop so that Clang vectorizes it well for NEON, and perf tests check its speedup over the scalar version.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Aliasing (Example 1)

```cpp
[[gnu::noinline]] inline void add_bytes(std::uint8_t* a, const std::uint8_t* b, std::size_t n);
```

`add_bytes(a, b, n)`: `a[i] += b[i]`. At least **6×** faster than scalar. `a` and `b` may overlap, and **the semantics must match the sequential loop exactly** (when `a` comes after `b`, later elements must see the results written to earlier ones). The compiler generates a runtime overlap check, so you don't need C's `restrict`; C++ has no standard `restrict`, and on this machine `__restrict` doesn't necessarily get rid of that check either (look with `-S`).

## Stage 2 — Replacing a conditional store with a select (Example 2)

```cpp
[[gnu::noinline]] inline void max_bytes(std::uint8_t* a, const std::uint8_t* b, std::size_t n);
```

`max_bytes(a, b, n)`: `a[i] = max(a[i], b[i])`. At least **6×** faster than scalar. Writing it as `if (b[i] > a[i]) a[i] = b[i];` fails because of the "conditional store" (think about whether NEON has a masked store); rewrite it as an unconditional select.

## Stage 3 — Floating-point reduction (Example 4)

```cpp
[[gnu::noinline]] inline double sum_doubles(const double* x, std::size_t n);
[[gnu::noinline]] inline float dot_floats(const float* a, const float* b, std::size_t n);
```

- `sum_doubles(x, n)` must be at least **3×** faster than scalar, and `dot_floats(a, b, n)` at least **4×**.
- Floating-point addition **is not associative**, so by default `y += x[i]` must be computed in order: the compiler says vectorized, but it's still one `fadd` after another (the assembly shows it). Allow the reordering explicitly with `#pragma clang fp reassociate(on)` at the start of the function's block; don't blanket the whole translation unit with `-ffast-math`.

## Stage 4 — Data-parallel loops and element width (§3.1)

```cpp
template <typename T>
[[gnu::noinline]] void add_lanes(T* out, const T* a, const T* b, std::size_t n);
```

`template <class T> add_lanes(out, a, b, n)`: `out[i] = a[i] + b[i]` for the types `uint8/16/32/64_t`. A 128-bit NEON register holds 16/8/4/2 lanes, so the speedup must grow as the type gets narrower: at least **8/4/2/1.4×**, respectively. The loop bound is a runtime parameter. Change the operation to `*` and the speedup for `uint64_t` drops to 1 (NEON has no 64×64 multiply); try it yourself.

## Stage 5 — Stride and SoA (§3.2.2)

```cpp
class ParticleStore {
public:
    explicit ParticleStore(std::size_t count);

    std::size_t size() const;
    std::span<float> x();
    std::span<float> y();
    std::span<float> z();
    std::span<float> vx();
    std::span<float> vy();
    std::span<float> vz();

    [[gnu::noinline]] void integrate(float dt);
};
```

`ParticleStore`: six separate `float` fields, `x, y, z, vx, vy, vz` (SoA), and `integrate(dt)` performs `x += vx*dt` (and the same for y and z). A new store starts with every field at 0, and each accessor returns a span of `size()` elements. The test's baseline is an array of structures (AoS), and you must beat it by at least **1.4×**. Stride is vectorization's nemesis, not because the compiler can't handle it, but because AoS data simply isn't contiguous.

## Stage 6 — Reduction into an array (R3)

```cpp
[[gnu::noinline]] inline void row_sums(std::span<const int> matrix, std::size_t rows,
                                       std::size_t cols, std::span<int> out);
```

`row_sums(matrix, rows, cols, out)` (row-major): write the sum of each row to `out`. At least **3×** faster than scalar.

## Stage 7 — Real-world extension: Brute-force nearest neighbor

```cpp
[[gnu::noinline]] inline std::optional<std::size_t> best_match(std::span<const float> query,
                                                               std::span<const float> matrix,
                                                               std::size_t dim);
```

`best_match(query, matrix, dim)`: `matrix` is a `rows × dim` row-major array of floats (`rows = matrix.size() / dim`, `dim > 0`). Return the row with the largest dot product; on a tie, take the lowest row number; when there are no rows, return `std::nullopt`. At least **4×** faster than scalar.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Example 1's assembly counts its index up from `-2^16` to 0. Why not from 0 up to `2^16`? What can C's `restrict` save? Why doesn't the C++ standard have it?
2. How do AVX2's aligned loads and stores differ from NEON's `ld1`? Does aligning data still matter on Apple Silicon? (HW3 Write-up 2)
3. Why doesn't the loop `a[i] = b[i + 1]` use the vector registers? Is it slower than `vectorized`? (HW3 Write-up 4, loop idiom)

---

**Official solution:** [R3 Solutions](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/dd10cda9a809e5afb37a9723e10a45ef_MIT6_172F18_rec3sol.pdf). Read it after you finish.
