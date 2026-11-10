---
title: Deterministic Scheduler (turning races into reproducible bugs)
unit: 12 · Deterministic Execution (HW9)
source: 6.172 Lecture 16 Determinism; HW9 §4 replay concept; Recitation 9 Problem 4
pdf: materials/6.172/lectures/lec16.pdf
answer: materials/6.172/recitations/r9/solutions.pdf
read: materials/6.172/recitations/r9/problems.pdf
kind: code
minutes: 130
---

# 1201 · Deterministic Scheduler

**Based on:** [Nondeterministic Parallel Programming](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/e4fe97d2a250b57bb1e330fba8cf5992_MIT6_172F18_lec16.pdf) · **Read first:** [R9: Deterministic Execution Problems](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/edc0cd9509aecd107836c3fbd90bc86c_MIT6_172F18_rec9.pdf)

> **Why this comes up in practice:** the most painful thing about a race condition is that you can't reproduce it. Folly's `DeterministicSchedule`, Rust's `loom`, and CHESS on Windows all rest on the same idea: let the threads take turns, with a scheduler deciding who runs next, so that an interleaving becomes a sequence of decisions you can record, replay, and enumerate.

## Original problem

6.172 Recitation 9 Problem 4 is a multiple-choice question about two cores of a dual-core x86 processor, named Albert and Bernard. They run at the same time and share two volatile variables, x and y, which both start at 0:

- Albert checks whether x is 0 and, if it is, sets y to x + 1.
- Bernard does the mirror image: if y is 0, he sets x to y + 1.

Once both cores have finished, which final values of x and y are possible? Pick every option that can occur:

- (A) x = 0, y = 1
- (B) x = 0, y = 2
- (C) x = 1, y = 1
- (D) x = 1, y = 2
- (E) x = 2, y = 2

The exercise also draws on two other parts of the course:

- Lecture 16 calls a program deterministic on an input when every execution writes the same sequence of values to each memory location (different locations may still be updated in different orders). Its advice is to avoid nondeterministic parallel code, and where you can't, to plan a test strategy: switch the nondeterminism off, wrap it behind an interface, replace it with a deterministic version, or use analysis tools.
- HW9 §4 (replay debugging) has you record a run under gdb and step backward through it (reverse-next, reverse-continue, and watchpoints) to track down a bug in the homework's hash table.

## Your task

In this exercise you build a minimal version of such a scheduler yourself. The concurrent exercises from 1211 onward use the same technique (with the full version in `_shared/08_dsched.hpp`) to make their tests fully deterministic.

> ⚠️ **Spoiler warning:** `_shared/08_dsched.hpp` is the full solution to this exercise. Do not open it until you are done.

**Model:** each body runs on its own `std::thread`, but only one of them runs at any moment. A body hands over control when it calls `yield_point()`. There are two kinds of **decision points**: choosing the first thread at the start, and every `yield_point()`, thread exit, or block; each time, as long as **two or more** threads can still run, a decision must be made. `schedule` records which thread each decision picked. When it isn't a decision point (only one thread can run), nothing is recorded.

Instead of reasoning about Recitation 9 Problem 4 by hand, you build the machinery that runs every interleaving of such code, and the Stage 4 tests use it to list that problem's possible outcomes.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — A prescribed schedule

```cpp
struct RunOutcome {
    std::vector<int> schedule;  // id of the thread chosen at every point with a real choice
    bool deadlocked = false;
};

RunOutcome run_schedule(std::vector<std::function<void()>> bodies,
                        const std::vector<int>& choices);
void yield_point();
```

`run_schedule(bodies, choices)`: make the decisions with the ids in `choices`, in order; once they run out, always pick the smallest id. If `choices` names a thread that cannot run, throw `std::invalid_argument`. Return a `RunOutcome`.

An exception thrown inside a body must be rethrown unchanged when `run_schedule` returns to its caller (`PRACTICE_TODO` is one such exception).

## Stage 2 — Random runs and replay

```cpp
RunOutcome run_random(std::vector<std::function<void()>> bodies, std::uint64_t seed);
RunOutcome replay_schedule(std::vector<std::function<void()>> bodies,
                           const std::vector<int>& schedule);
```

- `run_random(bodies, seed)`: at each decision, pick uniformly at random among the runnable threads (`std::mt19937_64`; the same seed must give the same `schedule`).
- `replay_schedule(bodies, schedule)`: run again by following the record; the result must be exactly the same.

This is the point of the homework's replay debugging: once a failure can be replayed, you can investigate it at your own pace.

## Stage 3 — Blocking and deadlock detection

```cpp
class SchedMutex {
public:
    SchedMutex();
    SchedMutex(const SchedMutex&) = delete;
    SchedMutex& operator=(const SchedMutex&) = delete;

    void lock();
    bool try_lock();
    void unlock();
};
```

`SchedMutex`: in `lock()`, if the lock is free, take it right away (this is not a decision point); if someone else holds it, mark the caller **blocked** (not runnable) and hand control to another thread, and when the lock is released, wake the threads waiting for it (they compete for it again). After `unlock()` releases the lock, there is a decision point. `try_lock()` returns immediately, with `false` if it can't get the lock.

If no thread can run but some thread hasn't finished, that is a **deadlock**: set `RunOutcome::deadlocked = true`, and every thread must still finish (use an internal exception to clean up the blocked threads; the run must not hang).

## Stage 4 — Enumerating every interleaving

```cpp
struct Scenario {
    std::vector<std::function<void()>> bodies;
    std::function<bool()> check;
};

struct ExploreSummary {
    long runs = 0;
    long failures = 0;
    long deadlocks = 0;
    std::vector<int> bad_schedule;  // the first failing schedule, for replay
};

ExploreSummary explore_all(const std::function<Scenario()>& make);
```

`explore_all(make)`: each call to `make()` returns a fresh `Scenario{bodies, check}`. Run once for **every** possible schedule (enumerate the decision tree with DFS), and tally the results in an `ExploreSummary{runs, failures, deadlocks, bad_schedule}`: each time `check` returns `false` counts as one failure, and the first failing schedule must be stored in `bad_schedule`.

Two bodies of two segments each (with one yield in between) should give exactly C(4,2) = 6 schedules; the tests also enumerate every possible outcome of Recitation 9 Problem 4.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. This model assumes sequential consistency. Does Recitation 9 Problem 4 give the same results on x86? When would weak memory add new outcomes?
2. How fast does the number of schedules to enumerate grow? Why does CHESS's preemption bounding help?
3. Lecture 16 says "deterministic" programs are easier to debug. Why is Recitation 9 Problem 3 false: "if every concurrent access is protected by the same mutex, a program that is deterministic when run serially stays deterministic"?

---

**Official solution:** [R9 Solutions](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/c6c9289ff27bb530da2bf81dd96b8627_MIT6_172F18_rec9sol.pdf). Read it after you finish.
