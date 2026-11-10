---
title: Skip List
unit: 05 · Skip Lists, Hashing, and DP (PS5)
source: 6.046J Lecture 7
pdf: materials/6.046j/lectures/lec07.pdf
read: materials/6.046j/lectures/lec07.pdf
kind: code
minutes: 150
---

# 0501 · Skip List

**Based on:** [Randomization: Skip Lists](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/14fe095781943ff9bd485e6e263b48f7_MIT6_046JS15_lec07.pdf) · **Read first:** [Randomization: Skip Lists](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/14fe095781943ff9bd485e6e263b48f7_MIT6_046JS15_lec07.pdf)

> **Why this comes up in practice:** Redis's sorted sets and LevelDB's memtable are both skip lists. A skip list is simpler to write than a balanced tree and makes fine-grained concurrency easier; the only price is that its balance is guaranteed only probabilistically.

## Your task

Implement a skip list that stores `Key`s with set semantics. `Key` is only guaranteed to support moving and `operator<` (`insert` receives its key by value, so you can move it directly).

`Node` is a public structure: `next[i]` is this node's next node on level `i`, and the tower's height is `next.size()`. `height()` is the number of levels currently in use (`0` for an empty list), `first_at(i)` is the first node on level `i`, and `first()` is `first_at(0)`. The tests walk the `Node`s to check that every level is sorted and that every node on level `i+1` is also on level `i`.

The constructor uses `seed` to initialize the internal random number generator, and the same `seed` must produce the same structure (don't use the time or `random_device`). Flip a coin with probability `1/2` to decide each tower's height.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Search and insertion

```cpp
template <typename Key>
class SkipList {
public:
    struct Node {
        Node(Key k, std::size_t height) : key(std::move(k)), next(height, nullptr) {}  // provided

        Key key;
        std::vector<Node*> next;
    };

    explicit SkipList(std::uint64_t seed = 6046);

    ~SkipList();  // free every node, iteratively

    SkipList(const SkipList&) = delete;
    SkipList& operator=(const SkipList&) = delete;

    SkipList(SkipList&& other);             // should be noexcept once implemented
    SkipList& operator=(SkipList&& other);  // should be noexcept once implemented

    std::size_t size() const;
    bool empty() const;
    int height() const;
    Node* first_at(int level) const;
    Node* first() const;

    const Node* find(const Key& key) const;
    bool contains(const Key& key) const;
    bool insert(Key key);
};
```

| Operation | Behavior |
| --- | --- |
| `insert(key)` | Return whether the key was added; do nothing for a duplicate |
| `find(key)` | Return a pointer to the node, or `nullptr` if the key is absent |
| `contains(key)`, `size()`, `empty()` | |
| Destructor | Free every node without recursion; 300,000 nodes must not overflow the stack |
| Move construction and assignment | The source becomes a valid empty list |

A search must cost **expected O(log n)** comparisons, and the tests measure this with `Counted`.

## Stage 2 — Deletion

```cpp
// in class SkipList<Key>
bool erase(const Key& key);
```

`erase(key)`: if the key exists, delete it and return `true`. Remove the node from every level and drop the top levels that become empty, so that `height()` goes down with them; once everything is deleted, it is back to `0`.

## Stage 3 — Real-world extension: a delayed-job scheduler

```cpp
class JobScheduler {
public:
    explicit JobScheduler(std::uint64_t seed = 6046);

    int schedule(std::int64_t run_at);
    bool cancel(int job);
    std::vector<int> pop_due(std::int64_t now);
    std::size_t pending() const;
};
```

`JobScheduler(seed)`: keep the jobs that haven't come due yet in a skip list.

- `schedule(run_at)`: return the job's number (`0, 1, 2, ...`).
- `cancel(job)`: cancel a job that hasn't run yet, and return whether it was actually cancelled (`false` if it doesn't exist or has already run).
- `pop_due(now)`: remove and return every job with `run_at ≤ now`, in increasing order of `run_at`, and in scheduling order among jobs with the same time.
- `pending()`: the number of jobs that have neither come due nor been cancelled.

Each operation has an expected cost of O(log n).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does "flip coins to decide the heights" make the expected search path O(log n)? Walk through the argument that traces the path backward (backward analysis).
2. Why are concurrent updates easier for this structure than for a balanced BST? When does it still lose?
3. What changes if the probability goes from `1/2` to `1/4`? Why does Redis choose `1/4`?
