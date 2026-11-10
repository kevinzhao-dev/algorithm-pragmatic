---
title: Ordering Functions by Growth Rate
unit: 01 · Sequences (PS1)
source: 6.006 PS1 Problem 1-1
pdf: materials/6.006/psets/ps1/questions.pdf
answer: materials/6.006/psets/ps1/solutions.pdf
read: materials/6.006/lectures/lec01.pdf, materials/6.006/recitations/rec01.pdf
kind: paper
minutes: 30
---

# 0111 · Ordering Functions by Growth Rate

**Original problem:** [Problem Set 1 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/dc56bd1995baf20cfcdd3fbabdd2dbc9_MIT6_006S20_ps1-questions.pdf) · **Read first:** [Lecture 1: Introduction notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/477c78e0af2df61fa205bcc6cb613ceb_MIT6_006S20_lec1.pdf), [Recitation 1 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/c6d8f06c6f11e3342633dec85498f551_MIT6_006S20_r01.pdf)

> **Why this comes up in practice:** when an interviewer asks "will this solution hold up at n = 10⁹?", you need to rank `n log n`, `n^1.5`, and `2^√n` within 30 seconds. This exercise trains that reflex.

## Original problem

6.006 PS1 Problem 1-1 is about the asymptotic behavior of functions. It gives four groups of five functions. For each group, list the functions in an order where every function is O of each function that comes after it. When two functions are each O of the other, so that either could come first, group them together inside curly braces. For instance, with f1 = n, f2 = √n, and f3 = n + √n, both (f2, {f1, f3}) and (f2, {f3, f1}) are correct.

Conventions: a stacked exponent a^b^c is read as a^(b^c), not (a^b)^c; log is base 2 unless another base is written out; and Stirling's approximation is useful for comparing factorials. Here C(n, k) is the binomial coefficient "n choose k."

- (a) f1 = log(n^n), f2 = (log n)^n, f3 = log(n^6006), f4 = (log n)^6006, f5 = log log(6006n).
- (b) f1 = 2^n, f2 = 6006^n, f3 = 2^(6006^n), f4 = 6006^(2^n), f5 = 6006^(n^2).
- (c) f1 = n^n, f2 = C(n, n − 6), f3 = (6n)!, f4 = C(n, n/6), f5 = n^6.
- (d) f1 = n^(n+4) + n!, f2 = n^(7√n), f3 = 4^(3n log n), f4 = 7^(n^2), f5 = n^(12 + 1/n).

The answer is written; there is no code.

## Your task

The Original problem section above lists four groups of five functions each (go by the formulas exactly as they are written there). Sort each group from slowest to fastest growth: if `f` comes before `g`, that means `f = O(g)`. When two functions have the same order of growth, mark them.

There is no code to write: your answer goes in a new `answer.md` file in this folder, as described under Your answer below.

## Your answer

Create `answer.md` in this folder, and for each group write down your ordering and a **one-sentence reason**. Tools you can use:

- Take logs, then compare (you almost always have to break down a tower of exponents this way).
- Stirling: `log(n!) = Θ(n log n)`.
- Changing between constant bases only costs a constant factor, but a constant **in the exponent** is not a constant factor.

When you're done, open the solution that the `answer` field points to and compare, then run `./practice done 0111`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Which is larger, `log(n^6006)` or `(log n)^6006`? How far apart are their orders of growth?
2. Why are `2^n` and `6006^n` of different orders, while `log₂ n` and `log₆₀₀₆ n` are of the same order?

---

**Official solution:** [Problem Set 1 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/10e4e3444c1bde4c662a47a8f14dadd3_MIT6_006S20_ps1-solutions.pdf). Read it after you finish.
