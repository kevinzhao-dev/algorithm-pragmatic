---
title: FFT and Polynomial Multiplication (with a numeric contract and NTT)
unit: 02 · FFT and B-Trees (PS2)
source: 6.046J Lecture 3
pdf: materials/6.046j/lectures/lec03.pdf
read: materials/6.046j/lectures/lec03.pdf
kind: code
minutes: 180
---

# 0201 · FFT and Polynomial Multiplication

**Based on:** [Divide & Conquer: FFT](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/3dc84e78914f226665357234d6f4e25c_MIT6_046JS15_lec03.pdf) · **Read first:** [Divide & Conquer: FFT](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/3dc84e78914f226665357234d6f4e25c_MIT6_046JS15_lec03.pdf)

> **Why this comes up in practice:** convolution shows up in signal processing, image filters, big-integer arithmetic, and statistics such as "the distribution of the sum of two histograms". FFT brings O(n²) down to O(n log n), but when the result must be **exact integers**, this code most often breaks on floating-point rounding.

## Your task

A polynomial is a vector of coefficients, where `a[i]` is the coefficient of `x^i`. When both polynomials are non-empty, the product always has length `|a| + |b| − 1` (do **not** trim trailing 0s); if either one is empty, return an empty result.

You build the FFT from Lecture 3, use it for exact integer multiplication under an explicit precision contract, switch to a number-theoretic transform (NTT) for exact modular products, and finish with big-integer multiplication.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Direct multiplication

```cpp
std::vector<std::int64_t> multiply_naive(std::span<const std::int64_t> a,
                                         std::span<const std::int64_t> b);
```

`multiply_naive(a, b)`: O(|a|·|b|). It is the reference that later tests compare against, and also the best solution for most small inputs.

## Stage 2 — FFT

```cpp
using Complex = std::complex<double>;

void fft(std::span<Complex> a, bool inverse = false);
```

`fft(a, inverse)`: `a.size()` must be a power of 2; otherwise throw `std::invalid_argument` (0 is invalid too). Transform in place, in O(n log n).

- Forward: `â[k] = Σ_j a[j] · e^{+2πi·jk/n}` (a positive exponent, as in Lecture 3).
- Inverse: `a[j] = (1/n) Σ_k â[k] · e^{−2πi·jk/n}`; you must divide by `n` yourself.

## Stage 3 — Integer multiplication and the numeric contract

```cpp
std::vector<std::int64_t> multiply(std::span<const std::int64_t> a,
                                   std::span<const std::int64_t> b);
```

`multiply(a, b)`: return the **exact** integer product, in O(n log n).

| Symbol | Meaning |
| --- | --- |
| `n` | `\|a\| + \|b\|` |
| `A`, `B` | The largest absolute value of a coefficient in `a` and in `b`, respectively |

**Contract:** whenever `n · A · B ≤ 2^42` (about 4.4×10¹²), you must return the exact result; beyond that, throw `std::domain_error`, and never return a value that might be rounded wrong. Watch out for overflow when you evaluate this condition, because `A` and `B` can be close to the `int64` limit.

Why the line is drawn there: the rounding error of a `double` FFT is roughly on the order of `ε · ‖a‖₂ · ‖b‖₂` (`ε ≈ 1.1×10⁻¹⁶`, times a constant that grows slowly with log n). Within the contract this value is far below 0.5, so rounding to the nearest integer is always correct.

Getting there has a few common pitfalls:

- **Compute each twiddle factor directly** or look it up in a table. If you accumulate them with `w *= w_step`, the error exceeds 0.5 at the contract limit, and the tests will catch it.
- Use `std::llround`, not truncation (`2.9999999` is not 2).
- The main test cases sit at the contract limit (`n = 2^20`, `A = B = 2048`), with every coefficient at full magnitude and random signs.

## Stage 4 — NTT

```cpp
inline constexpr std::uint32_t kNttModulus = 998'244'353;  // provided

std::vector<std::uint32_t> multiply_mod(std::span<const std::uint32_t> a,
                                        std::span<const std::uint32_t> b);
```

`multiply_mod(a, b)`: every coefficient is already reduced into `[0, kNttModulus)` (otherwise throw `std::invalid_argument`). Return the product modulo `998244353 = 119·2²³ + 1` (primitive root 3), without using floating point. Throw `std::length_error` when `|a| + |b| − 1` exceeds `2^23`.

## Stage 5 — Real-world extension: multiplying big integers

```cpp
std::string multiply_decimal(std::string_view a, std::string_view b);
```

`multiply_decimal(a, b)`: `a` and `b` are decimal strings that may have a leading `-` and leading 0s. Throw `std::invalid_argument` for any other format (an empty string, a lone `-`, non-digits). Return the normalized string: no extra 0s, and `0` has no minus sign.

It must handle two numbers of one million digits each within the time limit. The Stage 3 contract decides how many digits you can pack into each slot; choose wrong and you'll throw an exception.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why must this FFT have a power-of-2 size? How would you handle an `n` that isn't a power of 2?
2. How would you change the code to relax the contract to `n·A·B ≤ 2^60`? (Compare long double, splitting into several FFTs, and NTT combined with CRT.)
3. If you only need the convolution of `a` with itself, or of two real sequences, how do you cut the number of FFTs from 3 to 2?
