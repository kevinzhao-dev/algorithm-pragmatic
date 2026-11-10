---
title: Heap with Updatable Priorities (handles)
unit: 04 · Trees & Heaps (PS4)
source: 6.006 Lecture 8, Recitation 8
pdf: materials/6.006/recitations/rec08.pdf
read: materials/6.006/lectures/lec08.pdf
kind: code
minutes: 75
---

# 0406 · Heap with Updatable Priorities

**Based on:** [Recitation 8 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/701c2f339890cbf9c0c2a3495d007f26_MIT6_006S20_r08.pdf) · **Read first:** [Lecture 8: Binary Heaps notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/40d4851e550507ca14dc778b9b2266cc_MIT6_006S20_lec8.pdf)

> **Why this comes up in practice:** Dijkstra's decrease-key, an OS scheduler adjusting priorities, a rate limiter recomputing when it will next let a request through: all of these need to change an element's priority in O(log n) through a handle you already hold. `std::priority_queue` can't do that, because it doesn't know where an element sits in its array.

## Your task

`UpdatableHeap<T, Compare>` is a max-heap (with respect to `Compare`). Unlike 0405, `push` returns a stable `Handle` that you can later use at any time to look up or modify that element, even though the element's position in the array keeps changing.

The core approach: **keep the heap array and the handle table in sync with each other**. A handle records which slot it maps to, and the slot records its position in the heap array; every swap must update both sides.

`Handle` is a comparable, default-constructible value type, and you design its contents.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — push / pop and handles

```cpp
template <typename T, typename Compare = std::less<T>>
class UpdatableHeap {
public:
    class Handle {
    public:
        Handle() = default;
        friend bool operator==(const Handle&, const Handle&);
    };

    explicit UpdatableHeap(Compare compare = Compare{});

    Handle push(T value);
    const T& top() const;
    Handle top_handle() const;
    T pop();
    std::size_t size() const;
    bool empty() const;
    bool contains(Handle handle) const;
    const T& get(Handle handle) const;
};
```

- `push(v)`: return a handle, in O(log n).
- `top()` / `top_handle()`: O(1); throw `std::out_of_range` on an empty heap.
- `pop()`: remove and return the top (throw `std::out_of_range` on an empty heap).
- `contains(h)`: whether this handle is currently valid. **A default-constructed handle is never valid.**
- `get(h)`: get the value; throw `std::invalid_argument` for an invalid handle.
- `size()` / `empty()`.

## Stage 2 — Changing priorities

```cpp
// in class UpdatableHeap<T, Compare>
void update(Handle handle, T value);
```

`update(h, v)`: change the value of `h` to `v` in O(log n) (at most `3⌈lg n⌉ + 8` comparisons in the tests); the element may move up or down. Throw `std::invalid_argument` if `h` is invalid.

## Stage 3 — Arbitrary deletion and stale-handle detection

```cpp
// in class UpdatableHeap<T, Compare>
T erase(Handle handle);
std::vector<Handle> build(std::vector<T> items);
```

- `erase(h)`: delete and return that element in O(log n); throw `std::invalid_argument` if `h` is invalid.
- After a `pop` or `erase`, the old handle must stay invalid forever, **even if the internal slot is reused by a new element** (think about a generation counter).
- `build(items)`: throw away the current contents and rebuild from `items` in O(n) (no more than `3n` comparisons), returning a handle for every item (in the same order as `items`).

## Stage 4 — Real-world extension: deadline scheduler

```cpp
class DeadlineScheduler {
public:
    bool submit(const std::string& name, std::int64_t deadline);
    bool reschedule(const std::string& name, std::int64_t deadline);
    bool cancel(const std::string& name);
    std::optional<std::string> next();
    std::optional<std::string> peek() const;
    std::size_t size() const;
};
```

`DeadlineScheduler` manages named jobs, and job names are unique:

- `submit(name, deadline)`: return `true` for a new job; if the name already exists, return `false` and do nothing.
- `reschedule(name, deadline)`: change the deadline (keeping the original submission order); return `false` if the name isn't found.
- `cancel(name)`: remove the job; return `false` if the name isn't found.
- `next()`: take out the name of the job that should run first, or `std::nullopt` if there are no jobs. Among equal deadlines, **the job submitted first runs first**.
- `peek()`: look at that name without taking it out. `size()`.

Every operation is O(log n) (expected, because you need a hash map to find a handle from a name).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. For Dijkstra, what are the trade-offs between lazy deletion (pushing duplicates and skipping stale entries when popping) and decrease-key?
2. Why can't a handle simply store an array index? What bugs does a generation counter prevent?
3. Does your slot table grow without bound? What happens to a slot after `erase`?
