---
title: Bit Tricks Toolbox
unit: 03 · Bit Hacks (Project 1)
source: 6.172 Lecture 3 (Bit Hacks)
pdf: materials/6.172/lectures/lec03.pdf
read: materials/6.172/lectures/lec03.pdf
kind: code
minutes: 80
---

# 0301 · Bit Tricks Toolbox

**Based on:** [Bit Hacks](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/cc6983c9ebd77c28e8ae85bc0e575360_MIT6_172F18_lec3.pdf) · **Read first:** [Bit Hacks](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/cc6983c9ebd77c28e8ae85bc0e575360_MIT6_172F18_lec3.pdf)

> **Why this comes up in practice:** bitmap indexes, bit packing, hash table probing, and a scheduler's ready mask all rely on these tricks. When an interviewer asks "how do you compute popcount without a loop?", the answer isn't a memorized formula; it's knowing why it works.

## Your task

All the functions go in `solution.hpp`. This exercise practices the techniques, not calls into the standard library: don't use `std::popcount`, `std::countr_zero`, or `__builtin_*` (the tests use them as the reference). The tests can't catch a violation of the "no branch" requirement, so hold yourself to it: eliminate every conditional with arithmetic.

Lecture 3 presents most of these tricks; here you implement each one yourself, and the last stage adds two that come up in real code.

Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Masks and fields

```cpp
std::uint64_t set_bit(std::uint64_t x, unsigned k);
std::uint64_t clear_bit(std::uint64_t x, unsigned k);
std::uint64_t toggle_bit(std::uint64_t x, unsigned k);
std::uint64_t extract_field(std::uint64_t x, unsigned lo, unsigned len);
std::uint64_t insert_field(std::uint64_t x, unsigned lo, unsigned len, std::uint64_t value);
```

`set_bit`, `clear_bit`, `toggle_bit` (`k < 64`), `extract_field(x, lo, len)`, and `insert_field(x, lo, len, value)`.
`len` can be anywhere from `0` to `64`, and `lo + len <= 64`. `insert_field` changes only `[lo, lo+len)` and must ignore any extra high bits of `value`. Careful: when `len == 64`, don't use `1 << 64`.

## Stage 2 — No-branch

```cpp
std::int64_t min_no_branch(std::int64_t x, std::int64_t y);
std::int64_t abs_no_branch(std::int64_t x);
std::uint64_t mod_add_no_branch(std::uint64_t a, std::uint64_t b, std::uint64_t n);
void merge_no_branch(const int* a, std::size_t na, const int* b, std::size_t nb, int* out);
```

- `min_no_branch(x, y)`: must be correct over the whole range of `int64_t` (including the cases where `x - y` overflows).
- `abs_no_branch(x)`: `x != INT64_MIN` is guaranteed.
- `mod_add_no_branch(a, b, n)`: given `0 <= a, b < n`, return `(a + b) mod n`. `n` can be as large as `2^63`.
- `merge_no_branch(a, na, b, nb, out)`: merge two sorted arrays into `out`, with no branch in the comparison step.

## Stage 3 — Powers and counting

```cpp
std::uint64_t ceil_pow2(std::uint64_t x);
std::uint64_t lowest_set_bit(std::uint64_t x);
int count_trailing_zeros(std::uint64_t x);
int popcount_table(std::uint64_t x);
int popcount_parallel(std::uint64_t x);
std::uint64_t reverse_bits(std::uint64_t x);
```

| Function | Spec |
| --- | --- |
| `ceil_pow2(x)` | The smallest power of 2 that is not less than `x`; return `1` when `x == 0`, and `0` when `x > 2^63` |
| `lowest_set_bit(x)` | Keep only the lowest 1 bit; return `0` when `x == 0` |
| `count_trailing_zeros(x)` | `x != 0`; use a de Bruijn multiplication and a table lookup (Lecture 3) |
| `popcount_table(x)` | A 256-entry lookup table |
| `popcount_parallel(x)` | SWAR: add adjacent bits, then sum in pairs, then in fours, and so on |
| `reverse_bits(x)` | Reverse all 64 bits end to end, again with divide-and-conquer masks |

## Stage 4 — Real-world extensions

```cpp
class SetBits {
public:
    struct Iterator {
        unsigned operator*() const;
        Iterator& operator++();
        friend bool operator==(const Iterator& a, const Iterator& b);
    };

    explicit SetBits(std::uint64_t word);

    Iterator begin() const;
    Iterator end() const;
};

std::uint64_t next_with_same_popcount(std::uint64_t x);
```

- `SetBits`: a type you can use in a range-for. `for (unsigned i : SetBits{x})` yields the position of each 1 bit in order (from low to high), in time proportional to the number of 1s.
- `next_with_same_popcount(x)` (Gosper's hack): the smallest integer greater than `x` with the same `popcount`; return `0` when no such 64-bit integer exists. Enumerating `k`-subsets relies on it.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. What do `x & (x - 1)` and `x & -x` each do? What is `-x` at the bit level?
2. When does the `^` version of the no-temp swap set your data to 0?
3. Why can a de Bruijn sequence compress "which position holds the lowest 1 bit" into a 64-entry table?
