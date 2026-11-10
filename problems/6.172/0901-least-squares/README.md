---
title: Least Squares and Complexity Classification
unit: 09 · Dynamic Analysis (HW7)
source: 6.172 HW7 §3 (Linear-regression analysis)
pdf: materials/6.172/homework/hw07/handout.pdf
kind: code
minutes: 70
---

# 0901 · Least Squares and Complexity Classification

**Based on:** [HW7: Dynamic Analysis Tools](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/1ca096c3f8a039f3b782479c1deca8a4_MIT6_172F18hw7.pdf)

> **Why this comes up in practice:** no instrument directly measures something like "how long does one function call take"; you can only measure the total time of many runs and then use regression to separate the individual costs. Fitting measurements to a cost model is one of the most common techniques in performance engineering, and how well-conditioned the model is (collinearity) also decides whether the numbers can be trusted.

## Original problem

6.172 HW7 §3 (Linear-regression analysis) models a program's running time as a linear function of event counts and uses linear regression, fit with any data-analysis tool, to solve for the cost of each kind of event. The counts come from CSI, a compiler-instrumentation framework in which a tool defines hook functions that the compiler calls at events such as basic-block entry and function entry.

- To measure a function call, it uses T = a · I + b · F, where I is the number of instructions other than function calls, F is the number of function calls, and a and b are constants. The test program computes the nth Fibonacci number by exponential-time recursion until n drops below a base-case size (at least 2), and then switches to a loop, so the choice of n and of the base case changes both I and F.
- To measure a spawn, it uses T1 = a · I + b · S, where I now includes the serial function calls and S is the number of `cilk_spawn`s. The Cilk version spawns the first recursive call whenever n is larger than the base-case size.

The write-ups:

- Write-up 1: write a CSI tool that counts LLVM IR instructions (using each basic block's non-empty size) and function calls.
- Write-up 2: describe a measurement strategy: inputs that keep the instruction and call counts from being highly correlated; how you time the runs (which timers, how many runs per input, and whether you aggregate with the min, mean, or median); and how you cope with the instrumentation perturbing the running time.
- Write-up 3: collect the measurements, solve for a and b, and report how correlated the instruction and call counts are.
- Write-up 4: extend the tool to count spawns as well (CSI calls a detach hook at each spawn, and you may assume a single worker).
- Write-up 5: repeat the analysis for the spawn model with a single worker: describe your strategy, report b and the correlation between the instruction and spawn counts, and express the cost of a spawn in function calls.

## Your task

Write all of the homework's math yourself. The later exercises 0911–0913 use `_shared/08_ols.hpp` directly (it has the same interface as this exercise, in namespace `practice::ols`, except that `LineFit`, `pearson`, and `fit_multilinear` are named `Line`, `correlation`, and `solve` there).

> ⚠️ **Spoiler warning:** `_shared/08_ols.hpp` is the complete solution to this exercise; don't open it until you're done.

This exercise covers only the math behind the analysis: the least-squares fits, the correlation, and the multiple regression that the homework leaves to a tool such as gnuplot or Excel, plus a classifier that turns call counts into a growth class.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Simple linear regression

```cpp
struct LineFit {
    double slope = 0;
    double intercept = 0;
    double r_squared = 0;
};

inline LineFit fit_line(std::span<const double> x, std::span<const double> y);
```

`fit_line(x, y)`: return the `y = slope·x + intercept` that minimizes the squared error, along with `r_squared` (`1 - SS_res/SS_tot`, defined as 1 when `SS_tot` is 0).

If `x` and `y` differ in length, there are fewer than 2 points, or all the `x` values are equal, throw `std::invalid_argument`.

## Stage 2 — Correlation and multiple regression

```cpp
inline double pearson(std::span<const double> x, std::span<const double> y);
inline std::vector<double> fit_multilinear(const std::vector<std::vector<double>>& rows,
                                           std::span<const double> y);
```

- `pearson(x, y)`: the Pearson correlation coefficient. Return 0 if either series is constant. Invalid inputs are the same as in Stage 1 (lengths that differ, or fewer than 2 points), except that a constant series returns 0 instead of throwing.
- `fit_multilinear(rows, y)`: solve `y ≈ rows · β` (**without** an intercept term), where `rows[i]` holds all the regressors of observation i. Use the normal equations with Gaussian elimination and partial pivoting.

In the homework's model `T = a·I + b·F`, `I` can reach 10⁹ while `F` is only 10³. Scales that far apart break the pivot decisions, so scale each column first (for example, divide it by its entry of largest absolute value). When the regressors are linearly dependent, throw `std::domain_error`; never return garbage. A column of all zeros counts as linearly dependent. Rows of different lengths, or a `y` that doesn't have exactly one value per row, throw `std::invalid_argument`.

## Stage 3 — Estimating complexity from counts

```cpp
enum class Growth { Constant, Logarithmic, Linear, NLogN, Quadratic, Cubic, Exponential };

inline double power_law_exponent(std::span<const double> n, std::span<const double> counts);
inline Growth classify_growth(std::span<const double> n, std::span<const double> counts);
```

Apply regression to call counts and you get a "complexity detector".

- `power_law_exponent(n, counts)`: the slope of `log(counts)` against `log(n)`.
- `classify_growth(n, counts)`: return a `Growth`. Method: for each of `Constant / Logarithmic / Linear / NLogN / Quadratic / Cubic`, solve `counts ≈ a·f(n)` by least squares and compute the RMS residual on a log scale; for `Exponential`, use the two-parameter fit `log(counts) = n·log b + log a`. The smallest residual wins, and when two residuals are within `1e-9` of each other, pick the one that comes earlier in the list.

At least 3 data points are required; otherwise throw `std::invalid_argument`. Both functions also throw `std::invalid_argument` when `n` and `counts` differ in length.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why are the normal equations less numerically stable than QR? When does the difference become large? Does it matter for the homework's case of two regressors?
2. What does collinearity do to the results of a regression? How do you design an experiment to avoid it? (Homework Write-up 2)
3. What does a log-log slope make of `n log n`? Why does the Stage 3 method make `n log n` a candidate of its own instead of looking only at the slope?
