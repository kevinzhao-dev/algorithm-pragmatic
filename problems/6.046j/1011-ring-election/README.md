---
title: Leader Election on an Anonymous Ring (symmetry, randomness, and impossibility)
unit: 10 · Distributed Algorithms (PS10)
source: 6.046J PS10 Problem 10-1
pdf: materials/6.046j/psets/ps10/questions.pdf
answer: materials/6.046j/psets/ps10/solutions.pdf
read: materials/6.046j/lectures/lec19.pdf
kind: code
minutes: 130
---

# 1011 · Leader Election on an Anonymous Ring

**Original problem:** [Problem Set 10](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/d4865809813b7de690cbdebb0bf8b4f4_MIT6_046JS15_pset10.pdf) · **Read first:** [Synchronous Distributed Algorithms: Symmetry-breaking. Shortest-paths Spanning Trees](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/662439afce08e29eda88612482506f78_MIT6_046JS15_lec19.pdf)

> **Why this comes up in practice:** IoT groups without UIDs and consensus protocols that must break symmetry all run into one basic fact: perfectly symmetric deterministic nodes can never elect a leader, and even with randomness the problem stays unsolvable when n is unknown. This exercise turns all three arguments into code you can run.

## Original problem

6.046J PS10 Problem 10-1 places n identical processes on a synchronous ring. Each process has ports to its two neighbors: the ports toward the clockwise neighbor are called left and those toward the counterclockwise neighbor right, so all processes share one sense of direction. The goal is to elect a single leader, meaning that eventually exactly one process outputs LEADER. Each part asks you either to give an algorithm and analyze its time and message complexity, or to prove that no such algorithm exists.

- (a) The processes are deterministic and know n, the size of the ring.
- (b) The processes are randomized and know n. The algorithm must never elect more than one leader, and must elect one eventually with probability 1. Relate the complexity to the success probability: for any ε with 0 < ε < 1, give time and message bounds that hold with probability at least 1 − ε.
- (c) The processes are randomized but don't know n, so one algorithm has to work on rings of every size. The requirements and the analysis are the same as in (b).

## Your task

The ring is built by `Topology::ring(n)` (the synchronous model, `SyncNetwork`). All processes are **identical**; the only thing they have is a sense of port direction.

Each stage turns one part into code: Stage 1 checks the symmetry argument behind part (a), Stage 2 implements a randomized election for part (b), and Stage 3 builds a harness that acts out the argument for part (c).

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Why deterministic election is impossible (Part a)

```cpp
template <typename P>
bool symmetry_preserved(std::function<P()> make, int n, int rounds);
```

Create n processes with `make()` and place them on a ring, run `rounds` rounds one at a time (`net.step()`), and after each round compare the `state_digest()` of every process (`P` provides it, and it returns a `std::string`). Return `true` if they are all the same, and `false` as soon as any of them differ.

The tests also require these details: call `make()` once for each of the n processes; compare the digests once before the first round as well, so that `rounds = 0` still checks the initial states; and if every process halts before `rounds` rounds are up (`net.step()` returns `false`), stop and return `true`. The tests feed in deterministic processes to confirm that symmetry can't be broken, and also a process that secretly carries a different internal number, to confirm that your check really compares.

(This is the lecture's proof by induction: the same state and the same port inputs lead to the same state in the next round.)

## Stage 2 — Randomized, knowing n (Part b)

```cpp
class RandomizedElection {
public:
    using Message = std::int64_t;
    RandomizedElection(int n, std::uint64_t seed);

    void send(std::int64_t round, netsim::Outbox<Message>& out);
    void receive(std::int64_t round, std::span<const std::optional<Message>> inbox);
    bool halted() const;
    bool is_leader() const;
    int phases() const;
};
```

`RandomizedElection(int n, std::uint64_t seed)`: a synchronous process that satisfies all of the following:

1. **Safety:** at any time, at most one process outputs leader (`is_leader()`).
2. **Termination** (with probability 1): eventually every process is `halted()` and exactly one is `is_leader()`.
3. `phases()` returns how many phases were used, and every process must report the same value (the tests read process 0's). Each phase takes at most `n` rounds and at most `n²` messages (`stats.rounds ≤ n·phases`, `stats.messages ≤ n²·phases`).
4. Each phase succeeds with probability at least 1/2: the fraction of runs that need more than `h` phases must be statistically ≤ `2^-h` (the tests measure the tail over many seeds).

Hint: in each phase, every process draws a random id from `1..n²` and passes it clockwise for n rounds, then checks whether the maximum is unique.

## Stage 3 — Why it's impossible without knowing n (Part c)

```cpp
struct MirrorResult { int leaders_small = 0; int leaders_large = 0; };
template <typename P, typename Make>
MirrorResult mirror_attack(Make make, int n, std::uint64_t seed, std::int64_t max_rounds);
```

`make(s)` returns a process that uses random seed `s` (a `P` that doesn't know n and has `is_leader()`). Run it on a ring of size `n` (node i gets seed `seed+i`), then on a ring of size `2n`, where positions `i` and `i+n` get the **same** seed `seed+i` (a copied random tape). Return the number of leaders in each run. Run each ring for at most `max_rounds` rounds (for example with `net.run(max_rounds)`), because the candidate processes need not halt on their own.

This harness makes the argument concrete: whenever the ring of size `n` elects a leader, the mirrored run on the ring of size `2n` has two leaders, so no algorithm handles both sizes correctly.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does the proof of Part (a) need "determinism" and "the same port labeling"? What if the port labels were inconsistent?
2. Why does Stage 2 need an id space of only `1..n²`? Why use the birthday bound to compute the success probability? How do you convert between success probability and time?
3. In practice, how do distributed systems get around the impossibility in Stage 3? (An upper bound on n, UIDs, or giving up deterministic guarantees.)

---

**Official solution:** [Solutions to Problem Set 10](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/7a3fe48c42ff8d2f7052e9fcf5054ae6_MIT6_046JS15_pset10sols.pdf). Read it after you finish.
