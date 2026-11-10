---
title: N-Queens from Serial to Parallel
unit: 05 · Fork-Join and Reducers (HW4)
source: 6.172 HW4 §3.1–3.2 (Write-up 1–6)
pdf: materials/6.172/homework/hw04/handout.pdf
read: materials/6.172/lectures/lec03.pdf, materials/6.172/lectures/lec06.pdf
kind: code
sanitizer: tsan
minutes: 140
---

# 0512 · N-Queens from Serial to Parallel

**Original problem:** [HW4: Cilk, Reducer Hyperobjects](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/b2ff5659ae776c6fa6834967f73a2ab4_MIT6_172F18hw4.pdf) · **Read first:** [Bit Hacks](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/cc6983c9ebd77c28e8ae85bc0e575360_MIT6_172F18_lec3.pdf), [Multicore Programming](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/cbef71a2b92bc700fbad43e69f951513_MIT6_172F18_lec6.pdf)

> **Why this comes up in practice:** backtracking search is the most common introduction to parallelization, and "accumulating results into a shared list" is the most common first race. The fix is always the same trick: each parallel branch accumulates on its own, and the results are merged after the join.

## Original problem

6.172 HW4 §3 revisits the N-queens problem from Lecture 3: place N queens on an N × N board so that no two share a row, a column, or a diagonal. The provided recursive `queens.c` finds every solution for N = 8 (there are 92; N = 16 already has 14772512) and appends each one to a linked list, `board_list`; an 8 × 8 board fits in the bits of a `uint64_t`. Where the lecture's `down`, `left`, and `right` bit vectors have sizes N, 2N − 1, and 2N − 1, this code uses three N-bit vectors, a trick from a 1991 Usenet post by Tony Lezard. In those vectors, 0 bits mark available squares and 1 bits mark unavailable ones.

- **Write-up 1 (bonus):** explain how that backtracking code works, and why N bits per vector are enough.
- §3.1: time the serial program, then add `cilk_spawn` and `cilk_sync` to the recursive calls and run it in parallel.
  - **Write-up 2:** what happens when you run the parallel version, and whether parallelization did what you expected.
  - **Write-up 3:** whether Cilksan detects the problem; describe the race in words and give the line numbers of the read and the write involved.
- §3.2: remove the parallelism, then pass each recursive call its own temporary list and concatenate them all into `board_list` after the calls synchronize. This needs `merge_lists`, which moves the second list onto the end of the first and leaves the second empty; it must handle empty lists and keep all three fields of the list (head, tail, and size) up to date. Then re-parallelize and check with Cilksan.
  - **Write-up 4:** the most efficient way to concatenate two singly linked lists, and the asymptotic running time of your `merge_lists`.
  - **Write-up 5:** explain your implementation, and compare the parallel code's performance with the serial code's.
  - **Write-up 6:** coarsen the recursion so that a base case falls back to the original serial code; say which base case you used, and compare the coarsened version with the uncoarsened parallel one and with the serial one.

The original is C with Cilk.

## Your task

You build the same pieces in C++: a list with an O(1) merge, a bitboard backtracking search for any `n <= 16`, and a race-free parallel search in which every branch collects into its own list. `exec::Meter` measures the work and spawns, and ThreadSanitizer takes Cilksan's place.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — ResultList

```cpp
template <typename T>
class ResultList {
public:
    ResultList() = default;
    ~ResultList();

    ResultList(const ResultList&) = delete;
    ResultList& operator=(const ResultList&) = delete;

    ResultList(ResultList&& other);
    ResultList& operator=(ResultList&& other);

    void push_back(T value);
    void merge(ResultList& other);
    std::size_t size() const;
    bool empty() const;
    std::vector<T> to_vector() const;
};
```

`template <typename T> class ResultList`: a singly linked list that owns its nodes.

| Interface | Requirement |
| --- | --- |
| `push_back(v)` | O(1) |
| `merge(other)` | O(1): append all of `other`'s nodes to the end; `other` becomes empty |
| `size()` / `empty()` | O(1) |
| `to_vector()` | Copy the elements out, in order |
| Destructor | Free every node, without recursion |
| Moves | Support move construction and move assignment; the source becomes empty |

Edge cases: `merge` with itself or with an empty list, and merging elements into an empty list. Merging a list into itself must leave it unchanged.

## Stage 2 — Bitwise backtracking

```cpp
using Board = std::uint64_t;

int board_column(Board board, int row);
Board board_place(Board board, int row, int column);
ResultList<Board> queens_serial(int n);
```

Pack the board into a `std::uint64_t` (Bentley's packing): the column of the queen in row `r` occupies bits `[4r, 4r+4)`, so `n <= 16`. With `using Board = std::uint64_t;`, implement both `board_column(b, r)` and `board_place(b, r, c)`.

Use Lecture 3's three `n`-bit vectors (`down`, `left`, and `right`; `left`/`right` shift one position left/right for each row you go down, keeping only `n` bits): the available positions are `~(down|left|right) & all`, and `x & -x` takes the lowest one. Remember to trim the bits beyond `n`.

`queens_serial(n)` (`1 <= n <= 16`) returns a `ResultList<Board>` of all the solutions.

## Stage 3 — A race-free parallel version

```cpp
template <typename Exec>
ResultList<Board> queens_parallel(Exec& exec, int n, int serial_depth);
```

`queens_parallel(exec, n, serial_depth)`: `exec` is an `exec::Meter` or an `exec::ForkJoin` (`_shared/01_exec.hpp`).

- Each parallel branch puts its solutions into **its own** `ResultList`, and the lists are `merge`d into one result only after `fork2` returns (HW4's temporary-list strategy).
- Nodes with `row < serial_depth` use `fork2` (split the available queen positions into two halves and recurse on each); nodes with `row >= serial_depth` use ordinary recursion (coarsening).
- `exec.charge(1)` for every search node visited (every call of the backtracking function, including the leaves where `row == n`).
- With `exec::Meter` the run is fully reproducible; with `exec::ForkJoin`, TSan guarantees that there is no race.

## Stage 4 — The cost of coarsening (Write-up 6)

With `serial_depth = 0` (fully serial), `spawns()` is 0; with `serial_depth = 1`, only the first row forks (`n-1` times); and every depth finds the same set of solutions.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Write-up 1: why are `n` bits enough for `left`/`right`, rather than `2n-1`?
2. Why does concatenating two singly linked lists need a tail pointer? Without one, what does a merge cost, and how does that affect the work of the parallel version? (HW4 Write-up 4)
3. When the parallel version is slower than the serial one, what is the usual cause? How would you measure it?
