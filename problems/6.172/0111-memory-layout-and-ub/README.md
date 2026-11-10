---
title: Memory Layout, Object Lifetime, and UB
unit: 01 · C++ Fundamentals and Tools (HW1)
source: 6.172 HW1 §4 C Primer (Write-up 2–4)
pdf: materials/6.172/homework/hw01/handout.pdf
read: materials/6.172/homework/hw01/handout.pdf
kind: code
minutes: 120
---

# 0111 · Memory Layout, Object Lifetime, and UB

**Original problem:** [HW1: Basic Tools, C Primer](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/2724d8594cb413754669fc4e9c6ce7db_MIT6_172F18hw1.pdf) · **Read first:** [HW1: Basic Tools, C Primer](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/2724d8594cb413754669fc4e9c6ce7db_MIT6_172F18hw1.pdf)

> **Why this comes up in practice:** forget to reorder the fields of a struct that stores tokens, and the whole corpus takes 10% more memory; a single `(a + b) / 2` can be the root cause of a production crash. This exercise rewrites the C primer's `sizeof`, pointers, and `swap` as C++ that UBSan catches in the act.

## Original problem

6.172 HW1 §4 is a short C primer built around a few small C programs in the homework repository. Along the way it has you:

- run the preprocessor (`clang -E`) on a sample file with and without `-DNDEBUG`, and watch the debug-only code disappear;
- edit `sizes.c` to print the size of each of these types: `int`, `short`, `long`, `char`, `float`, `double`, `unsigned int`, `long long`, `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`, `uint_fast8_t`, `uint_fast16_t`, `uintmax_t`, `intmax_t`, `__int128` (a Clang extension), `int[]` (the array `x` in the starter code), and `student` (a struct of two `int` fields);
- compile `pointer.c`, which declares `char` pointers with `const` in each possible position and then tries a series of assignments, and comment out the ones that fail to compile;
- see why a `swap` that takes its two `int` arguments by value leaves the caller's variables unchanged.

The write-ups:

- **Write-up 2:** answer the questions in the comments of `pointer.c`, such as why some of the assignments are legal and others are not.
- **Write-up 3:** for every type from the `sizes.c` exercise, also print the size of a pointer to that type, and report both sizes.
- **Write-up 4:** rewrite `swap()` in `swap.c` to take pointers, change `main()` so that the call really swaps, and check `sizes.c` and `swap.c` with the provided Python verifier.

The original is plain C, built with the homework's Makefile.

## Your task

This exercise's tests all run under ASan/UBSan. **Any undefined behavior aborts the program immediately**, so "it seems to work" doesn't count as passing.

Instead of printing sizes and swapping through pointers, you build what those exercises are about: compute struct layouts by the language's rules, hand out aligned memory with pointer arithmetic, manage object lifetimes in raw storage, and write integer helpers (an XOR swap among them) that never trigger UB.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Struct layout calculator

```cpp
struct FieldSpec {
    std::size_t size;
    std::size_t align;
};

struct StructLayout {
    std::vector<std::size_t> offsets;
    std::size_t size = 0;
    std::size_t align = 1;
};

StructLayout compute_layout(std::span<const FieldSpec> fields);
std::size_t padding_bytes(std::span<const FieldSpec> fields);
std::vector<std::size_t> packed_order(std::span<const FieldSpec> fields);
```

Given a list of fields (`FieldSpec{size, align}`, where `align` is a power of 2 and `size` is a multiple of `align`), compute their offsets by the C++ rules: each field goes at the current cursor rounded up to its `align`; the struct's alignment is the largest field alignment; and the total size is rounded up to a multiple of the struct's alignment.

| Interface | Behavior |
| --- | --- |
| `compute_layout(fields)` | Return `StructLayout{offsets, size, align}`; with no fields, `size == 1` and `align == 1` (even an empty struct has a size); throw `std::invalid_argument` when a `size` is not a multiple of its `align` |
| `padding_bytes(fields)` | The total size minus the sum of all the fields' `size`; throw `std::invalid_argument` on the same invalid input as `compute_layout` |
| `packed_order(fields)` | Return a permutation of `0..n-1` such that laying the fields out in that order gives the smallest total size from `compute_layout` |

## Stage 2 — Pointer arithmetic: Bump Arena

```cpp
class BumpArena {
public:
    explicit BumpArena(std::span<std::byte> buffer);
    void* allocate(std::size_t bytes, std::size_t alignment);
    void reset();
    std::size_t used() const;       // bytes consumed so far, including padding
    std::size_t capacity() const;
};
```

