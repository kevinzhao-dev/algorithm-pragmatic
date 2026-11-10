---
title: Hand-Written NEON Scanning (NEON in place of AVX2)
unit: 04 · Vectorization (HW3)
source: 6.172 HW3 (§3.1.3 Vector flavors, AVX2 to NEON)
pdf: materials/6.172/homework/hw03/handout.pdf
read: materials/6.172/homework/hw03/handout.pdf
kind: perf
minutes: 120
---

# 0412 · Hand-Written NEON Scanning

**Original problem:** [HW3: Vectorization](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/072651a8229a63376d5720c9a500ae45_MIT6_172F18hw3.pdf) · **Read first:** [HW3: Vectorization](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/072651a8229a63376d5720c9a500ae45_MIT6_172F18hw3.pdf)

> **Why this comes up in practice:** the inner loops of `memchr`, JSON/CSV parsers, and string matching have early exits, so the compiler's auto-vectorization can't help, which is exactly why libraries like simdjson hand-write their SIMD. On Apple Silicon the tool is NEON, and its biggest difference from AVX2 is that it **has no `movemask`**.

## Original problem

6.172 HW3 §3.1.3 ("Flavors of vector arithmetic") starts from the fact that the vector unit is built in hardware, so every kind of vector operation (subtract, multiply, and so on) needs hardware of its own. It reuses the data-parallel loop from §3.1, which combines two `uint32_t` arrays elementwise into a third, and swaps the operator through the `__OP__` macro to see which operators still vectorize.

- **Write-up 8:** try different operators in the loop (fix, however you like, the division-by-zero errors caused by array `B` starting out all zeros). Do any of them fail to vectorize with `VECTORIZE=1 AVX2=1`? Study the assembly for `<<` built with only `VECTORIZE=1`, and explain how it differs from the AVX2 version.

The original is C compiled by Clang for x86, with and without AVX2.

## Your task

This exercise uses the intrinsics from `<arm_neon.h>` (`vld1q_u8`, `vceqq_u8`, `vdupq_n_u8`, `vandq_u8`, …). Rules shared by every stage:

- **Never read outside the buffer:** handle a tail of fewer than 16 bytes with scalar code. The tests run under ASan with exactly-sized allocations, so reading even one byte too many gets caught.
- You must get a clear speedup over a scalar loop in the same executable (`#pragma clang loop vectorize(disable)`); each stage below gives the factor.
- When nothing is found, return `npos` (defined as `haystack.size()`).

Where the original asks which operations the compiler can vectorize for x86, here you write the vector code by hand with NEON intrinsics, for scanning loops with early exits that the compiler won't vectorize, and you work around NEON's missing `movemask`.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — `find_byte(haystack, needle)`

```cpp
[[gnu::noinline]] inline std::size_t find_byte(std::span<const std::uint8_t> haystack,
                                               std::uint8_t needle);
```

Return the index of the first byte equal to `needle`. At least **4×** faster than scalar (on a long buffer with no match).

What do you do without `movemask`? Use `vshrn_n_u16(vreinterpretq_u16_u8(eq), 4)` to squeeze the 16 mask bytes into 64 bits (4 bits per byte), then count the trailing zeros and divide by 4.

## Stage 2 — `count_byte(haystack, needle)`

```cpp
[[gnu::noinline]] inline std::size_t count_byte(std::span<const std::uint8_t> haystack,
                                                std::uint8_t needle);
```

Return the number of occurrences, at least **4×** faster than scalar. Each lane's comparison result is `0x00` or `0xFF`.

## Stage 3 — `to_lower_ascii(text)`

```cpp
[[gnu::noinline]] inline void to_lower_ascii(std::span<char> text);
```

Convert only `'A'..'Z'` to lowercase, and leave every other byte unchanged (including non-ASCII bytes `>= 0x80`). At least **4×** faster than scalar. No branches: add `0x20` through a mask.

## Stage 4 — `find_any_of3(haystack, a, b, c)`

```cpp
[[gnu::noinline]] inline std::size_t find_any_of3(std::span<const std::uint8_t> haystack,
                                                  std::uint8_t a, std::uint8_t b, std::uint8_t c);
```

The index of the first byte equal to `a`, `b`, or `c`, at least **3×** faster than scalar.

## Stage 5 — Real-world extension: Splitting CSV fields

```cpp
std::vector<std::string_view> split_fields(std::string_view line, char separator);
```

`split_fields(std::string_view line, char separator)`: return a `std::string_view` for each field, pointing into `line` rather than into a copy (empty fields count too; `""` splits into one empty field; there is no quote handling). Use your `find_byte` from earlier.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why won't the compiler vectorize a loop with an early exit? (Hint: you only know whether to continue after reading byte `i`, and reading past it may cross into a page you shouldn't touch.)
2. Why is AVX2's `_mm256_movemask_epi8` such a pain on NEON? What does the `vshrn` trick cost?
3. 256-bit AVX2 looks at 32 bytes at a time and NEON at only 16, so why is `memchr` on Apple Silicon still so fast? (Hint: decode width and caches.)
