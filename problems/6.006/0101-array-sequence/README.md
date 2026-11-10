---
title: Static Array and Array Sequence
unit: 01 · Sequences (PS1)
source: 6.006 Lecture 1, Recitation 1-2
pdf: materials/6.006/recitations/rec02.pdf
read: materials/6.006/lectures/lec01.pdf, materials/6.006/lectures/lec02.pdf, materials/6.006/recitations/rec01.pdf
kind: code
minutes: 70
---

# 0101 · Static Array and Array Sequence

**Based on:** [Recitation 2 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/c08a3b63dfe5f6f6b32257d35f86ae63_MIT6_006S20_r02.pdf) · **Read first:** [Lecture 1: Introduction notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/477c78e0af2df61fa205bcc6cb613ceb_MIT6_006S20_lec1.pdf), [Lecture 2: Data Structures notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/79a07dc1cb47d76dae2ffedc701e3d2b_MIT6_006S20_lec2.pdf), [Recitation 1 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/c6d8f06c6f11e3342633dec85498f551_MIT6_006S20_r01.pdf)

> **Why this comes up in practice:** underneath, `std::vector` is just a contiguous block of memory plus a "length." In the RAM model, indexed access is O(1) because the address can be computed, while inserting in the middle is O(n) because every element after it has to move. This exercise has you build that yourself, from the lowest level up.

## Your task

Manage the memory yourself: **do not use `std::vector`** as the underlying container.

You build a fixed-length `StaticArray` that owns its elements, implement the Lecture 2 Sequence interface on top of it as `ArraySequence`, and finish with the first algorithm from Lecture 1.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — StaticArray (Rule of Five)

```cpp
template <typename T>
class StaticArray {
public:
    explicit StaticArray(std::size_t n = 0);

    ~StaticArray();

    StaticArray(const StaticArray& other);
    StaticArray& operator=(const StaticArray& other);

    StaticArray(StaticArray&& other);             // should be noexcept once implemented
    StaticArray& operator=(StaticArray&& other);  // should be noexcept once implemented

    std::size_t size() const;

    const T& get_at(std::size_t i) const;
    void set_at(std::size_t i, T value);

    T& operator[](std::size_t i);
    const T& operator[](std::size_t i) const;

    T* begin();
    T* end();
    const T* begin() const;
    const T* end() const;
};
```

`StaticArray<T>`: fixed length, and it owns its elements (`T` can be default-constructed and assigned).

| Operation | Behavior |
| --- | --- |
| `explicit StaticArray(n)` | `n` value-initialized elements (0 for `int`) |
| `size()` | The length, O(1) |
| `get_at(i)`, `set_at(i, x)` | O(1); throw `std::out_of_range` if `i` is out of range |
| `operator[]` (`const` and non-`const`) | O(1), no range check |
| `begin()` / `end()` | Plain pointers are enough (so that range-for works) |

Resource management:

- The destructor frees every element. Copying is a deep copy; copy assignment must provide the **strong guarantee** (if copying an element throws an exception, the original object is unaffected).
- Move must be O(1) and must not touch any element (just steal the pointer), it must be `noexcept`, and a moved-from object's `size()` is 0.
- Self-assignment `a = a` must work correctly.

## Stage 2 — ArraySequence

```cpp
template <typename T>
class ArraySequence {
public:
    ArraySequence();
    explicit ArraySequence(std::span<const T> items);

    std::size_t size() const;
    const T& get_at(std::size_t i) const;
    void set_at(std::size_t i, T value);

    void insert_at(std::size_t i, T value);
    T delete_at(std::size_t i);

    void insert_first(T value);
    void insert_last(T value);
    T delete_first();
    T delete_last();

    T* begin();
    T* end();
    const T* begin() const;
    const T* end() const;
};
```

Use Stage 1's `StaticArray` to implement the Sequence interface from Lecture 2. The array is always exactly full, so every insertion or deletion must **reallocate** and move the elements:

| Operation | Behavior |
| --- | --- |
| `ArraySequence()` / `explicit ArraySequence(span<const T>)` | Empty / copy-construct, O(n) |
| `size()`, `get_at(i)`, `set_at(i, x)` | O(1); throw `std::out_of_range` when out of range |
| `insert_at(i, x)` | Insert before index `i`, `0 ≤ i ≤ size()`, O(n) |
| `delete_at(i)` | Delete and return element `i`, O(n) |
| `insert_first` / `insert_last` / `delete_first` / `delete_last` | Built from the operations above |
| `begin()` / `end()` | Iteration |

`insert_at` and `delete_at` must provide the strong guarantee: if copying an element throws an exception, or the index is invalid, the sequence's contents stay unchanged. `ArraySequence` itself should not need a custom destructor or copy / move operations (Rule of Zero).

## Stage 3 — The first algorithm from Lecture 1

```cpp
template <typename Record, typename KeyOf>
std::optional<std::pair<std::size_t, std::size_t>> first_collision(std::span<const Record> records,
                                                                   KeyOf key_of);
```

`first_collision(records, key_of)`: check the records in order and return `(i, j)`, where `j` is the **smallest** index whose record has the same key as some earlier record, and `i` is the earliest of those earlier records; when there are no duplicates, return `std::nullopt`.

Use Stage 1's `StaticArray` to keep a record of what you have seen. O(n²) is fine (sorting and hashing come in later chapters).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does `std::vector`'s `operator[]` skip the range check while `at()` performs one? What is the impact on performance?
2. Why is `insert_at` O(n) here, while switching to a dynamic array (0103) makes insertion at the end amortized O(1)? Where does the difference come from?
3. Use induction to show that the loop in `first_collision` is correct (hint: whether there is a collision among the first k records).
