---
title: Dynamic Array (amortized analysis and exception safety)
unit: 01 · Sequences (PS1)
source: 6.006 Lecture 2, Recitation 2
pdf: materials/6.006/recitations/rec02.pdf
read: materials/6.006/lectures/lec02.pdf
kind: code
minutes: 120
---

# 0103 · Dynamic Array

**Based on:** [Recitation 2 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/c08a3b63dfe5f6f6b32257d35f86ae63_MIT6_006S20_r02.pdf) · **Read first:** [Lecture 2: Data Structures notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/79a07dc1cb47d76dae2ffedc701e3d2b_MIT6_006S20_lec2.pdf)

> **Why this comes up in practice:** `std::vector` is the container you use most when you write C++. Why insertion at the end is amortized O(1), why an array that is nearly empty after deletions still shouldn't shrink right away, and why a `move` not marked `noexcept` makes `vector` fall back to copying when it grows: it's all in this exercise.

## Your task

Manage the memory yourself: **do not use `std::vector` or `std::deque`**. `T` can't necessarily be default-constructed, so you can't use `new T[n]`: allocate raw memory with `std::allocator` (or `::operator new`), put the objects into it with `std::construct_at`, and make sure the destructors get called.

You build `DynamicArray<T>`, a container like `std::vector` with amortized O(1) operations at the end, value semantics, insertion and deletion in the middle, and the strong exception guarantee for `insert_last`.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Basic operations and object lifetimes

```cpp
template <typename T>
class DynamicArray {
public:
    DynamicArray();

    ~DynamicArray();

    std::size_t size() const;
    std::size_t capacity() const;
    bool empty() const;

    const T& get_at(std::size_t i) const;
    void set_at(std::size_t i, T value);

    T& operator[](std::size_t i);
    const T& operator[](std::size_t i) const;

    void insert_last(T value);
    T delete_last();

    T* begin();
    T* end();
    const T* begin() const;
    const T* end() const;
};
```

| Operation | Behavior | Complexity |
| --- | --- | --- |
| `DynamicArray()` | An empty array; must not allocate memory | O(1) |
| `insert_last(v)` | Append at the end | Amortized O(1) |
| `delete_last()` | Delete and return the last element; throw `std::out_of_range` if the array is empty | Amortized O(1) |
| `size()`, `capacity()`, `empty()` | | O(1) |
| `get_at(i)`, `set_at(i, v)` | Throw `std::out_of_range` when out of range | O(1) |
| `operator[]` (both versions), `begin()`/`end()` | No range check | O(1) |

- Only the first `size()` slots hold objects: **the spare capacity must not contain any constructed object** (the tests check this step by step with `Tracked`).
- Deleted elements must actually be destroyed. Move-only elements (`std::unique_ptr`) must work too.

## Stage 2 — The amortized-analysis contract

`capacity()` isn't tied to any particular value, but it must satisfy:

1. Always `size() ≤ capacity() ≤ 4·size() + 16`.
2. **n consecutive `insert_last` calls move elements (copy + move) at most 5n times in total.**
3. However `insert_last` and `delete_last` alternate (especially right at the boundary where the array is exactly full or has just shrunk), repeated mass moving (thrashing) must never happen.

Lecture 2's approach: when the array is full, grow it to 2 times its size; shrink it by half only once `size` drops to 1/4 of the capacity. Think through why "shrink as soon as it drops to 1/2" doesn't work.

## Stage 3 — Value semantics and operations in the middle

```cpp
// in class DynamicArray<T>
DynamicArray(const DynamicArray& other);
DynamicArray& operator=(const DynamicArray& other);
DynamicArray(DynamicArray&& other);             // should be noexcept once implemented
DynamicArray& operator=(DynamicArray&& other);  // should be noexcept once implemented

void insert_at(std::size_t i, T value);
T delete_at(std::size_t i);
```

- Copy constructor and copy assignment (deep copy, strong guarantee); move constructor and move assignment (O(1), `noexcept`, and the moved-from object's `size()` is 0).
- `insert_at(i, v)`: insert before index `i`, `0 ≤ i ≤ size()`. When the capacity is sufficient, move only the elements after `i`.
- `delete_at(i)`: delete and return element `i`, again moving only the elements after `i`.
- If `i` is out of range, throw `std::out_of_range` and leave the contents unchanged.

## Stage 4 — Strong Exception Guarantee

`insert_last` must provide the strong guarantee, just like `std::vector::push_back`: **if an element's copy or move throws an exception partway through growing, the array's contents and `size()` stay exactly the same**, and nothing may leak.

- When growing, if the element's move constructor is `noexcept`, use move (otherwise growing degrades into copying, which is expensive); if move might throw, use copy instead (`std::move_if_noexcept`).
- Allocate the new memory before touching the old data, and build all the new elements before switching over in one step.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Use the accounting method to explain why doubling gives amortized O(1): how much "money" does each `insert_last` have to save up?
2. Why can't the shrink threshold be too close to the grow threshold? Give a concrete worst-case sequence of operations.
3. Why does `std::vector::insert` in the middle provide only the basic guarantee, while `push_back` at the end provides the strong one?
4. Why do some implementations grow by 1.5 times instead of 2? How does that relate to the allocator reusing old blocks?
