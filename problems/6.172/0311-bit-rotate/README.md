---
title: Bit Array and Fast Rotation
unit: 03 · Bit Hacks (Project 1)
source: 6.172 Project 1 — Bit Hacks
pdf: materials/6.172/projects/p1/handout.pdf
read: materials/6.172/lectures/lec03.pdf, materials/6.172/projects/p1/handout.pdf
kind: project
minutes: 300
---

# 0311 · Bit Array and Fast Rotation

**Original problem:** [Project 1: Bit Hacks](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/271aca8dd9fe0ff159ae31396a180f17_MIT6_172F18_project1.pdf) · **Read first:** [Bit Hacks](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/cc6983c9ebd77c28e8ae85bc0e575360_MIT6_172F18_lec3.pdf), [Project 1: Bit Hacks](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/271aca8dd9fe0ff159ae31396a180f17_MIT6_172F18_project1.pdf)

> **Why this comes up in practice:** a columnar database's bit-packed columns, compressed bitmap indexes, and bit shifting in a ring buffer all have to move ranges of bits at positions that "aren't byte-aligned." This exercise applies word-level parallelism (processing 64 bits at a time) where it really matters.

## Original problem

6.172 Project 1 ("Bit Hacks") is about word-level parallelism: treating a machine word as a vector of bits and operating on all of them at once. The starter program, `everybit`, stores a large bit array packed 8 bits per byte, with `bitarray_get()` and `bitarray_set()` for single bits. Its `bitarray_rotate()` rotates a substring of the array left or right by a given amount, but the prototype does it by repeating one-bit rotations, which is very slow. The project is to make `bitarray_rotate()` fast.

- **Rules:** standard C only, so no multithreading, inline assembly, intrinsics, or libraries built on them. The x86 machines are little-endian, and the code need not run on big-endian machines. Don't call `bitarray_new()` from your implementation (small stack or global buffers are fine), and don't change `main.c`, `tests.c`, or the signatures they call.
- **Correctness:** a rotation counts as correct when `bitarray_get_bit_sz()` and `bitarray_get()` see exactly the bits that the original slow version would leave.
- **Performance:** the `./everybit -s`, `-m`, and `-l` benchmarks keep enlarging the rotation by a constant factor until a single run exceeds 0.01, 0.1, or 1 second, respectively. Speed is judged by how far (which tier) a benchmark like `-l` gets, and how long its last tier takes.
- **Testing:** tests use a simple text format under `tests/`. Each team puts its 20 best in `tests/mytests`, and those tests run against every other team's code: the more bugs they catch, the higher the test-coverage part of the grade.
- **Approaches:** view the region to rotate as `ab` and turn it into `ba`. (1) Copy `a` to an auxiliary array, move `b` into place, and copy `a` back: simple, but the large auxiliary array hurts the cache on long strings. (2) A cyclic approach sends each bit straight to its final position with constant extra space, but it jumps all over memory. (3) Three reversals, based on the identity `(aᴿbᴿ)ᴿ = ba`, touch each bit twice and need no extra buffer. Other approaches may be competitive too.
- **Write-ups:** the beta and final releases each come with a short write-up that explains the design, how complete and fast it is, and any known bugs.

The original is a team project in C, benchmarked on x86 Linux with `awsrun`.

## Your task

The project has two files: `bit_array.hpp` holds `BitArray`, `bit_packed_vector.hpp` holds `BitPackedVector`, and `solution.hpp` includes both. Don't use built-ins like `__builtin_bitreverse64`; to reverse 64 bits at once, do it yourself with masks (see 0301).

As in the original, the core is a fast rotate on a bit array. Here it is a C++ class with bounds-checked accessors, a perf test compares it with a fixed bit-by-bit baseline, and the last stage uses the rotation to build a bit-packed column.

Each stage lists the declarations it adds, copied from those files.

## Stage 1 — BitArray basics

```cpp
class BitArray {
public:
    explicit BitArray(std::size_t bits);
    static BitArray from_string(std::string_view text);

    std::size_t size() const;
    bool get(std::size_t index) const;
    void set(std::size_t index, bool value);
    std::string to_string() const;
};
```

