---
title: Layered BFS on an Asynchronous Network (no corrections)
unit: 10 · Distributed Algorithms (PS10)
source: 6.046J PS10 Problem 10-2
pdf: materials/6.046j/psets/ps10/questions.pdf
answer: materials/6.046j/psets/ps10/solutions.pdf
read: materials/6.046j/lectures/lec20.pdf
kind: code
minutes: 150
---

# 1012 · Layered BFS

**Original problem:** [Problem Set 10](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/d4865809813b7de690cbdebb0bf8b4f4_MIT6_046JS15_pset10.pdf) · **Read first:** [Asynchronous Distributed Algorithms: Shortest-paths Spanning Trees](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/78a79d1a832dbae7274695d7bd33d044_MIT6_046JS15_lec20.pdf)

> **Why this comes up in practice:** in the BFS trees of routing protocols and in tree building for data center fabrics, once a node has "output" its parent, that is hard to take back. The relaxation version in 1002 keeps making corrections; this exercise replaces corrections with layered waves, which also gives cleaner bounds on messages and time.

## Original problem

6.046J PS10 Problem 10-2 improves on the asynchronous BFS algorithm from lecture. That algorithm makes corrections, and each correction can trigger many messages: until every node's parent variable has settled on a correct parent in a breadth-first spanning tree, it may send O(nE) messages and take O(diam·n·d) time in the worst case. (These bounds ignore the processes' parent outputs, global termination, and local processing time.) The goal is an asynchronous BFS that never makes a correction, so a process can output its parent as soon as it sets it, because that choice is final. The network graph is connected and has at least two nodes.

- (a) Describe, in words and in detail, an algorithm where the root v0 directs the construction of the tree one level at a time; all parent variables must be set within O(diam²·d) time. The hint: build each new level with a broadcast-convergecast wave from the root. Four kinds of message should do, such as search (probing for new nodes), parent(b) with a Boolean b (the parent/nonparent reply), and ready and done (the broadcast and convergecast signals along the tree).
- (b) Analyze the algorithm's running time and message count, and compare them with the cost of the asynchronous BFS from lecture.

## Your task

Use `netsim::AsyncNetwork`. Message delays are arbitrary (but FIFO), with an upper bound of `d`. The graph is connected and undirected with at least two nodes, and the root is `v0` (a process learns whether it is the root through its constructor); n and diam are unknown.

```cpp
class LayeredBfs {
public:
    struct Message { ... };   // define it yourself; four message types (search, parent, ready, done) are enough
    explicit LayeredBfs(bool is_root);
    void on_start(netsim::Context<Message>& ctx);
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx);

    bool root_done() const;     // root only: the whole BFS has finished
    int parent_port() const;    // parent port of a non-root node; -1 for the root
    int level() const;          // BFS depth; 0 for the root
};
```

You implement the algorithm of part (a) as one process class, and the tests check bounds of the kind that part (b) asks you to derive.

Write your code in `solution.hpp`. Since this exercise has no stages, the block above shows the whole interface instead of each stage listing the declarations it adds.

## Specification

In each phase, the root broadcasts `ready` along the current BFS tree to carry the signal out to the current frontier. Frontier nodes send `search` to all their neighbors, and after collecting every reply (`parent(true)`/`parent(false)`), they convergecast `done(bit)` back to the root, where `bit` says whether this round produced any new nodes. Once the root has collected every `done`, it starts the next phase if there are new nodes, and finishes otherwise.

- As soon as a node has decided its parent, it outputs it with `ctx.output(parent_port)`, **exactly once in its lifetime**, and must never change it afterwards (the root doesn't output). This is the "no corrections" that the problem asks for.
- `level()` must equal the hop distance to the root.

## Complexity requirements

Let `E` be the number of edges and `diam` the diameter of the graph. The tests confirm that:

- The total number of messages is ≤ `4E + 2·n·(diam + 2)`.
- With delays bounded by `d`, the completion time is ≤ `3·d·(diam + 3)²`.
- Under every delay policy (unit, random, and adversarial skew within FIFO), `root_done()` eventually holds, every node outputs its parent exactly once, and `level` is correct.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does each phase of layered BFS take O(diam·d), for O(diam²·d) in total? And why is the message count O(E + n·diam)?
2. Compared with the relaxation BFS in 1002, what do you gain and what do you lose in time and in messages? When is the layered version slower?
3. Why does each tree edge carry at most two ready/done messages per phase? If one phase covered several levels at once, how much faster could it be? (The idea behind a synchronizer.)

---

**Official solution:** [Solutions to Problem Set 10](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/7a3fe48c42ff8d2f7052e9fcf5054ae6_MIT6_046JS15_pset10sols.pdf). Read it after you finish.
