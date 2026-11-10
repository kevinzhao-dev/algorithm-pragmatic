---
title: Sanitizer Debugging and Crash Triage
unit: 01 · C++ Fundamentals and Tools (HW1)
source: 6.172 HW1 §5 Basic tools (Write-up 5–8)
pdf: materials/6.172/homework/hw01/handout.pdf
read: materials/6.172/homework/hw01/handout.pdf
kind: code
minutes: 120
---

# 0112 · Sanitizer Debugging and Crash Triage

**Original problem:** [HW1: Basic Tools, C Primer](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/2724d8594cb413754669fc4e9c6ce7db_MIT6_172F18hw1.pdf) · **Read first:** [HW1: Basic Tools, C Primer](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/2724d8594cb413754669fc4e9c6ce7db_MIT6_172F18hw1.pdf)

> **Why this comes up in practice:** one night of fuzzing in CI spits out 300 ASan reports that turn out to be only 3 bugs. Reading reports, classifying them, and reproducing them in lldb are basic survival skills for a C++ engineer. This exercise swaps HW1's GDB/Valgrind workflow for the tools this machine actually has.

## Original problem

6.172 HW1 §5 ("Basic tools") debugs a deliberately broken C matrix-multiply program from the homework repository on Linux:

- Build it with the provided Makefile, switch the optimization level from `-O1` to `-O3`, and rebuild. The program prints some output and then dies with a segmentation fault.
- Run it under GDB (`run`, then `bt`). The optimized build only names the crashing function, so rebuild with `make DEBUG=1` (debug symbols, `-O0`), rerun, and print variables at the faulting line: a row of `B` is a null pointer, so the matrix dimensions are wrong.
- Enable the commented-out `tbassert` dimension checks in `matrix_multiply.c`, rerun a debug build to get a readable assertion failure, and move to `main`'s stack frame with `frame` to print both dimensions. Then fix `testbed.c`, which builds the matrices, so that the program runs to completion.
- The printed product (`-p`) is still wrong; `-pz` multiplies zero matrices for comparison. Build with `make ASAN=1` and run under AddressSanitizer. Then run Valgrind on a fresh build, which reports a use of an uninitialized value while the result matrix is printed; fix the missing initialization in `matrix_multiply.c`.
- Valgrind's leak summary (use `--leak-check=full` for details) shows that matrices `A`, `B`, and `C` are never freed. Free them in `testbed.c` with `free_matrix` until Valgrind has nothing to report.
- Finally, add `-fprofile-arcs` and `-ftest-coverage` to the build, run `llvm-cov gcov`, and read the per-line execution counts: they reveal lines the testbed never runs and the hot triple loop in `matrix_multiply_run`. Then take the flags out again, because the instrumentation slows the program down and skews every timing.

The write-ups:

- **Write-up 5:** what `make clean; make` shows once the Makefile uses `-O3`.
- **Write-up 6:** the AddressSanitizer output about the memory bug.
- **Write-up 7:** the output of `./matrix_multiply -p` after the initialization fix, showing a correct product.
- **Write-up 8:** Valgrind output showing that no errors remain.

## Your task

`support/planted_bugs.hpp` has nine functions with deliberately planted bugs, all of them common slips. Reproduce them by hand before you start writing code:

```bash
D=problems/6.172/0112-sanitizer-bug-hunt
clang++ -std=c++20 -g -O0 -fsanitize=address,undefined -fno-sanitize-recover=undefined \
  -fno-omit-frame-pointer -I$D/support $D/lab/bugzoo.cpp -o /tmp/bugzoo
/tmp/bugzoo use-after-realloc      # read the ASan report: what kind of error, which line, who freed it
lldb -- /tmp/bugzoo overflow       # r, bt, frame select, frame variable, p, breakpoint set -n
leaks --atExit -- /tmp/bugzoo leak # Apple Silicon has no LeakSanitizer; find leaks with the leaks tool
```

Run every scenario once and make sense of its report. Mind the tools' blind spots: ASan can't catch reads of uninitialized memory (that takes MSan, which macOS doesn't have), so make a habit of RAII and `{}` initialization; on this machine, memory leaks have to be found with `leaks`.

Where the original walks one broken C program through GDB, Valgrind, and gcov, this exercise has you build the triage tooling itself: parse sanitizer reports, run code in a child process and capture its stderr, classify the planted bugs live, fix them, and group duplicate crashes.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Parsing a report

```cpp
enum class BugClass {
    None,
    Unknown,
    HeapUseAfterFree,
    HeapBufferOverflow,
    StackBufferOverflow,
    DoubleFree,
    SignedIntegerOverflow,
    ShiftOutOfRange,
    MisalignedAccess,
    NullDereference,
    OutOfBoundsIndex,
};

struct SanitizerReport {
    BugClass bug = BugClass::None;
    std::string tool;
    std::string access;
    std::size_t access_size = 0;
    std::string top_frame;
    std::string location;
};

SanitizerReport parse_report(std::string_view log);
```

`SanitizerReport parse_report(std::string_view log)` looks only at the **first** report in `log`:

| Field | Contents |
| --- | --- |
| `bug` | A `BugClass` (see the table below); `Unknown` if you can't tell, and `None` if there is no report at all |
| `tool` | `"AddressSanitizer"` or `"UndefinedBehaviorSanitizer"`; an empty string when there is no report |
| `access` / `access_size` | ASan's `READ of size 4` or `WRITE of size 1`; `""` / `0` when there is none |
| `top_frame` | The function name of ASan's first stack frame that "has a `file:line` location" (skip `__asan_memcpy` and the runtime's internal frames); an empty string for UBSan reports |
| `location` | `"file:line"`, without the directory or the column. ASan uses the same frame; UBSan uses the location at the start of the report |

