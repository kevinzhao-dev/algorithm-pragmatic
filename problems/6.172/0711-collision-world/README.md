---
title: 2D Collision World (quadtree + fork-join)
unit: 07 · Collision Detection (Project 2)
source: 6.172 Project 2
pdf: materials/6.172/projects/p2/handout.pdf
read: materials/6.172/lectures/lec06.pdf, materials/6.172/lectures/lec07.pdf, materials/6.172/lectures/lec08.pdf, materials/6.172/homework/hw04/handout.pdf
kind: project
minutes: 540
sanitizer: tsan
---

# 0711 · 2D Collision World

**Original problem:** [Project 2: Collision Detection](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/ac1794b01bec7e14b4fd34d5b1a7d2aa_MIT6_172F18_project2.pdf) · **Read first:** [Multicore Programming](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/cbef71a2b92bc700fbad43e69f951513_MIT6_172F18_lec6.pdf), [Races and Parallelism](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/543e419bff1ed05b1a649115c24c44b3_MIT6_172F18_lec7.pdf), [Analysis of Multithreaded Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/5655e50541f85421cc47b7c85d8e5de3_MIT6_172F18_lec8.pdf), [HW4: Cilk, Reducer Hyperobjects](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/b2ff5659ae776c6fa6834967f73a2ab4_MIT6_172F18hw4.pdf)

> **Why this comes up in practice:** a physics engine's broad phase, path-conflict checks for AGVs and robot arms, and a map service's spatial join all do the same thing: out of a large number of moving objects, pick only the pairs that "might collide" for the expensive exact test. Making that exact test over a span of time is called continuous collision detection.

## Original problem

6.172 Project 2 (Collision Detection) gives you a screensaver written in C: line segments move around a 2D box and bounce off each other and off the walls under a simple physics model. The code you start from checks every pair of segments at every time step, Θ(N²) tests for N segments. Your job is to make it fast, first serially and then on a multicore machine with Open Cilk. The program reports its running time and its counts of line-wall and line-line collisions; you time it with the graphics turned off.

Ground rules (§3.2):

- The collisions found in a step are resolved in a fixed order, sorted by line IDs, because resolving the same set in another order changes the outcome in discrete time. A correct version reports exactly the same collision counts as the original code.
- Reference testing helps: keep the old and the new version of a function side by side and assert that they agree.
- You may use any data structure, but start with a quadtree, and make the serial code as fast as you can before you parallelize it.

Quadtrees (§3.3): each node covers a square, and a node that holds more than r elements (r is tunable) splits into four quadrants. Objects in different quadrants cannot collide. Because a segment moves during a step, the tree should hold the parallelogram the segment sweeps through during the next step, not just the segment itself. Some of these regions fit in no quadrant; the handout hints that not every segment has to be stored in a leaf. Use the provided `intersect` function for the exact test, and feel free to rebuild the tree from scratch at every step. Write-up questions:

- What speedup did you get, and was it what you expected?
- How does r affect performance, and why?
- What were your key design decisions, for example how you pull the collisions out of the finished tree?

