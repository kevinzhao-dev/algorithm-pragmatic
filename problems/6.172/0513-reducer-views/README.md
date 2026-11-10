---
title: Reducers — Concurrent Accumulation Without Locks
unit: 05 · Fork-Join and Reducers (HW4)
source: 6.172 HW4 §3.3 Reducers (Write-up 7–8)
pdf: materials/6.172/homework/hw04/handout.pdf
read: materials/6.172/lectures/lec06.pdf, materials/6.172/homework/hw04/handout.pdf
kind: code
sanitizer: tsan
minutes: 180
---

# 0513 · Reducers: Concurrent Accumulation Without Locks

**Original problem:** [HW4: Cilk, Reducer Hyperobjects](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/b2ff5659ae776c6fa6834967f73a2ab4_MIT6_172F18hw4.pdf) · **Read first:** [Multicore Programming](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/cbef71a2b92bc700fbad43e69f951513_MIT6_172F18_lec6.pdf), [HW4: Cilk, Reducer Hyperobjects](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/b2ff5659ae776c6fa6834967f73a2ab4_MIT6_172F18hw4.pdf)

> **Why this comes up in practice:** when you add up counters, append to logs, or collect results in parallel, the first instinct is "add a lock" or "give each thread its own buffer and join them at the end." The latter scrambles the order, and a lot of data (an ordered log, output by index) needs exactly serial semantics. That's what Cilk's reducer hyperobject provides: no locks, no races, and a result identical to the serial execution.

## Original problem

6.172 HW4 §3.3 introduces Cilk's reducer hyperobjects. A reducer is a variable that parallel strands can update safely: the runtime keeps several views of it, lets only one strand at a time touch each view, and has merged all the views into one by the time a `cilk_sync` completes. A reducer is defined by a monoid: a set T with an associative binary operator ⊕ and an identity element e (for example, the integers under + with e = 0). Because ⊕ is associative and e is an identity, a serial chain of updates applied to the initial value can be split into several views, each starting from e, and combined in order with the same result. So a reducer keeps serial semantics no matter how many processors run the program or how the work is scheduled, needs no locks, and costs about a hash-table lookup per access. Cilk ships reducers for common monoids such as +, but none for the list of N-queens boards from §3.2.

- **Write-up 7:** explain how a `BoardListReducer` would remove the need for the temporary lists of §3.2, and define its monoid: the type of objects it operates on, its associative operator, and its identity.
- Then build that reducer with Cilk's C reducer API: write reduce, identity, and destroy callbacks for a `BoardList`, declare and initialize the reducer, register it before use and unregister it afterwards, update it through its view, and read its final value once all parallel work is done.
- **Write-up 8:** parallelize `queens` with the reducer, time it with `awsrun8`, check that the answers match the serial code's, and compare its performance with the parallel version that used temporary lists.

The original is C with Cilk.

## Your task

A **Monoid** `M` provides `value_type`, `M::identity()` (the identity element), and `M::reduce(left, right)` (which merges `right` into `left`; the order matters, and the caller guarantees associativity).

`Reducer<M>` is built on `practice::fork2` (see `harness/forkjoin.hpp`). In this exercise you **must not use a mutex**, and don't "hand out views by `worker_index()` and join them by worker number at the end" either: that happens to be correct for a commutative `+`, but it's wrong for string concatenation (consider that the way work stealing divides the work has nothing to do with worker numbers).

Instead of filling in Cilk's reducer callbacks, you implement the reducer mechanism itself on top of `practice::fork2`: first a view per branch, then views created only when needed, and finally a reducer that collects N-queens solutions in serial order.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — One view per branch

```cpp
template <typename M>
concept Monoid = requires(typename M::value_type& left, typename M::value_type&& right) {
    { M::identity() } -> std::convertible_to<typename M::value_type>;
    M::reduce(left, std::move(right));
};

template <Monoid M>
class Reducer {
public:
    using value_type = typename M::value_type;

    Reducer();
    explicit Reducer(value_type initial);

    value_type& view();

    template <typename F, typename G>
    void fork2(F&& f, G&& g);

    template <typename Body>
    void parallel_for(std::size_t begin, std::size_t end, Body&& body, std::size_t grain = 1);

    value_type take();
};
```

| Interface | Behavior |
| --- | --- |
| `Reducer<M>(initial)` | The root view starts at `initial` (the default constructor uses `M::identity()`)|
| `view()` | Return (a reference to) the current strand's view|
| `fork2(f, g)` | Run `f` and `g` in parallel, with semantics identical to "first `f`, then `g`": `f` carries on with the current view, `g` gets a new view (`M::identity()`), and when `fork2` returns, `g`'s view is `reduce`d into `f`'s view |
| `parallel_for(begin, end, body, grain = 1)` | Split the range in halves recursively with `fork2`; `body(i)` updates through `view()` |
| `take()` | Take out the final value after the parallel region ends |

The key is how `view()` knows which strand it's on: a `thread_local` pointer that `fork2` sets while it runs each branch and restores when the branch ends (RAII).

Acceptance uses string concatenation: no matter how many workers there are, the result must match the serial one. Everything runs under ThreadSanitizer.

Assume that only one `Reducer`'s parallel region runs at a time (each `Reducer` just uses its own `fork2`; you don't need to handle two reducers interleaving).

## Stage 2 — Creating views lazily (no steal, no cost)

Cilk creates a new view only when a steal actually happens; in a serial execution a reducer costs almost 0. The improvement: when `g` starts, if `f` has already finished, `g` simply takes over `f`'s view, with no new view and no reduce (think about why this still has serial semantics, and what synchronization you need to know safely whether `f` has finished).

Test: with `workers = 1`, the whole computation of a `Reducer<M>(initial)` must not call `M::identity()` or `M::reduce()` even once (a single worker never steals, so not even one new view is needed). A `PERF` test also makes 2^21 nested forks on one worker within a time budget, so the single-worker path must be nearly free.

## Stage 3 — Real-world extension: Collecting N-Queens solutions with a reducer

```cpp
std::vector<std::uint64_t> queens_ordered(int n, int serial_depth);
```

`queens_ordered(n, serial_depth)`: return all solutions (a `std::vector<std::uint64_t>`, with the same `board` encoding as 0512: 4 bits per row, row 0 in the lowest bits), trying candidate columns from smallest to largest. Nodes with `row < serial_depth` run in parallel through `reducer.fork2`, and the rest run serially. The parallel and the serial (`serial_depth = 0`) results must be **identical element by element**, order included.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does a reducer need only a monoid (associativity + an identity) and not commutativity? Which property does Stage 3's ordering guarantee rely on?
2. If `M::identity()` is expensive (say, it allocates a big new buffer), how much does Stage 2's improvement matter? (HW4 Write-up 8)
3. HW4's reducers find their views through a hash table, while you use thread_local here. What's the difference? When several reducers are in use at the same time, which approach runs into trouble?
