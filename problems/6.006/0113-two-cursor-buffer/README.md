---
title: Two-Cursor Line Buffer (Binder Bookmarks)
unit: 01 · Sequences (PS1)
source: 6.006 PS1 Problem 1-3
pdf: materials/6.006/psets/ps1/questions.pdf
answer: materials/6.006/psets/ps1/solutions.pdf
read: materials/6.006/lectures/lec02.pdf, materials/6.006/recitations/rec02.pdf
kind: code
minutes: 90
---

# 0113 · Two-Cursor Line Buffer

**Original problem:** [Problem Set 1 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/dc56bd1995baf20cfcdd3fbabdd2dbc9_MIT6_006S20_ps1-questions.pdf) · **Read first:** [Lecture 2: Data Structures notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/79a07dc1cb47d76dae2ffedc701e3d2b_MIT6_006S20_lec2.pdf), [Recitation 2 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/c08a3b63dfe5f6f6b32257d35f86ae63_MIT6_006S20_r02.pdf)

> **Why this comes up in practice:** a text editor's gap buffer (Emacs) keeps the "gap" wherever the cursor is, so that edits are O(1). Collaborative editors and the timelines of non-linear video editors also have an operation that "moves a clip between two markers," while still needing O(1) reads by line number.

## Original problem

6.006 PS1 Problem 1-3, "Binder Bookmarks," is about Sisa Limpson, a tidy second grader who keeps her course notes as loose pages in a three-ring binder. With n pages, the first page has index 0 and the last has index n − 1. She often reorders pages while studying, and uses two bookmarks, A and B, to keep her place.

Describe a database that tracks the pages in the binder and supports the operations below, where n is the number of pages at the time of the operation. For each operation, say whether your running time is worst-case or amortized. You may assume that both bookmarks are placed before any shift or move happens, and that A always stays at a lower index than B.

- `build(X)`: initialize the database with the pages from iterator X, in O(|X|) time.
- `place_mark(i, m)`: put bookmark m ∈ {A, B} into the gap between page i and page i + 1, in O(n) time.
- `read_page(i)`: return the page at index i, in O(1) time.
- `shift_mark(m, d)`: bookmark m ∈ {A, B} sits in front of page i; move it to sit in front of page i + d, where d ∈ {−1, 1}, in O(1) time.
- `move_page(m)`: take the page directly in front of bookmark m ∈ {A, B} and relocate it to directly in front of the other bookmark, in O(1) time.

The answer is written; there is no code.

## Your task

The original problem manages the pages of a binder with bookmarks. Here it becomes an editor's line buffer: `TwoCursorBuffer<T>` stores a sequence of lines (items), with two cursors, A and B, sitting between lines.

**Positions are gaps:** gap `g` (`0 ≤ g ≤ n`) is the slot "in front of" line `g`, and `g == n` means after the last line. A cursor's "previous line" is the line at index `g − 1`. Rules: **once both cursors are placed, A must be strictly before B** (`gap(A) < gap(B)`).

Every invalid operation (an index out of range, an operation before the cursors are placed, an operation that would break the order) throws `std::logic_error` (or a subclass), and leaves both the contents and the cursors unchanged.

You build the buffer, its single-step cursor shifts, and the amortized O(1) `move_item` from the original problem, then add editing at the cursors, which the original problem doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Building and reading

```cpp
enum class Cursor { A, B };

template <typename T>
class TwoCursorBuffer {
public:
    explicit TwoCursorBuffer(std::span<const T> items);

    std::size_t size() const;
    const T& read(std::size_t i) const;
    void place_cursor(Cursor c, std::size_t gap);
    std::size_t cursor(Cursor c) const;
};
```

| Operation | Behavior | Complexity |
| --- | --- | --- |
| `TwoCursorBuffer(span<const T>)` | Build in order, with neither cursor placed yet | O(n) |
| `size()` | The number of lines | O(1) |
| `read(i)` | Line `i` (indices are unaffected by the cursors); throw `std::out_of_range` when out of range | **O(1)** |
| `place_cursor(c, g)` | Put cursor `c` at gap `g` (you may call it again to move the cursor); `c` is `Cursor::A` or `Cursor::B` | O(n) |
| `cursor(c)` | The current gap; throw `std::logic_error` if the cursor hasn't been placed yet | O(1) |

## Stage 2 — `shift_cursor(c, d)`

```cpp
// in class TwoCursorBuffer<T>
void shift_cursor(Cursor c, int direction);
```

Move the cursor by one position: `d` can only be `+1` or `−1`, in **O(1)**. The order of the lines themselves doesn't change, and neither do the results of `read`. It can only be used once both cursors are placed; it must not let A and B cross or collide, and it must not go past either end.

## Stage 3 — `move_item(c)`

```cpp
// in class TwoCursorBuffer<T>
void move_item(Cursor c);
```

Move the **line before** cursor `c` to **just in front of the other cursor**, in **amortized O(1)**:

- `move_item(A)`: move the line before A to just in front of B. The line before A disappears, so `gap(A)` decreases by 1 and `gap(B)` stays the same.
- `move_item(B)`: move the line before B to just in front of A; `gap(A)` increases by 1 and `gap(B)` stays the same.
- It is an invalid operation when there is no line in front of the cursor, or when the move would make `gap(A) == gap(B)` (which can only happen with `move_item(B)`).

Hint: only `read` needs O(1) random access, so you can think of the sequence as three segments (before A, between A and B, after B) and keep each one in a suitable container. **The tests count element moves with `Probe`, and use `PERF` to rule out brute-force solutions.**

## Stage 4 — Real-world extension: editing at the cursors

```cpp
// in class TwoCursorBuffer<T>
void insert_before_cursor(Cursor c, T item);
T delete_before_cursor(Cursor c);
```

- `insert_before_cursor(c, x)`: insert a line in front of cursor `c`, and `gap(c)` increases by 1; if `c` is A, `gap(B)` must increase by 1 as well.
- `delete_before_cursor(c)`: delete and return the line before cursor `c`, following the same rules as above (it must not make `gap(A) == gap(B)`, and it is invalid when there is no line in front of the cursor).

Both operations must also be **amortized O(1)**.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why doesn't a linked list work for this exercise? What do the O(1) requirement on `read(i)` and the O(1) requirement on `move_item` each force you to choose?
2. The official solution uses one array of length 3n that leaves two gaps. Compared with "two stacks plus a deque," what are the pros and cons of each design (constant factors, caching, how often it rebuilds)?
3. For an amortized O(1) `move_item`: how much does one rebuild cost? What triggers it, and why must there be Ω(n) operations between two consecutive rebuilds?

---

**Official solution:** [Problem Set 1 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/10e4e3444c1bde4c662a47a8f14dadd3_MIT6_006S20_ps1-solutions.pdf). Read it after you finish.
