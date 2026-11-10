---
title: Min Queue (a FIFO with an amortized O(1) minimum)
unit: 04 · Amortization and Randomization (PS4)
source: 6.046J PS4 Problem 4-1
pdf: materials/6.046j/psets/ps04/questions.pdf
answer: materials/6.046j/psets/ps04/solutions.pdf
read: materials/6.046j/lectures/lec05.pdf
kind: code
minutes: 75
---

# 0411 · Min Queue

**Original problem:** [Problem Set 4](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/5eb7c1134350c89889dcaf7ec3fd609a_MIT6_046JS15_pset4.pdf) · **Read first:** [Amortization: Amortized Analysis](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/13e2c7d165259712327af0af312a068e_MIT6_046JS15_lec05.pdf)

> **Why this comes up in practice:** to report "the highest latency in the past minute", scanning the whole window every time is too expensive, and deleting from a heap is awkward. The amortized argument for this data structure is short enough to finish on a whiteboard, which is exactly the kind of thing interviews love.

## Original problem

6.046J PS4 Problem 4-1 ("Extreme FIFO Queues") asks for a data structure that holds a first-in, first-out queue of integers and supports ENQUEUE, DEQUEUE, and FIND-MIN in O(1) amortized time each, so that any sequence of m operations costs O(m) in total. You may assume that every item ever enqueued is distinct.

- (a) Describe the data structure along with explicit invariants that capture what it guarantees. Hint: pair an ordinary queue with one or more helper structures that do the bookkeeping.
- (b) Describe the ENQUEUE, DEQUEUE, and FIND-MIN procedures precisely, in prose or pseudocode.
- (c) Prove that the operations return correct answers, for example by deriving correctness from your invariants and sketching why the invariants are maintained.
- (d) Give the worst-case cost of a single operation and the amortized cost over m operations.

## Your task

Build a FIFO queue that can also report the minimum of its current elements, with every operation in **O(1) amortized** time.

Parts (a), (b), and (d) become code whose answers and comparison counts the tests check, and the correctness argument of part (c) moves to the interview follow-ups. Unlike the original, the element type and the ordering are generic, elements need not be distinct, and Stage 3 uses the queue to track a peak over a time window.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Basic operations

```cpp
template <typename T, typename Compare = std::less<T>>
class MinQueue {
public:
    explicit MinQueue(Compare compare = Compare{});

    std::size_t size() const;
    bool empty() const;
    void enqueue(T value);
    T dequeue();
    const T& front() const;
    const T& find_min() const;
};
```

`MinQueue<T, Compare>`, where `Compare = std::less<T>` by default:

| Operation | Behavior |
| --- | --- |
| `enqueue(value)` | Add to the back of the queue |
| `dequeue()` | Remove and return the front of the queue |
| `front()` | Return the front of the queue |
| `find_min()` | Return the current minimum (as decided by `Compare`) |
| `size()`, `empty()` | The current number of elements |

When the queue is empty, `dequeue`, `front`, and `find_min` throw `std::out_of_range`. Copying a queue copies its full state, and the copy is independent of the original afterwards.

**The original problem assumes the elements are distinct; this exercise doesn't.** The most common mistake is deciding by value, on a dequeue, whether to remove something from the auxiliary structure: two equal values are not the same element.

## Stage 2 — Amortized cost and generality

- Comparisons are amortized O(1), and the tests count them with `Counted`: a random mix of `m` enqueues and dequeues, with a `find_min` after each step, may make at most `3m` comparisons in total. A run of `n` decreasing enqueues followed by `n` dequeues may make at most `3n`, and the worst case, `n` increasing enqueues followed by one smaller value that evicts the entire run at once, at most `3(n + 1)`.
- `Compare` may have state (a lambda that captures outside variables).
- `T` may be **move-only** (for example, it may hold a `unique_ptr`): the auxiliary structure must not keep copies of `T`.

## Stage 3 — Real-world extension: the peak within a window

```cpp
class PeakTracker {
public:
    explicit PeakTracker(std::int64_t window);

    void record(std::int64_t timestamp, std::int64_t value);
    std::optional<std::int64_t> peak(std::int64_t now);
};
```

`PeakTracker(window)`: `window ≥ 1`; otherwise throw `std::invalid_argument`. Use a `MinQueue<int64, std::greater<>>` internally.

- `record(timestamp, value)`: record one data point.
- `peak(now)`: return the largest value whose time falls in `(now - window, now]`, or `std::nullopt` if there is no such data.

Time only moves forward: the time arguments of `record` and `peak`, taken together, must be non-decreasing; otherwise throw `std::invalid_argument`. Several data points may share the same time.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Prove the O(1) amortized bound with the potential method: how do you define `Φ`? What is the worst case of a single `enqueue`?
2. What is the invariant? Why is the auxiliary structure always increasing? (This is the correctness argument the original problem asks for; explain it out loud here.)
3. If you need O(1) in the **worst case**, or must support a non-commutative aggregate (such as string concatenation), how do you change the two-stack approach?

---

**Official solution:** [Solutions to Problem Set 4](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/8cc8292f1b04e9adc43546295b2ecae3_MIT6_046JS15_pset4sols.pdf). Read it after you finish.
