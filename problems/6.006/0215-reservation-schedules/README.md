---
title: Resource Reservation Schedules (merge, divide and conquer, and an online version)
unit: 02 · Sorting & Recurrences (PS2)
source: 6.006 PS2 Problem 2-5
pdf: materials/6.006/psets/ps2/questions.pdf
answer: materials/6.006/psets/ps2/solutions.pdf
read: materials/6.006/lectures/lec03.pdf
kind: code
minutes: 100
---

# 0215 · Resource Reservation Schedules

**Original problem:** [Problem Set 2 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/70d9c6b212c6dfce6723259ffb429521_MIT6_006S20_ps2-questions.pdf) · **Read first:** [Lecture 3: Sorting notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6d1ae5278d02bbecb5c4428928b24194_MIT6_006S20_lec3.pdf)

> **Why this comes up in practice:** a cluster scheduler needs to know "how many machines this batch of jobs needs at once," which means merging the demand of a pile of intervals into one step function over time; that is the same trick as merge sort's merge. An LSM tree accumulates writes into layer after layer and then merges them, and that is exactly what Stage 3 does.

## Original problem

6.006 PS2 Problem 2-5, "Beaver Bookings," has Tim the Beaver running Tim Talks, a lecture series in which anyone in the MIT community can book a time to speak. The problem defines:

- A "talk request" (s, t) gives a talk's start time s and end time t, with s < t. Times are positive integers, counted in time units from a fixed starting point.
- A "room booking" (k, s, t) reserves k > 0 rooms from time s to time t, where s < t.
- Bookings (k1, s1, t1) and (k2, s2, t2) are "disjoint" when t1 ≤ s2 or t2 ≤ s1, and "adjacent" when t1 = s2 or t2 = s1.
- A "booking schedule" is an ordered tuple of room bookings that are pairwise disjoint, listed by increasing start time, and in which any two adjacent bookings reserve different numbers of rooms.

Every set R of talk requests has exactly one booking schedule B that "satisfies" it, meaning that B reserves exactly as many rooms as the talks need. For example, for R = {(2, 3), (4, 10), (2, 8), (6, 9), (0, 1), (1, 12), (13, 14)}, the satisfying schedule is B = ((1, 0, 2), (3, 2, 3), (2, 3, 4), (3, 4, 6), (4, 6, 8), (3, 8, 9), (2, 9, 10), (1, 10, 12), (1, 13, 14)); the original also draws this example on a timeline.

- (a) You are given the booking schedules B1 and B2 of two sets of talk requests R1 and R2, with n = |B1| + |B2|. Describe an O(n)-time algorithm that computes the booking schedule B for R = R1 ∪ R2.
- (b) Given a set R of n talk requests, describe an O(n log n)-time algorithm that returns the booking schedule that satisfies R.
- (c) Write `satisfying_booking(R)`, which takes a tuple of (s, t) talk requests and returns the satisfying booking schedule as a tuple of (k, s, t) triples, using your algorithm.

Implement part (c) in the Python template, which comes with a few test cases.

## Your task

A job occupies one machine over the half-open interval `[start, end)` (`start < end`). A **schedule** is a sequence of `Reservation{machines, start, end}` values, each meaning that `machines` machines are needed over `[start, end)` (`machines > 0`). A valid schedule satisfies three conditions: the intervals don't overlap, they are in increasing time order, and intervals that are adjacent and touch always have different `machines` (if they were equal, they should be merged into one piece). A period with no jobs simply has no reservation. Each set of jobs corresponds to exactly one valid schedule.

For example, the schedule for the jobs `[0,2) [1,3) [2,6)` is `(1,[0,1)) (2,[1,3)) (1,[3,6))`.

This is the original problem with jobs and machines in place of talks and rooms; Stage 3 adds an online version, which the original problem doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Merging two schedules

```cpp
struct Reservation {
    std::int64_t machines;
    std::int64_t start;
    std::int64_t end;
    friend bool operator==(const Reservation&, const Reservation&) = default;
    friend std::ostream& operator<<(std::ostream& os, const Reservation& r);  // provided
};

using Schedule = std::vector<Reservation>;

Schedule merge_schedules(const Schedule& a, const Schedule& b);
```

`merge_schedules(a, b)`: `a` and `b` are two valid schedules (the demand of two separate sets of jobs); return the schedule for both sets of jobs combined, in **O(|a| + |b|)**.

- Edge cases: either side is empty, the two sides start or end at the same time, one interval completely contains another, one piece of `a` spans several pieces of `b`, and pieces that touch without overlapping (adjacent intervals with the same number of machines must be merged).
- You should first work out when "the current time `t`" has to advance to the next event.

## Stage 2 — Divide and conquer

```cpp
struct JobRequest {
    std::int64_t start;
    std::int64_t end;
};

Schedule satisfying_schedule(std::span<const JobRequest> jobs);
```

`satisfying_schedule(jobs)`: `jobs` is a `std::span<const JobRequest>`; return the valid schedule in **O(n log n)**, using the structure of merge sort (with Stage 1's merge as the combine step). An empty `jobs` returns an empty schedule; a job with `start ≥ end` throws `std::invalid_argument`.

## Stage 3 — Real-world extension: online accumulation (the LSM approach)

```cpp
class IncrementalPlanner {
public:
    void add(JobRequest job);
    Schedule schedule() const;
    std::int64_t peak() const;
    std::size_t size() const;
};
```

`IncrementalPlanner`: jobs arrive one at a time, and you may be asked for the current schedule at any moment.

- `add(job)`: amortized **O(log n)**. Don't recompute everything every time (a `PERF` test checks this): keep several small schedules whose sizes grow geometrically (merge them the way a binary counter carries, which is exactly the LSM-tree approach). A job with `start ≥ end` throws `std::invalid_argument` and leaves the planner unchanged.
- `schedule()`: return the schedule of all the jobs so far, in O(n).
- `peak()`: the largest number of machines needed at the same time so far; O(n) is fine.
- `size()`: the number of jobs added.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why use divide and conquer for this problem instead of sorting all the endpoints and sweeping once? How do the complexity and the constant factors of the sweep-line method (sort + sweep) compare?
2. What is the correctness invariant of `merge_schedules`? Which events can `t` skip over in a single step?
3. If you turned Stage 3 into a real LSM tree, when would compaction be triggered? What are read amplification and write amplification?

---

**Official solution:** [Problem Set 2 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/b06b8a8baf9b15f98947e75bae55b8bf_MIT6_006S20_ps2-solutions.pdf). Read it after you finish.