Further optimization (§3.4): ideas include capping the tree depth and varying the cap, precomputing values that never change between steps (such as each segment's length), updating the tree instead of rebuilding it, and a faster intersection test. Keep a record of what you tried and how well it worked.

Parallelization (§4):

- Profile the serial program to find a region that dominates the running time and offers coarse-grained parallelism.
- Before adding any spawns, make the shared list of intersecting pairs safe for concurrent inserts with a reducer, as in Homework 4.
- Parallelize with `cilk_spawn` and `cilk_sync`. If little of the code suits them, restructure detection as a recursive depth-first walk of the quadtree. The collision counts must not change, and Cilksan must report no races.
- Measure work and span with Cilkscale. Vary the maximum depth, the node capacity, and any spawn cutoffs: how much parallelism can you reach?
- Tune for 8 workers, and describe your decisions, trade-offs, and further optimizations.

The graders run your code on inputs you haven't seen and on machines with more than 8 cores, so it must stay general and processor-oblivious. Correct code reproduces the original collision counts, looks physically plausible in graphics mode, and has no determinacy races. Teams also contribute new inputs that are both demanding and fun to watch. The final write-up explains your final strategy, your experiments, the reasons for your choices, and what you abandoned and why.

## Your task

The world holds a set of rigid thin bars (`Segment`) that advance in fixed ticks. Bars that touch exchange velocities, and a bar that reaches a wall bounces off it. You take detection all the way from O(N²) brute force to a quadtree plus fork-join, and at every stage the result must match the brute-force solution **pair for pair**.

`support/` is ready and needs no changes: `world_types.hpp` (`Vec`, `Segment`, `Box`, `Hit`), `scene.hpp` (seeded scene generation), and `scale.hpp` (`scale::fork2` and deterministic work/span measurement). Your code is split across `collide.hpp`, `world.hpp`, and `quadtree.hpp`. Use integer coordinates throughout: **floating point is not allowed**. It is guaranteed that `|coordinate| ≤ 2²⁶` and `|v| ≤ 2²⁰`; work out for yourself why `int64_t` is enough.

Unlike the original, which hands you a working C program to speed up, here you write the exact narrow-phase test, the world step, and all three detectors yourself, in C++ and with integer arithmetic only, and fork-join through `scale::fork2` takes the place of Open Cilk.

Each stage lists the declarations it adds, copied from those files. Stage 1 also shows the provided types from `support/world_types.hpp`.

## Stage 1 — Narrow phase, brute-force detection, and one world step

```cpp
// support/world_types.hpp (provided)
struct Vec {
    std::int64_t x = 0;
    std::int64_t y = 0;
    // +, -, == and << are provided
};

// A rigid bar with endpoints p and q (p == q is allowed) moving by v per tick.
struct Segment {
    std::uint32_t id = 0;
    Vec p;
    Vec q;
    Vec v;
    // == and << are provided
};

// The closed world rectangle [0, width] x [0, height].
struct Box {
    std::int64_t width = 0;
    std::int64_t height = 0;
};

// An unordered pair of segment ids, normalized so that a < b.
struct Hit {
    std::uint32_t a = 0;
    std::uint32_t b = 0;

    friend constexpr auto operator<=>(const Hit&, const Hit&) = default;
};

// collide.hpp
inline bool collides(const Segment& a, const Segment& b);

// The default predicate for every detector. Tests swap in their own to count calls.
struct ExactPredicate {
    bool operator()(const Segment& a, const Segment& b) const { return collides(a, b); }
};

// quadtree.hpp
template <typename Pred = ExactPredicate>
std::vector<Hit> detect_brute(std::span<const Segment> segments, Pred pred = {});

// world.hpp
class World {
public:
    World(Box box, std::vector<Segment> segments);  // provided

    // detect: any callable taking std::span<const Segment> and returning std::vector<Hit>
    template <typename Detect>
    void step(Detect&& detect);

    std::span<const Segment> segments() const;  // provided
    std::uint64_t segment_hits() const;         // provided
    std::uint64_t wall_hits() const;            // provided
};
```

**`collides(a, b)`:** at time parameter `τ ∈ [0, 1]` within one tick, bar `s` occupies `s.p + λ(s.q − s.p) + τ·s.v` (`λ ∈ [0, 1]`). Two bars collide if there is some common `τ` at which their positions overlap. **The sets are closed:** endpoints that just touch, collinear overlap, and bars that overlap at the start and then move apart all count. Zero-length bars with `p == q` must also be handled correctly.

**`detect_brute(segments, pred)`** returns every colliding `Hit`, with the ids normalized (`a < b`) and sorted by `(a, b)` in increasing order. A `Hit` holds `Segment::id` values, not array indices. Call `pred` exactly once for each unordered pair. This is the baseline for every later stage.

**`World::step(detect)`** runs one tick (`id == index`). The order is part of the spec:

1. `hits = detect(segments)`. For each hit, **in sorted order**, swap the two bars' `v`, and add the number of hits to `segment_hits`.
2. Process the bars in id order, treating the x and y axes independently: if either endpoint would fall outside `[0, width]` (or `[0, height]`) after the move, negate `v` on that axis, **do not move along that axis this tick**, and add one to `wall_hits`; otherwise move.

## Stage 2 — Quadtree

```cpp
// quadtree.hpp
// Stage 2 tuning knobs; change the defaults to whatever measures best.
struct QuadtreeOptions {
    std::size_t leaf_capacity = 12;
    int max_depth = 16;
};

template <typename Pred = ExactPredicate>
std::vector<Hit> detect_quadtree(std::span<const Segment> segments, Pred pred = {},
                                 const QuadtreeOptions& options = {});
```

`detect_quadtree(segments, pred, options)`: the output must be identical to `detect_brute`'s, but call `pred` only on pairs that might collide, and **at most once per pair**.

Things you have to think through yourself:

- Objects move, so the index must cover the whole area swept during the tick; a fast thin bar may cross many cells.
- A long bar lying across a split line fits in no child cell.
- Two bars may touch exactly on a split line. How must cell boundaries be defined so that the two are not separated?
- Bars that coincide exactly can never be separated, however much you split: `max_depth` must really take effect, and `leaf_capacity` is only a starting point.

Every valid `QuadtreeOptions` setting must give the correct answer (including `max_depth == 0`).

The tests also count the pairs that reach `pred`: on a sparse uniform scene of 4000 short bars it must see fewer than 1% of all pairs, and at constant density the number of pair tests must grow nearly linearly with the number of bars (a growth exponent below 1.35 from 2000 to 32000 bars).

## Stage 3 — Fork-join with zero data races

```cpp
// quadtree.hpp
// Spawns only through scale::fork2. May call pred from several threads at once.
template <typename Pred = ExactPredicate>
std::vector<Hit> detect_parallel(std::span<const Segment> segments, Pred pred = {},
                                 const QuadtreeOptions& options = {}, std::size_t grain = 256);
```

`detect_parallel(segments, pred, options, grain)`: split the work with `scale::fork2` (normally the same as `practice::fork2`), and take the serial path once a subproblem is no larger than `grain`. Requirements:

- For any number of workers, the output must be identical to the serial version's and deterministic (sorted). `pred` is called from several threads at once, and the total number of calls must equal the serial version's.
- ThreadSanitizer verifies this. All of this exercise's functional tests run under TSan, and a race stops the test that is running when it is found.
- Collecting the results is where things most often go wrong. Think about what you have to work with: per-worker buffers indexed by `practice::worker_index()`, or returning results up the recursion and merging them.
- A deterministic parallelism check: the tests measure work/span with `scale::measure` (work is the number of `pred` calls), and on spread-out scenes `work / span` must exceed a threshold (16 on a uniform scene of 20000 bars, and 8 on a clustered one). A `grain` that is too large fails right here.
- If `pred` throws, the exception must reach the caller of `detect_parallel`.

## Stage 4 — Measurement

There is no new interface. What you must pass are speedup ratios against the baseline, measured within the same executable (Release builds only):

| Scene | Requirement |
| --- | --- |
| Many bars, uniformly distributed | quadtree at least 20x faster than brute force |
| Clustered | quadtree at least 5x faster than brute force |
| Every bar crosses the center (the tree can't help) | quadtree at most 3x slower than brute force |
| 8 workers vs 1 worker | at least 1.5x (skipped with fewer than 8 hardware threads) |
| A complete `World` simulation | at least 8x faster than the brute-force world |

If a test fails, measure before you change anything: what value should `leaf_capacity` take? Do you allocate memory once per node? What happens when `grain` is too small? You can use `sample` or Instruments to find the hot spots. The ratio tests take the best of N runs, but they can still be off when the machine is busy, so rerun once before drawing conclusions.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why must the fork-join version's output be sorted? Without sorting, why would the same set of hits still make the world evolve differently?
2. Under what conditions does rebuilding the quadtree every tick, instead of updating it incrementally, pay off? According to your measurements, what share of the time does the rebuild take?
3. With many long bars, the root node's list blows up. Can you still keep the parallel span low? What do you trade for it?

To catch memory errors as well (TSan does not catch out-of-bounds accesses): `cmake -S . -B build/asan -G Ninja -DPRACTICE_MODE=asan && cmake --build build/asan --target t_6_172_0711_collision_world && build/asan/bin/6.172/0711-collision-world/run --skip-perf`.
