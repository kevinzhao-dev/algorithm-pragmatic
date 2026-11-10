---
title: Asynchronous Networks (max, BFS with corrections, node counting, Bellman-Ford)
unit: 10 · Distributed Algorithms (PS10)
source: 6.046J Lecture 20, Recitation 10
pdf: materials/6.046j/lectures/lec20.pdf
read: materials/6.046j/lectures/lec20.pdf, materials/6.046j/recitations/rec10.pdf
kind: code
minutes: 130
---

# 1002 · Asynchronous Networks

**Based on:** [Asynchronous Distributed Algorithms: Shortest-paths Spanning Trees](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/78a79d1a832dbae7274695d7bd33d044_MIT6_046JS15_lec20.pdf) · **Read first:** [Asynchronous Distributed Algorithms: Shortest-paths Spanning Trees](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/78a79d1a832dbae7274695d7bd33d044_MIT6_046JS15_lec20.pdf), [More Distributed Algorithms](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/2218648e1afd6e487e114d451aa54e2d_MIT6_046JS15_Recitation10.pdf)

> **Why this comes up in practice:** real networks have no lockstep: messages arrive out of order and packets get stuck in congestion. This exercise shows how algorithms that worked fine synchronously are forced to make "corrections" under asynchrony, and how in the worst case the message count blows up exponentially.

## Your task

Everything runs on `netsim::AsyncNetwork`: a process implements `on_start(ctx)` and `on_message(port, msg, ctx)`, and sends messages with `ctx.send(port, m)` / `ctx.broadcast(m)`. There are no rounds; when a message arrives depends on the delays the tests choose (channels stay FIFO). The network stops when every channel is empty.

You implement four asynchronous algorithms from Lecture 20 and Recitation 10 and see what asynchrony costs each of them.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Max flooding

```cpp
class MaxFlood {
public:
    using Message = long long;
    explicit MaxFlood(long long value);

    void on_start(netsim::Context<Message>& ctx);
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx);
    long long value() const;
};
```

`MaxFlood(long long value)`: every process has an initial value. At the start, broadcast your value; whenever you receive a larger one, update and broadcast it. `value()` returns the current maximum. After convergence, every process's `value()` is the global maximum, and the total number of messages is ≤ `2·E·n`.

## Stage 2 — Asynchronous BFS (with corrections)

```cpp
inline constexpr long long kInf = std::numeric_limits<long long>::max() / 4;

class AsyncBfs {
public:
    using Message = long long;
    explicit AsyncBfs(bool is_root);

    void on_start(netsim::Context<Message>& ctx);
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx);
    long long distance() const;
    int parent_port() const;
};
```

`AsyncBfs(bool is_root)`: maintain `distance()` (the hop distance, `kInf` while unknown) and `parent_port()` (-1 for the root). The root starts by broadcasting 0. When a process receives `d` and `d+1` is smaller than its current distance, it updates its parent and broadcasts the new distance (this is asynchronous relaxation). Relax only on a strict improvement: the tests require at most 2E messages under unit delays (on a 24-node ring), and strictly more when skewed delays force corrections.

## Stage 3 — Counting nodes (convergecast)

```cpp
class CountingTree {
public:
    struct Message {
        int kind = 0;
        long long value = 0;
    };
    explicit CountingTree(bool is_root);

    void on_start(netsim::Context<Message>& ctx);
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx);
    long long root_count() const;
};
```

`CountingTree(bool is_root)`: count the total number of nodes over any spanning tree (it doesn't have to be BFS). On the root, `root_count()` returns `n` (and -1 until it is done). The message count must be ≤ `4E + n`, which takes a convergecast with parent/nonparent replies and done counts. A process that receives a search replies whether it accepts. Once it has collected every reply (so it knows which neighbors are its children) and a done(k) from every child, it reports its subtree size to its parent. A root with no neighbors must still end with `root_count()` equal to 1; since no message will ever arrive, set it in `on_start`.

## Stage 4 — Asynchronous Bellman-Ford and its cost

```cpp
class AsyncBellmanFord {
public:
    using Message = long long;
    AsyncBellmanFord(bool is_root, std::vector<long long> port_weights);

    void on_start(netsim::Context<Message>& ctx);
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx);
    long long distance() const;
    int parent_port() const;
};
```

`AsyncBellmanFord(bool is_root, std::vector<long long> port_weights)`, with `distance()` and `parent_port()`. Every time dist improves, broadcast the new value on **all** ports (as in the lecture notes). As in Stage 2, the root starts by broadcasting 0 in `on_start`. With nonnegative weights (the tests include weight-0 edges) it eventually converges to the correct answer, however the messages are scheduled.

The tests also count messages on the lecture's exponential gadget, a chain of k segments that each have two parallel edges, of weight 2^(k−1−i) and 0. Under an adversarial schedule, the k = 14 run must send at least 2^14 messages and at least 200 times as many as the k = 4 run; when the good links are delivered first, it must send at most 4E. So relay every strict improvement at once on every port, and send nothing else.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does asynchronous BFS eventually converge, even though `parent` can be very bad along the way? Where does the `n` in the time bound `O(diam·n·d)` come from?
2. What does the exponential lower-bound graph for Stage 4 look like? Think about how each segment uses a "binary countdown" to pile 2^k messages into a channel.
3. What practical ways are there to eliminate corrections? (The layered wave of PS10-2, or a synchronizer.) What do they cost?
