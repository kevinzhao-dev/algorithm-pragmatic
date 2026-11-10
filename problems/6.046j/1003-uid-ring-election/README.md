---
title: Leader Election on a UID Ring (LCR and Hirschberg-Sinclair)
unit: 10 · Distributed Algorithms (PS10)
source: 6.046J Recitation 10
pdf: materials/6.046j/recitations/rec10.pdf
read: materials/6.046j/recitations/rec10.pdf, materials/6.046j/lectures/lec19.pdf
kind: code
minutes: 100
---

# 1003 · Leader Election on a UID Ring

**Based on:** [More Distributed Algorithms](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/2218648e1afd6e487e114d451aa54e2d_MIT6_046JS15_Recitation10.pdf) · **Read first:** [More Distributed Algorithms](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/2218648e1afd6e487e114d451aa54e2d_MIT6_046JS15_Recitation10.pdf), [Synchronous Distributed Algorithms: Symmetry-breaking. Shortest-paths Spanning Trees](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/662439afce08e29eda88612482506f78_MIT6_046JS15_lec19.pdf)

> **Why this comes up in practice:** token-ring topologies, ring maintenance in Chord-style DHTs, and any setting where "nodes have unique IDs and must elect one leader". For the same problem, the message count can differ by as much as O(n²) vs O(n log n).

## Your task

Use `netsim::AsyncNetwork` (with unit delays it behaves exactly like synchronous rounds). The ring is built by `netsim::Topology::ring(n)`: port 0 (`kClockwise`) leads to the clockwise neighbor and port 1 (`kCounterclockwise`) to the counterclockwise neighbor, and all processes share this sense of direction. Each process knows its own UID from the start (the UIDs are distinct), but it doesn't know n.

The tests always compare `stats.messages`.

You implement the two ring election algorithms from Recitation 10, LCR and Hirschberg-Sinclair, and compare their message counts.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — LCR (Le Lann–Chang–Roberts)

```cpp
class LcrProcess {
public:
    using Message = long long;
    explicit LcrProcess(long long uid);

    void on_start(netsim::Context<Message>& ctx);
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx);
    bool is_leader() const;
};
```

`LcrProcess(long long uid)`, with `is_leader()`. Use only the clockwise channels: at the start, send your UID clockwise. Keep forwarding a received UID clockwise if it is larger than yours, and discard it if it is smaller; when you receive your own UID, you become the leader.

The worst-case message count is Θ(n²). The tests use rings in ascending and in descending order and compare exact message counts, so send nothing but the UID relay (no leader announcement or halt message). With unit delays, a ring whose UIDs decrease clockwise (vertex v has UID n − v) must take exactly n(n+1)/2 messages, and one whose UIDs increase clockwise (vertex v has UID v + 1) exactly 2n − 1.

## Stage 2 — Hirschberg-Sinclair

```cpp
class HsProcess {
public:
    struct Message {
        bool reply = false;
        long long uid = 0;
        int hops = 0;
    };
    explicit HsProcess(long long uid);

    void on_start(netsim::Context<Message>& ctx);
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx);
    bool is_leader() const;
};
```

`HsProcess(long long uid)`, with `is_leader()`. It works in phases: in phase ℓ, a process sends a probe `2^ℓ` hops in each direction. Along the way, a probe is forwarded only while its UID is larger than that of each node it passes, and at the distance limit it turns back along the same path. If the replies from both sides come back, the process moves on to the next phase; if its probe goes all the way around the ring back to it, it becomes the leader. O(n log n) messages in total (the tests' limit is `8·n·(⌈lg n⌉+1)`, under unit and random delays alike). On the descending 2048-node ring with unit delays, HS must also use at most 1/20 as many messages as LCR.

The same code must also be correct under random delays (exactly one leader, and it has the largest UID).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. What are LCR's worst-case and average message counts? Why is the average over random UID orders O(n log n)?
2. Why does Hirschberg-Sinclair at least halve the number of active nodes in each phase? Why do probes need to turn back? Isn't probing forward enough?
3. Why is (deterministic) election impossible on an anonymous ring (no UIDs)? How does that differ from the UID assumption in Stage 2? (Compare with 1011.)
