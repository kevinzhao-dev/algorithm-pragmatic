---
title: Ring Deque and Sliding-Window Rate Limiter
unit: 01 · Sequences (PS1)
source: 6.006 Lecture 2, Recitation 2
pdf: materials/6.006/recitations/rec02.pdf
read: materials/6.006/lectures/lec02.pdf
kind: code
minutes: 70
---

# 0104 · Ring Deque

**Based on:** [Recitation 2 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/c08a3b63dfe5f6f6b32257d35f86ae63_MIT6_006S20_r02.pdf) · **Read first:** [Lecture 2: Data Structures notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/79a07dc1cb47d76dae2ffedc701e3d2b_MIT6_006S20_lec2.pdf)

> **Why this comes up in practice:** work queues, network packet buffers, and sliding-window statistics all need O(1) pushes and pops at both ends. A ring buffer is the most common way to get them: compared with a linked list, it saves one allocation per element, and it is more cache-friendly.

## Your task

`RingDeque<T>`: implement a deque with an array that is reused in a circle. `T` can be default-constructed and moved (the point of this exercise is circular indexing; you already practiced raw storage in 0103, so you may use `std::vector<T>` underneath).

After the deque and its amortized contract, Stage 3 uses the deque to build a sliding-window rate limiter.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Circular indexing

```cpp
template <typename T>
class RingDeque {
public:
    RingDeque();

    std::size_t size() const;
    bool empty() const;
    std::size_t capacity() const;

    const T& get_at(std::size_t i) const;
    void set_at(std::size_t i, T value);

    void insert_first(T value);
    void insert_last(T value);
    T delete_first();
    T delete_last();
};
```

| Operation | Behavior | Complexity |
| --- | --- | --- |
| `RingDeque()` | Empty | O(1) |
| `insert_first(v)` / `insert_last(v)` | Insert at the front / back | Amortized O(1) |
| `delete_first()` / `delete_last()` | Delete and return from the front / back; throw `std::out_of_range` when empty | Amortized O(1) |
| `get_at(i)` / `set_at(i, v)` | Element `i` (counting from the front); throw `std::out_of_range` when out of range | O(1) |
| `size()`, `empty()`, `capacity()` | | O(1) |

Edge cases: the indices when the front or back wraps around the end of the array, telling the empty state apart from the full state, and starting from capacity 0.

## Stage 2 — The amortized contract

As in 0103, `capacity()` isn't tied to any particular value, but it must satisfy:

1. Always `size() ≤ capacity() ≤ 4·size() + 16`.
2. Whichever end you operate on, and in whatever order, **m operations in total move elements (copy + move) at most 9m + a constant times**.
3. It must not thrash.

Hint: when growing, lay the elements out again in logical order, with `head` starting at 0; don't do an O(n²) operation that shifts elements one at a time.

## Stage 3 — Real-world extension: a sliding-window rate limiter

```cpp
class SlidingWindowLimiter {
public:
    SlidingWindowLimiter(std::size_t limit, std::int64_t window);

    bool allow(std::int64_t t);
    std::size_t active(std::int64_t t);
};
```

`SlidingWindowLimiter`: the rule API gateways commonly use, "let through at most `limit` requests within the last `window` time units" (for example, at most 100 requests per 60 seconds).

- `SlidingWindowLimiter(limit, window)`: `limit ≥ 1` and `window ≥ 1`; otherwise throw `std::invalid_argument`.
- `allow(t)`: a request arrives at time `t`; return whether to let it through. It goes through if the number of requests already let through in the time interval `(t − window, t]` is `< limit`, and when it goes through, you must record it. **Rejected requests don't count.**
- `active(t)`: return the number of requests let through in the interval `(t − window, t]`.
- `t` must be non-decreasing; if it isn't, throw `std::invalid_argument`.

Store the times of the requests you let through in Stage 1's deque. Both functions must be **amortized O(1)** (the time complexity must not depend on `limit`).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why do ring buffers often make the capacity a power of 2? How do you replace `%` with a bitwise operation, and how does that help with negative indices?
2. In a fixed-capacity ring queue, both "full" and "empty" make `head == tail`. What are the common solutions, and what does each one cost?
3. Compared with a token bucket or a fixed window, what are the pros and cons of a sliding window log?