| Report text (excerpt) | `BugClass` |
| --- | --- |
| ASan `heap-use-after-free` / `heap-buffer-overflow` / `stack-buffer-overflow` | `HeapUseAfterFree` / `HeapBufferOverflow` / `StackBufferOverflow` |
| ASan `attempting double-free` | `DoubleFree` |
| ASan `SEGV on unknown address`, address `< 4096` | `NullDereference` (otherwise `Unknown`) |
| UBSan `signed integer overflow` | `SignedIntegerOverflow` |
| UBSan `shift exponent`, `left shift of`, `right shift of` | `ShiftOutOfRange` |
| UBSan `misaligned address` | `MisalignedAccess` |
| UBSan containing `null pointer` | `NullDereference` |
| UBSan `out of bounds` | `OutOfBoundsIndex` |

## Stage 2 — Running code in a child process and capturing stderr

```cpp
struct CaptureResult {
    int exit_code = 0;
    int signal = 0;
    std::string output;
};

CaptureResult capture_stderr(std::function<void()> fn);
```

`CaptureResult capture_stderr(std::function<void()> fn)`: run `fn` in a child created with `fork`, and return `{exit_code, signal, output}`.

- A normal return (`fn` returns) counts as exit code 0; when a signal kills the child, `exit_code == -1` and `signal` is the signal number.
- When `fn` throws, the child's exit code is 1, and you **must not** let the exception propagate up into the parent's test flow (the child must `_exit` after `fork`).
- `output` is everything the child wrote to stderr (fd 2). The output can be long; even when it exceeds the pipe buffer, you must not deadlock.

## Stage 3 — Live triage

```cpp
SanitizerReport triage(std::function<void()> fn);
```

`SanitizerReport triage(std::function<void()> fn)`: run `fn` in a child process and parse its report. The tests really do set off the nine planted bugs under ASan/UBSan. When there is no error, return `BugClass::None`.

## Stage 4 — Fixing the bugs

```cpp
int first_after_push(std::vector<int>& values, int extra);
std::unique_ptr<int[]> clone_prefix(const int* src, std::size_t n);
std::size_t copy_truncated(const char* text, std::span<char> out);

class OwnedBuffer {
public:
    explicit OwnedBuffer(std::size_t n);
    ~OwnedBuffer();
    OwnedBuffer(const OwnedBuffer& other);
    OwnedBuffer& operator=(const OwnedBuffer& other);

    int* data();
    const int* data() const;
    std::size_t size() const;
};

long long sum_wide(const std::vector<int>& values);
std::uint32_t low_bits_mask_safe(int count);
std::uint32_t read_u32_safe(const std::uint8_t* buffer, std::size_t offset);
std::optional<int> second_value_safe(const planted::ListNode* head);
void record_bucket_safe(std::span<int> counts, int bucket);
```

Write the corrected versions; the same inputs must no longer trip a sanitizer (they are declared in `solution.hpp`):

- `first_after_push` (push `extra`, then return the old first element plus `extra`)
- `clone_prefix` (`std::unique_ptr<int[]>`; non-null even when `n == 0`)
- `copy_truncated` (a truncating copy that is always NUL-terminated and returns the full length, with `strlcpy` semantics; an empty `out` gets nothing written)
- `OwnedBuffer` (the rule of three with a deep copy; new elements start at 0, and self-assignment must be safe)
- `sum_wide` (the sum as a `long long`)
- `low_bits_mask_safe` (`count` can be anywhere from 0 to 32)
- `read_u32_safe`
- `second_value_safe` (returns `std::nullopt` when the list has fewer than two nodes, including when `head` is `nullptr`)
- `record_bucket_safe` (adds 1 to `counts[bucket]`; throws `std::out_of_range` when the bucket is out of bounds, leaving `counts` unchanged)

## Stage 5 — Real-world extension: Crash grouping

```cpp
struct CrashGroup {
    SanitizerReport report;
    std::size_t count = 0;
};

std::vector<CrashGroup> group_reports(std::span<const std::string> logs);
```

`group_reports(std::span<const std::string> logs)`: group the reports by the key `(bug, top_frame, location)` and return `CrashGroup{report, count}` entries (`report` is the group's first report), sorted by `count` from largest to smallest, with ties broken by `location` in lexicographic order. Skip logs that don't parse into a report (`None`).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why can't ASan catch a read of uninitialized memory? What is the only thing its shadow memory records?
2. A debug build (`-O0`) often hides bugs, and some bugs only show up at `-O2`. Give an example.
3. When is the code coverage (gcov) that HW1 has you do by hand more useful than a sanitizer?
