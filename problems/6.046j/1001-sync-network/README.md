---
title: Synchronous Network Algorithms (leader, BFS tree, MIS, Bellman-Ford)
unit: 10 · Distributed Algorithms (PS10)
source: 6.046J Lecture 19, Recitation 10
pdf: materials/6.046j/lectures/lec19.pdf
read: materials/6.046j/lectures/lec19.pdf, materials/6.046j/recitations/rec10.pdf
kind: code
minutes: 140
---

# 1001 · Synchronous Network Algorithms

**Based on:** [Synchronous Distributed Algorithms: Symmetry-breaking. Shortest-paths Spanning Trees](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/662439afce08e29eda88612482506f78_MIT6_046JS15_lec19.pdf) · **Read first:** [Synchronous Distributed Algorithms: Symmetry-breaking. Shortest-paths Spanning Trees](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/662439afce08e29eda88612482506f78_MIT6_046JS15_lec19.pdf), [More Distributed Algorithms](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/2218648e1afd6e487e114d451aa54e2d_MIT6_046JS15_Recitation10.pdf)

> **Why this comes up in practice:** the leader of a distributed database, the spanning tree of a data center network, and job scheduling that hands tokens to an independent set are all built on these basic protocols. The synchronous model takes away the chaos of timing, so you can first focus on "what gets sent in each round, and who can tell that it's over".

## Your task

Everything runs on `netsim::SyncNetwork` from `_shared/06_net_sim.hpp`; **do not write your own message passing**. Each protocol is a class: declare `using Message = ...` and implement `send(round, outbox)`, `receive(round, inbox)`, and `halted()` (see the header for the interface). A process knows only how many ports it has, not its vertex number.

The tests check the results **and the number of messages** (`stats.messages`).

You implement four synchronous protocols from Lecture 19 and Recitation 10, each as a process class that the simulator runs.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Leader election in a clique

```cpp
class CliqueLeader {
public:
    using Message = std::int64_t;
    explicit CliqueLeader(std::int64_t uid);

    void send(std::int64_t round, netsim::Outbox<Message>& out);
    void receive(std::int64_t round, std::span<const std::optional<Message>> inbox);
    bool halted() const;
    bool is_leader() const;
};
```

`CliqueLeader(std::int64_t uid)`, with distinct UIDs. In the first round, broadcast your UID; once all the UIDs have arrived, the largest UID becomes the leader (`is_leader()`). After that one round, `halted()` is true.

## Stage 2 — BFS spanning tree and convergecast

```cpp
class BfsTree {
public:
    struct Message {
        int kind = 0;   // define your own message types
        int depth = 0;
    };
    explicit BfsTree(bool is_root);

    void send(std::int64_t round, netsim::Outbox<Message>& out);
    void receive(std::int64_t round, std::span<const std::optional<Message>> inbox);
    bool halted() const;

    int parent_port() const;
    std::vector<int> children_ports() const;
    int depth() const;
    bool finished() const;
};
```

`BfsTree(bool is_root)`: the graph is connected, and n is unknown. `search` messages spread outward from the root. On receiving its first search, a process chooses its parent and replies `parent`; it replies `nonparent` to every other search. **Do not use UIDs**.

- `parent_port()`: the parent's port (-1 for the root, or before the process has joined).
- `children_ports()`: return the children's ports, in increasing order.
- `depth()`: the hop distance from the root (0 for the root).
- Convergecast: starting from the leaves, send `done` upward. Once the root has collected all of them, `finished()` returns true, and every node is `halted()` after it has sent (or collected all) `done` messages.

The total number of messages must be ≤ `4E + n`. O(diam) rounds: the tests require the whole run, until every process has halted, to take at most `3·ecc + 8` rounds, where ecc is the largest hop distance from the root.

## Stage 3 — Luby's maximal independent set

```cpp
class Luby {
public:
    struct Message {
        bool joined = false;
        std::uint64_t value = 0;
    };
    Luby(std::uint64_t seed, std::uint64_t n_upper);

    void send(std::int64_t round, netsim::Outbox<Message>& out);
    void receive(std::int64_t round, std::span<const std::optional<Message>> inbox);
    bool halted() const;

    bool decided() const;
    bool in_mis() const;
};
```

`Luby(std::uint64_t seed, std::uint64_t n_upper)`. Each phase has two rounds. In the first, every active node broadcasts a random value in `1..n⁵` (using `n_upper`), and a node whose value is larger than all of its neighbors' joins the MIS. In the second, the nodes that joined broadcast a notice, and every node that receives one drops out. A node halts once it has decided. `decided()`, `in_mis()`.

The simulator stops calling a process once it has halted, so a node that joins must stay active through the second round to broadcast its notice: joiners and the nodes they notify become decided (and halt) at the end of that round. The tests require every node to be decided at the end, and the run to finish within `12·⌈lg max(n, 2)⌉ + 12` phases (`stats.rounds / 2`).

## Stage 4 — Synchronous Bellman-Ford

```cpp
inline constexpr long long kInf = std::numeric_limits<long long>::max() / 4;

class SyncBellmanFord {
public:
    using Message = long long;
    SyncBellmanFord(bool is_root, std::vector<long long> port_weights, int rounds);

    void send(std::int64_t round, netsim::Outbox<Message>& out);
    void receive(std::int64_t round, std::span<const std::optional<Message>> inbox);
    bool halted() const;

    long long distance() const;
    int parent_port() const;
};
```

`SyncBellmanFord(bool is_root, std::vector<long long> port_weights, int rounds)`: `port_weights[p]` is the (nonnegative) weight of the edge on port p. Every round, broadcast your current distance on all ports, then relax with what you receive. Halt once all `rounds` rounds have run. `distance()` returns the distance (`kInf` if unreachable, defined in the header), and `parent_port()` the parent's port (-1 for the root).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is the BFS tree built this way always a shortest-path tree? Does that still hold if search messages have different delays (no synchrony)?
2. How many phases does Luby's algorithm take in expectation? What is the intuition for each phase removing half of the edges?
3. Why must convergecast wait until "every neighbor has responded" before sending `done` upward? What happens without that condition?
