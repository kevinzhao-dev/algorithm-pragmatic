---
title: Dynamic Max Flow (capacity updates without recomputing)
unit: 07 · Network Flow (PS7)
source: 6.046J PS7 Problem 7-1
pdf: materials/6.046j/psets/ps07/questions.pdf
answer: materials/6.046j/psets/ps07/solutions.pdf
read: materials/6.046j/lectures/lec13.pdf, materials/6.046j/lectures/lec14a.pdf
kind: code
minutes: 150
---

# 0711 · Dynamic Max Flow

**Original problem:** [Problem Set 7](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/93951baaee86c66b1f2fd48e2c493745_MIT6_046JS15_pset7.pdf) · **Read first:** [Incremental Improvement: Max Flow, Min Cut](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/ac6b6e8ac932bb5dfb6506fd0db9d5ad_MIT6_046JS15_lec13.pdf), [Incremental Improvement: Matching](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/bdafe05f4f3c20cabfd08d2fbd099f63_MIT6_046JS15_lec14A.pdf)

> **Why this comes up in practice:** an SDN controller allocates bandwidth while link capacities keep changing (maintenance, failures, QoS reallocation), and recomputing the max flow of the whole network every time costs too much. This exercise gets the work down to O(k(V+E)) when a capacity changes by only k.

## Original problem

6.046J PS7 Problem 7-1 asks for an algorithm that repairs a maximum flow after one capacity changes. Its input is a flow network F = (G, c), where G = (V, E) has a source s and a target t and c gives every directed edge a nonnegative integer capacity; a maximum flow f for F; and a triple (u, v, r), where u and v are vertices of G and r is a nonnegative integer with r ≠ c(u, v). It must output a maximum flow for F′ = (G, c′), where c′ agrees with c except that c′(u, v) = r. With k = |c(u, v) − r|, the algorithm must run in O(k·(V + E)) time, and it treats r > c(u, v) and r < c(u, v) differently.

- (a) First prove four general facts about flow networks. Raising one edge's capacity by 1 raises the max flow by at most 1, and raising it by a positive integer k raises the max flow by at most k. Likewise, lowering one edge's capacity by 1 lowers the max flow by at most 1, and lowering it by k lowers the max flow by at most k.
- (b) For r > c(u, v), give the algorithm, prove it correct, and analyze its running time in terms of V, E and k.
- (c) For r < c(u, v), do the same: give the algorithm, prove it correct, and analyze its running time.

## Your task

Vertices are `0..n-1`, the source and sink are given to the constructor, and capacities are nonnegative integers. Parallel and reverse edges are allowed; `u == v` never appears.

```cpp
class DynamicMaxFlow {
public:
    DynamicMaxFlow(int n, int source, int sink);
    int add_edge(int u, int v, long long capacity);   // edge ids increase from 0
    long long compute();                              // computes a max flow and returns its value
    long long value() const;                          // the current flow value
    long long flow_on(int id) const;
    long long capacity_of(int id) const;
    long long set_capacity(int id, long long r);      // see below; returns the new flow value
};
```

Every `add_edge` call comes before the first `compute`; after that, only `set_capacity` may change the network. `capacity_of(id)` returns the edge's current capacity (`r` after `set_capacity(id, r)`). `set_capacity` may also be called with `r` equal to the current capacity; then nothing changes, and it returns the current flow value.

Stage 1 builds the network and an initial max flow, and Stages 2–4 implement parts (b) and (c) inside `set_capacity`. Part (a) is not coded; it comes back as an interview follow-up.

Write your code in `solution.hpp`. Rather than each stage listing the declarations it adds, the block above shows the whole interface, copied from that file.

Read one stage at a time.

## Stage 1 — Building the network and the initial max flow

`compute()` builds the residual graph and computes a max flow, reusing your work from 0701 (or rewriting it). `flow_on` must satisfy the capacity constraints and conservation.

## Stage 2 — Increasing a capacity (Part b)

`set_capacity(id, r)` with `r > c`: return the new max flow. With `k = r - c`, it must take O(k(V+E)) time. You **must not** recompute from scratch.

## Stage 3 — Decreasing a capacity when the flow still fits (Part c, the easy case)

`r < c` and the current `flow_on(id) ≤ r`: the max flow can't increase, and no flow needs to change (why?).

## Stage 4 — Decreasing a capacity when the flow no longer fits (Part c, the main case)

`r < c` and `flow_on(id) > r`: first cancel the excess flow on this edge (restoring conservation), then try to make it up along other paths. With `k = c - r`, it must take O(k(V+E)) time. Afterwards `flow_on(id) ≤ r` and the flow is still a max flow.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does the max flow change by at most k when a capacity changes by k? Explain it with min cuts (Part a). How does that relate to the k augmentations behind O(k(V+E))?
2. When you cancel flow by walking backward along edges that carry positive flow, why are you guaranteed to reach s, t, or v? (The conservation invariant; what if the flow contains a cycle?)
3. If capacities weren't integers, would the O(k(V+E)) argument still hold? How would you fix it?

---

**Official solution:** [Solutions to Problem Set 7](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/a320dad5988c30172a2b1538536dc0a2_MIT6_046JS15_pset7sols.pdf). Read it after you finish.
