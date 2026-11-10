---
title: Combine B-Trees (merging in O(height difference))
unit: 02 · FFT and B-Trees (PS2)
source: 6.046J PS2 Problem 2-2
pdf: materials/6.046j/psets/ps02/questions.pdf
answer: materials/6.046j/psets/ps02/solutions.pdf
read: materials/6.046j/recitations/rec02.pdf
kind: code
minutes: 150
---

# 0212 · Combine B-Trees

**Original problem:** [Problem Set 2](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/4a1b44e7e3f34aa85adb782d70048210_MIT6_046JS15_pset2.pdf) · **Read first:** [B-trees](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/8fd15588f584c518b6ec83aedb2b96c9_MIT6_046JS15_Recitation2.pdf)

> **Why this comes up in practice:** in an index partitioned by time, actually rewriting all the data to merge old partitions is too expensive. If you can touch only the edges of the trees and leave the interior nodes alone, the cost of a merge no longer depends on the amount of data.

## Original problem

6.046J PS2 Problem 2-2 ("Combining B-trees") introduces a new operation, COMBINE(T1, T2, k). It takes two B-trees, T1 and T2, that share a minimum degree t, and a fresh key k found in neither; all keys of T1 are below k, and all keys of T2 are above it. COMBINE returns one B-tree T, again of minimum degree t, containing every key of T1 and T2 plus k, and T1 and T2 do not survive the operation. The goal is a running time of O(|h1 − h2| + 1), where h1 and h2 are the heights of T1 and T2 and t counts as a constant.

- (a) Assume h1 = h2, and combine the trees in constant time.
- (b) Assume h1 = h2 + 1, and combine the trees in constant time.
- (c) For arbitrary heights, first augment the B-tree so that every node x knows how tall its subtree is. Explain how ordinary insertion and deletion keep this information up to date without changing the asymptotic cost of any operation.
- (d) Give an O(|h1 − h2| + 1) algorithm that combines T1 and T2 for arbitrary h1 and h2.

## Your task

Copy the `BTree` from [`0202`](../0202-b-tree/README.md) and keep building on it (this exercise doesn't use `lower_bound` or `scan`, so you may leave them out). `Key` is only guaranteed to support copying, moving, and `operator<`. The invariants are the same as in 0202, and the tests walk `Node` to check them.

Stage 1 covers part (c), and Stages 2 to 4 cover parts (a), (b), and (d). Unlike part (c), the height is stored once for the whole tree rather than in every node, and the cost is measured as the number of keys moved, with `t` as an explicit factor.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## The combine contract

`BTree::combine(left, key, right)`:

- `left` and `right` must have the same `t`; otherwise throw `std::invalid_argument`. It is guaranteed that every key of `left` `< key <` every key of `right` (don't check this; the check alone could cost more than the merge).
- The returned tree holds every key of all three. Both `left` and `right` are emptied (`size() == 0`, `root() == nullptr`) and remain usable afterwards.
- You may move only **`O(t · (|h1 − h2| + 1))`** keys, where `h1` and `h2` are the heights of the two trees. The tests use a counting `Key` that charges every copy, move, and comparison of a key as work, so reinserting a whole tree blows straight through the limit.

## Stage 1 — Height

```cpp
template <typename Key>
class BTree {
public:
    struct Node {
        std::vector<Key> keys;
        std::vector<std::unique_ptr<Node>> children;

        bool is_leaf() const { return children.empty(); }  // provided
    };

    explicit BTree(std::size_t min_degree);

    BTree(const BTree&) = delete;
    BTree& operator=(const BTree&) = delete;

    BTree(BTree&& other);             // should be noexcept once implemented
    BTree& operator=(BTree&& other);  // should be noexcept once implemented

    std::size_t min_degree() const;
    std::size_t size() const;
    bool empty() const;
    const Node* root() const;

    int height() const;  // the only new member; everything else comes from 0202

    bool contains(const Key& key) const;
    bool insert(Key key);

    bool erase(const Key& key);
};
```

`height()`: return the number of edges from the root to a leaf, in O(1). An empty tree has height `-1`, and a tree with only a root has height `0`. Both `insert` and `erase` must maintain it.

## Stage 2 — Equal heights

```cpp
// in class BTree<Key>
static BTree combine(BTree&& left, Key key, BTree&& right);  // Stages 2 to 4
```

`h1 == h2`: merge the two roots and `key` into one node. If it doesn't fit (more than `2t-1` keys), split it at the median to create a new root, and the height grows by 1.

`left` or `right` may be an empty tree.

## Stage 3 — A height difference of 1

`|h1 − h2| == 1`: merge the shorter tree's root and `key` into the rightmost (or leftmost) child of the taller tree; the splits may cascade all the way up to the root.

The root may hold very few keys, because it is the only B-tree node allowed to have fewer than `t-1` keys, so work out carefully which key counts the merged node can end up with.

## Stage 4 — Arbitrary heights

`combine` must be correct for any heights, including an empty tree combined with a tall one.

Hint: walk down the taller tree's **right spine** (or left spine) to the level that matches the shorter tree's height, merge there, and then split on the way back up. Without parent pointers, you must record the path yourself. Handle both directions (left taller and right taller).

## Stage 5 — Real-world extension: merging adjacent partitions

```cpp
// in class BTree<Key>
static BTree concat(BTree&& left, BTree&& right);
```

`BTree::concat(left, right)`: there is no separator key; it is only guaranteed that every key of `left` is less than every key of `right` (either may be empty). Take one key out to serve as the separator, then use `combine`. The number of keys moved may be at most on the order of `O(t · log n)`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. The original problem has every node record its height, but this exercise stores it only on the tree. How do the two approaches differ? When would you need every node to record it?
2. Why is the total cost of `combine` O(|h1 − h2| + 1) rather than O(log n)? If you combine `n` trees into one, one after another, what is the total cost?
3. How would you implement the reverse operation, `split(T, k)`? If `size()` must also stay O(1), what else do you need to store in the nodes?

---

**Official solution:** [Solutions to Problem Set 2](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/d608c09d3f727411722e3ff5a95b103f_MIT6_046JS15_pset2sols.pdf). Read it after you finish.