- Throw `std::invalid_argument` when `alignment` is not a power of 2 (0 included).
- The returned pointer must satisfy `alignment`, and `[p, p + bytes)` must lie entirely inside `buffer`. If the request doesn't fit, return `nullptr` and **leave the internal state unchanged** (a smaller allocation afterwards must still succeed).
- `bytes == 0` must also return a valid, aligned pointer.
- `reset()` frees the whole buffer at once: `used()` goes back to 0, and the next allocation starts again at the beginning of `buffer`.
- When `bytes` is close to `SIZE_MAX`, you must not let the pointer addition overflow and produce a position that "looks like it fits" (that overflow is UB in itself, and UBSan catches it).

## Stage 3 — Object lifetime: StaticVector

```cpp
template <typename T, std::size_t N>
class StaticVector {
public:
    StaticVector();
    ~StaticVector();

    StaticVector(const StaticVector& other);
    StaticVector& operator=(const StaticVector& other);
    StaticVector(StaticVector&& other);
    StaticVector& operator=(StaticVector&& other);

    std::size_t size() const;
    static constexpr std::size_t capacity() { return N; }  // provided
    bool empty() const;

    T* data();
    const T* data() const;
    T* begin();
    T* end();
    const T* begin() const;
    const T* end() const;

    T& operator[](std::size_t index);
    const T& operator[](std::size_t index) const;

    T& push_back(const T& value);
    T& push_back(T&& value);
    template <typename... Args>
    T& emplace_back(Args&&... args);
    void pop_back();
    void clear();
};
```

Build a `StaticVector<T, N>` with a fixed capacity `N` (`N >= 1`) that keeps its elements in raw storage inside the object. You **must not** declare `T data_[N]` (that default-constructs N elements right away, and it can't hold a type that has no default constructor).

The interface is the one in `solution.hpp`, shown above. Behavior:

- `push_back` / `emplace_back` throw `std::length_error` when the vector is full, and return a reference to the new element.
- Misusing `pop_back` / `operator[]` (such as calling `pop_back` when the vector is empty) throws `std::out_of_range`. `data()` and the iterators must be contiguous `T*` pointers.
- When an element's constructor throws: nothing leaks and `size` doesn't change (a failed `emplace_back` is as if it never happened).
- When the copy constructor throws partway through, the elements already copied must be destroyed (RAII).
- After move construction or move assignment, the source is empty (`size() == 0`).
- A copy is independent of the original, and copy assignment replaces the contents (self-assignment included).
- The destructor must destroy every live element; for an over-aligned type (`alignas(64)`), `data()` must be aligned as well.

Hint: `std::construct_at`, `std::destroy_at`, and `std::launder`.

## Stage 4 — Integer arithmetic without UB

```cpp
std::optional<int> checked_add(int a, int b);
std::optional<int> checked_mul(int a, int b);
int midpoint_floor(int a, int b);
std::uint32_t rotl32(std::uint32_t x, int k);
std::int32_t sign_extend(std::uint32_t value, int bits);
std::uint32_t load_le32(const std::byte* p);
void store_le32(std::byte* p, std::uint32_t value);
void no_temp_swap(int& a, int& b);
```

The most obvious way to write each of these is undefined behavior:

| Function | Spec |
| --- | --- |
| `checked_add(a, b)` / `checked_mul(a, b)` | Return `std::nullopt` on overflow |
| `midpoint_floor(a, b)` | Return the true value of `⌊(a + b) / 2⌋` for every pair of `int`s, without overflowing |
| `rotl32(x, k)` | For any `int` value of `k`: rotate left by `k mod 32` bits (a negative value rotates right); `k == 0` and multiples of 32 must return `x` |
| `sign_extend(value, bits)` | Treat the low `bits` bits of `value` (`1 <= bits <= 32`) as two's complement and sign-extend them to `int32_t` |
| `load_le32(p)` / `store_le32(p, v)` | `p` may be unaligned; always little-endian |
| `no_temp_swap(a, b)` | XOR swap, **correct even when `&a == &b`** |

## Interview follow-ups

Once you're done, answer without looking at your code:

1. What is the difference between `char* const`, `const char*`, and `char const*`? Why can't `char**` be implicitly converted to `const char**`?
2. After you pass `int x[5]` to `void f(int a[])`, what is `sizeof(a)`? That's array decay; how does C++ avoid it with `std::span`?
3. Why is signed integer overflow UB when unsigned overflow isn't? How does the compiler take advantage of this (give an example where it optimizes one of your checks away because of it)?
