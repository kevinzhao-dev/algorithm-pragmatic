---
title: Wildcard Matching (string matching with FFT)
unit: 02 · FFT and B-Trees (PS2)
source: 6.046J PS2 Problem 2-1
pdf: materials/6.046j/psets/ps02/questions.pdf
answer: materials/6.046j/psets/ps02/solutions.pdf
read: materials/6.046j/lectures/lec03.pdf
kind: code
minutes: 90
---

# 0211 · Wildcard Matching

**Original problem:** [Problem Set 2](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/4a1b44e7e3f34aa85adb782d70048210_MIT6_046JS15_pset2.pdf) · **Read first:** [Divide & Conquer: FFT](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/3dc84e78914f226665357234d6f4e25c_MIT6_046JS15_lec03.pdf)

> **Why this comes up in practice:** sequence analysis (matching DNA fragments) and log scanning often use fixed patterns with wildcards. Algorithms like KMP can't handle `*`, and once you can't rely on a failure function, rewriting the matching as a convolution is the standard solution.

## Original problem

6.046J PS2 Problem 2-1 ("Pattern Matching") gives a source string S[0..n−1] of length n over the symbols a and b, and a pattern P[0..m−1] of length m, with m much smaller than n, over the symbols a, b, and ∗. The wildcard ∗ matches any single symbol, a or b, while all other symbols have to be equal. The answer M lists, in sorted order, every position j where P matches S[j..j+|P|−1]. For example, S = ababbab and P = ab∗ give M = [0, 2].

- (a) Describe a simple brute-force algorithm that runs in O(nm) time.
- (b) Solve the problem by reducing it to polynomial multiplication: explain how to turn S and P into polynomials whose product determines M, and illustrate both the encoding and the way you read M off the product on the example above.
- (c) If you combine part (b) with the FFT for polynomial multiplication from Lecture 3, what is the running time?
- (d) Now the text is a DNA strand D[0..n−1] over A, C, G, and T, and the pattern P[0..m−1] is over A, C, G, T, and ∗. The output is again the sorted list of match positions; for example, D = ACGACCAT and P = AC∗A give M = [0, 3]. Building on parts (b) and (c), give an efficient algorithm for this alphabet and run it on that example.

## Your task

The text `text` contains no wildcards. In the pattern `pattern`, `*` matches exactly **one** arbitrary character, and every other character must match exactly. Return every match position `j`, that is, every `j` at which `pattern` lines up with `text[j..j+m-1]`, in increasing order.

Rules for every stage:

- Throw `std::invalid_argument` if a string contains a character outside the alphabet (`*` may appear only in `pattern`).
- An empty `pattern` matches at every position `0..n` (including `n`). If `pattern` is longer than `text`, return an empty result.

Stage 1 is part (a), Stage 2 is parts (b) and (c), and Stage 3 generalizes part (d) to any alphabet through a binary encoding. You leave the analysis to the tests and the interview follow-ups.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Direct matching

```cpp
std::vector<std::size_t> match_naive(std::string_view text, std::string_view pattern);
```

`match_naive(text, pattern)`: the alphabet is `{a, b}`. O(nm) in the worst case.

## Stage 2 — Turning it into a convolution

```cpp
std::vector<std::size_t> match_fft(std::string_view text, std::string_view pattern);
```

`match_fft(text, pattern)`: the same alphabet, in **O((n+m) log (n+m))**. Copy over the `fft` you wrote in [`0201`](../0201-fft-polynomial/README.md).

Encode `a` as `1`, `b` as `-1`, and `*` as `0`, reverse the pattern, and multiply once: term `m-1+j` of the product is the pattern's "degree of agreement" at position `j`. Work out for yourself what counts as a match, and why.

**Numeric contract:** the coefficients are only `0, ±1`, so no product term exceeds `m` in absolute value. That is far looser than the 0201 contract, so rounding is always correct. Still, compare only the results of `llround`; never compare floating-point values directly.

The tests include the worst case: at every position the match fails only at the last character, so direct matching times out.

## Stage 3 — Any alphabet

```cpp
std::vector<std::size_t> match_alphabet(std::string_view text, std::string_view pattern,
                                        std::string_view alphabet);
```

`match_alphabet(text, pattern, alphabet)`: `alphabet` is the alphabet; it must not be empty, contain `*`, or repeat a character (otherwise throw `std::invalid_argument`).

Use a binary encoding to turn each character into `k` symbols from `{a, b}`, then call Stage 2. Each `*` expands into `k` copies of `*`. After encoding, a match can start in the middle of a character, and you must filter those out: which position of the original text does bit 3 of the encoded string belong to?

## Interview follow-ups

Once you're done, answer without looking at your code:

1. What is the correct match condition? Why can the product decide it with the `1` and `-1` encoding, but not with `1` and `0`?
2. When `alphabet` is large (say, all of Unicode), the binary encoding stretches the length by a factor of `log σ`. What approach works without an encoding? (Hint: `Σ P[j](P[j] − T[j])²`.)
3. If `pattern` is short but `text` is very long, how do you split the work into blocks to bring the FFT cost down from `O(n log n)` to `O(n log m)`?

---

**Official solution:** [Solutions to Problem Set 2](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/d608c09d3f727411722e3ff5a95b103f_MIT6_046JS15_pset2sols.pdf). Read it after you finish.
