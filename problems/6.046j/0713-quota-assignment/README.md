---
title: Quota Assignment (fewest rejections and incremental reassignment)
unit: 07 · Network Flow (PS7)
source: 6.046J PS7 Problem 7-3
pdf: materials/6.046j/psets/ps07/questions.pdf
answer: materials/6.046j/psets/ps07/solutions.pdf
read: materials/6.046j/lectures/lec14a.pdf
kind: code
minutes: 110
---

# 0713 · Quota Assignment

**Original problem:** [Problem Set 7](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/93951baaee86c66b1f2fd48e2c493745_MIT6_046JS15_pset7.pdf) · **Read first:** [Incremental Improvement: Matching](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/bdafe05f4f3c20cabfd08d2fbd099f63_MIT6_046JS15_lec14A.pdf)

> **Why this comes up in practice:** handing out GPU-model quotas to training jobs, ad inventory to campaigns, and seats to passengers. Each request accepts several kinds of resources, each resource has a quota, and the goal is to reject as few requests as possible. This exercise covers both the offline batch version and incremental reassignment.

## Original problem

6.046J PS7 Problem 7-3 ("Food Truck Orders") is about a food truck that makes m kinds of lunch items b1, …, bm, with exactly qj portions of item bj. Before lunch, n customers a1, …, an each text in a set Ai of one or more items they would accept. Every customer must be given either one of their acceptable items or a $10 voucher, and the truck wants to give out as few vouchers as possible. Give an efficient algorithm for this assignment; the hint is to model it as a max-flow problem.

## Your task

`accepts[i]` lists the resource types that request i accepts (`0..m-1`, possibly empty), and `quota[j]` is the number of units of resource j. A request gets at most one unit.

Stage 1 solves the original problem, with requests in place of customers, resources in place of menu items, and rejections in place of vouchers. Stages 2 and 3 add an explanation of shortages and an allocator that reassigns incrementally.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — The best offline assignment

```cpp
struct Plan {
    std::vector<int> choice;   // choice[i] = the assigned resource, or -1
    int rejected = 0;          // the number of requests that got nothing; must be minimal
};
Plan best_plan(std::span<const std::vector<int>> accepts, std::span<const int> quota);
```

Each request may only get a resource it accepts, and no resource hands out more than `quota[j]` units. O(E·f) is enough.

## Stage 2 — Explaining a shortage

```cpp
struct Shortage {
    std::vector<int> requests;
    std::vector<int> resources;
};
Shortage shortage_report(std::span<const std::vector<int>> accepts,
                         std::span<const int> quota);
```

When someone has to be rejected, operations wants to know why.

`Shortage shortage_report(accepts, quota)`, with `Shortage{requests, resources}`:
- Return a subset of the requests, `requests` (in increasing order), and a subset of the resources, `resources` (in increasing order), such that every resource accepted by a request in `requests` is in `resources`, and `Σ quota(resources) < requests.size()`.
- When `best_plan` gives `rejected == 0`, return two empty vectors.
- The tests pin down which subsets: in an optimal plan, `requests` is every request reachable from a rejected request by alternating steps (a request → a resource it accepts → a request holding that resource → …), and `resources` is exactly the resources reached that way. For `accepts {{0},{0},{1}}` and `quota {1,1}`, that is requests `{0, 1}` and resources `{0}`.

## Stage 3 — Real-world extension: an incremental allocator

```cpp
class Allocator {
public:
    explicit Allocator(std::vector<int> quota);
    // Adds a request and returns its request id (increasing from 0). Internally it reassigns other requests
    // so that the current set of requests still has the fewest rejections. O(E) per call.
    int add_request(std::vector<int> accepts);
    void cancel(int id);                 // cancels a request (a canceled id is no longer valid)
    int choice_of(int id) const;         // the currently assigned resource, or -1
    int rejected() const;                // how many of the requests not canceled are rejected
};
```

A resource freed by `cancel` must immediately give rejected requests a chance to take it; after every operation, the assignment must still be optimal. `add_request` never turns a served request into a rejected one (it may move it to another resource): if the newcomer can't be served that way, the newcomer is the one rejected.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is this problem the same as bipartite matching, and why don't you actually need to copy each resource into quota separate nodes?
2. In Stage 3, why does adding a request need only one augmenting path? And why does canceling a request need only one?
3. If requests had priorities (you would rather reject low-priority ones), would the algorithm still work? (min-cost flow / matroid)

---

**Official solution:** [Solutions to Problem Set 7](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/a320dad5988c30172a2b1538536dc0a2_MIT6_046JS15_pset7sols.pdf). Read it after you finish.
