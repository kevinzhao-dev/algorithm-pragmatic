---
title: Reproducible Debugging Workflow (seed, replay, and shrink)
unit: 12 · Deterministic Execution (HW9)
source: 6.172 HW9 §2–§4 Checkoff 1–3 (environment dependencies, non-deterministic hashtable, replay debugging)
pdf: materials/6.172/homework/hw09/handout.pdf
read: materials/6.172/lectures/lec16.pdf
kind: code
minutes: 70
---

# 1211 · Reproducible Debugging Workflow

**Original problem:** [HW9: Deterministic Execution](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/ddcc1a4046b1893ca38211dc54be44fc_MIT6_172F18hw9.pdf) · **Read first:** [Nondeterministic Parallel Programming](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/e4fe97d2a250b57bb1e330fba8cf5992_MIT6_172F18_lec16.pdf)

> **Why this comes up in practice:** with an intermittent bug in production, the hard part isn't fixing it but reproducing it. Fold every input into a single seed and the failure reproduces reliably; then shrink the failing sequence of operations to the shortest one, and the bug is usually obvious at a glance. Property-based testing and fuzzers both follow this workflow.

## Original problem

6.172 HW9 (Deterministic Execution) §2–§4 tracks down sources of nondeterminism in serial C code and then debugs the bug they hide. The work is done in C with the homework's Makefile.

- §2, environment dependencies: a tiny program, `undef.c`, prints the value of an uninitialized local variable and that variable's address. Makefile targets rebuild and rerun it, run it with ASLR turned off (`setarch x86_64 -R`), and run it under different environment variables. You explain each difference, initialize the variable, and see that the environment block shifts the stack, so even stack addresses depend on the environment.
- §3, nondeterministic hashtable: `hashtable.c` is a mostly working open-addressing hash table with linear probing. One run usually succeeds; 10 or 1000 runs in a row expose occasional failures.
  - Checkoff 1: what does it take to make this program deterministic? Change the `hashtable1000-good` Makefile target so that every run succeeds.
  - Checkoff 2: change the `hashtable1-bad` target so that the program always fails (check which command-line arguments `hashtable.c` accepts).
- §4, replay debugging: use gdb's process record and reverse execution (record, reverse-next, reverse-continue, and watchpoints), or asserts and print statements, to find and fix the bug in `hashtable_insert`.
  - Checkoff 3: what was the bug, and what is your fix? Rerun `make hashtable1000` to confirm that the fix always works. The handout adds that the same bug may also hide in other functions written the same way.

## Your task

The homework's `hashtable` uses pointers returned by `malloc` as keys; add ASLR and environment variables, and the same executable gives a different result on every run. It also hides a bug where linear probing doesn't wrap around to the start, which only bites for some seeds (`support/buggy_table.hpp` has a version with the same bug; when the bug bites, `corrupted()` becomes true instead of actually writing out of bounds).

This exercise turns the homework's debugging workflow into code. An `Op` is one operation: `{Kind::Insert, key, value}` or `{Kind::Find, key, 0}`. Keys are never 0.

Instead of editing Makefile targets and stepping through gdb, you write each step of that workflow as a function, and you finish with a fixed table.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Make everything depend only on the seed (Checkoff 1)

```cpp
struct Op {
    enum class Kind { Insert, Find };
    Kind kind;
    std::uint64_t key;
    std::uint64_t value;
    friend bool operator==(const Op&, const Op&) = default;
};

std::ostream& operator<<(std::ostream& os, const Op& op);  // provided

std::vector<Op> generate_ops(std::uint64_t seed, std::size_t count);

template <typename Table>
std::vector<std::optional<std::uint64_t>> run_ops(Table& table, std::span<const Op> ops);
```

`generate_ops(seed, count)`: return `count` operations, **determined only by the seed** (`std::mt19937_64`): about 70% are `Insert` (a new random key, with a value that is not 0), and the rest are `Find` (a key inserted earlier; while nothing has been inserted yet, emit an `Insert` instead).

`run_ops(table, ops)`: execute them in order. `Table` provides `insert(key, value)` and `find(key)` (which returns `std::optional<std::uint64_t>`). Return the result of every `Find`.

The tests run it twice with different heap states and check that the results are identical. Your code must not depend on addresses, time, or the environment.

## Stage 2 — Find a seed that breaks it (Checkoff 2)

```cpp
using FailureCheck = std::function<bool(std::span<const Op>)>;

std::optional<std::uint64_t> find_failing_seed(std::uint64_t first, std::uint64_t last,
                                               std::size_t ops_per_run,
                                               const FailureCheck& fails);
```

`find_failing_seed(first, last, ops_per_run, fails)`: for each seed in `[first, last)`, from smallest to largest, generate an operation sequence; the first seed for which `fails(ops)` returns `true` is the answer. If no seed breaks it, return `std::nullopt`.

## Stage 3 — Shrink

```cpp
std::vector<Op> shrink_ops(std::vector<Op> ops, const FailureCheck& fails);
```

`shrink_ops(ops, fails)`: `ops` makes `fails` return `true`. Return a subsequence (keeping the relative order) that still fails and is as short as possible: the result must be **1-minimal**, which means that removing any single operation makes `fails` return `false`. If the `ops` passed in doesn't fail to begin with, return it unchanged.

## Stage 4 — Fix it (Checkoff 3)

```cpp
struct MultiplicativeHash {
    std::uint64_t operator()(std::uint64_t key) const { return key * 0x9E3779B97F4A7C15ULL; }
};

template <typename Hash = MultiplicativeHash>
class ProbingTable {
public:
    explicit ProbingTable(std::size_t capacity, Hash hash = Hash{});

    bool insert(std::uint64_t key, std::uint64_t value);
    std::optional<std::uint64_t> find(std::uint64_t key) const;
    std::size_t size() const;
};
```

`ProbingTable<Hash>(capacity)`: open addressing + linear probing, with a replaceable `Hash` (the default is a multiplicative hash; the tests swap in a version that crams every key into the same slot).

- `insert(key, value)`: return whether it succeeded; return `false` when the table is full. Inserting an existing key overwrites it.
- `find(key)`: return the value.
- `size()`.

The key point is that `insert` and `find` **both must** wrap around to the start (as the homework reminds you, the same bug often turns up again in another function that looks just like it).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. The homework's `undef.c`: how do an uninitialized variable, ASLR, and the length of the environment variables each affect the output? To make the program fully deterministic, which inputs would you eliminate?
2. Why can the tests still pass most of the time if you fix only `insert` and forget to fix `find`? How would you catch this kind of bug in code review?
3. Is Stage 3's 1-minimal the same as the shortest possible result? What does delta debugging do better than your Stage 3 approach?