`BitArray(n)` (every bit 0), `size()`, `get(i)`, and `set(i, v)`; an out-of-range index throws `std::out_of_range`. `from_string("1011")` builds an array (character 0 of the string is index 0) and throws `std::invalid_argument` if the string contains any other character. `to_string()` converts back to a string.

## Stage 2 — Field access across words

```cpp
// in class BitArray
std::uint64_t read_bits(std::size_t offset, unsigned count) const;
void write_bits(std::size_t offset, unsigned count, std::uint64_t value);
```

- `read_bits(offset, count)`: with `count ∈ [0, 64]`, read `[offset, offset+count)` as one integer; the bit at `offset` becomes bit 0 of the result.
- `write_bits(offset, count, value)`: write the low `count` bits of `value` back, leaving every other bit unchanged.
- A range that goes past `size()` throws `std::out_of_range`, and so does `count > 64`. Don't let `offset + count` overflow: the tests also pass an `offset` of `SIZE_MAX`.

Everything after this builds on it. `count == 64` at an unaligned offset is the easiest case to get wrong.

## Stage 3 — A correct rotate

```cpp
// in class BitArray
void rotate(std::size_t offset, std::size_t length, std::ptrdiff_t right_amount);
```

`rotate(offset, length, right_amount)`: rotate the range `[offset, offset+length)`. `right_amount > 0` shifts it cyclically toward higher indices (each bit moves from `p` to `offset + ((p-offset+right_amount) mod length)`), and `< 0` shifts it toward lower indices. Every other bit stays unchanged. `right_amount` can be any `std::ptrdiff_t` value (including `PTRDIFF_MIN`, whose negation overflows). An out-of-range range throws `std::out_of_range` (the tests include an `offset` of `SIZE_MAX`, so watch for overflow).

This stage only has to be correct, so a simple O(length) method is fine.

## Stage 4 — Speeding it up with word-level parallelism

`rotate` must be at least **16×** faster than the fixed baseline (three `bit-by-bit` reversals made of `get`/`set`-level single-bit operations). The Project 1 handout lists three approaches (see Original problem): an auxiliary array, cyclic replacement, and three reversals (`(aᴿbᴿ)ᴿ = ba`). The reversals can be done at the word level: take 64 bits at a time, reverse them, and swap the two ends. Measure which approach is fastest yourself. A second `PERF` test also makes 100 large rotations of a 2^25-bit array within a time budget, so every rotation must stay close to linear time.

Hint: get `read_bits`/`write_bits` right first, and rotate is then just a combination of "reverse the bits" steps.

## Stage 5 — Real-world extension: Bit-packed column

```cpp
class BitPackedVector {
public:
    BitPackedVector(unsigned width, std::size_t capacity);

    std::size_t size() const;
    std::size_t capacity() const;
    unsigned width() const;

    std::uint64_t get(std::size_t index) const;
    void set(std::size_t index, std::uint64_t value);
    void push_back(std::uint64_t value);
    void insert(std::size_t index, std::uint64_t value);
    void erase(std::size_t index);
};
```

`BitPackedVector(width, capacity)`: a fixed-capacity column of integers in which each element takes `width` bits (`1..64`; an invalid width throws `std::invalid_argument`).

- `push_back(v)` / `get(i)` / `set(i, v)`: throw `std::invalid_argument` when `v` doesn't fit in `width` bits, `std::out_of_range` when an index is out of range, and `std::length_error` from `push_back` when the column is full.
- `insert(i, v)` (`0 <= i <= size()`) and `erase(i)` (`0 <= i < size()`): move the later elements at the bit level with `BitArray::rotate`. Do not move them one element at a time (a `PERF` test makes 2000 inserts at the front of a 400,000-element column). An index outside those ranges throws `std::out_of_range`, and `insert` into a full column throws `std::length_error`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Three reversals and cyclic replacement both use only O(1) extra space. Why is their cache behavior so different?
2. If you had to support big-endian machines, what would you change in your `read_bits`?
3. The handout makes test coverage a large part of the grade (see Original problem). Which 5 rotate test cases would you pick to catch other people's bugs the fastest?
