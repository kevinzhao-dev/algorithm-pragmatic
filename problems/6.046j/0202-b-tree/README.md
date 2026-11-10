---
title: B-Tree (insertion, deletion, and range scans)
unit: 02 · FFT and B-Trees (PS2)
source: 6.046J Recitation 2
pdf: materials/6.046j/recitations/rec02.pdf
read: materials/6.046j/recitations/rec02.pdf
kind: code
minutes: 180
---

# 0202 · B-Tree

**Based on:** [B-trees](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/8fd15588f584c518b6ec83aedb2b96c9_MIT6_046JS15_Recitation2.pdf) · **Read first:** [B-trees](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/8fd15588f584c518b6ec83aedb2b96c9_MIT6_046JS15_Recitation2.pdf)

> **Why this comes up in practice:** database indexes and file systems use B-trees because one node fills exactly one disk page, so the tree's height is the number of disk reads. Even in memory, the pointer chasing of `std::set` can't match one contiguous block of keys.

## Your task

Implement a B-tree parameterized by its minimum degree `t` (`t ≥ 2`). `Key` is only guaranteed to support copying, moving, and `operator<`.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Invariants

The tests walk the public `Node` structure directly and check that:

- Every non-root node holds a number of keys in `[t-1, 2t-1]`; the root holds 1 to `2t-1` keys; and an empty tree's `root()` is `nullptr`.
- An internal node has `keys.size() + 1` children, and a leaf has none (`children` is empty).
- All leaves are at the same depth, and the height is at most `⌊log_t((n+1)/2)⌋`.
- The keys within each `keys` are increasing, and an in-order traversal of the whole tree is strictly increasing.

## Stage 1 — Search and insertion

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

    bool contains(const Key& key) const;
    bool insert(Key key);
};
```

| Operation | Behavior |
| --- | --- |
| `BTree(t)` | Throw `std::invalid_argument` if `t < 2` |
| `contains(key)` | `O(t log_t n)` |
| `insert(key)` | Return `true` if the key was added; if it already exists, return `false` and leave the tree unchanged |
| `size()`, `empty()`, `min_degree()`, `root()` | O(1) |
| Move construction and assignment | The destination takes over the tree together with its `t`; the source becomes a valid empty tree (it can still be used) and keeps its `t` |

When inserting, you should split full nodes on the way down, so that you never have to walk back up. When the root splits, the height grows by 1.

## Stage 2 — Deletion

```cpp
// in class BTree<Key>
bool erase(const Key& key);
```

`erase(key)`: if the key exists, delete it and return `true`; otherwise return `false` and leave the tree unchanged.

On the way down, make sure that the child you are about to enter has at least `t` keys. When the key sits in an internal node, replace it with its predecessor or successor. When merges leave the root with no keys, remove the root and let the height drop by 1; after the last key is deleted, `root()` goes back to `nullptr`.

## Stage 3 — Real-world extension: range scans

```cpp
// in class BTree<Key>
std::optional<Key> lower_bound(const Key& key) const;

template <typename Visit>
void scan(const Key& low, const Key& high, Visit visit) const;
```

- `lower_bound(key)`: return the smallest key that is not less than `key`, or `std::nullopt` if there is none, in `O(t log_t n)`.
- `scan(low, high, visit)`: call `visit(key)` on each key in `[low, high)`, in order. With `k` the number of keys in the range, it must make `O(t log_t n + k)` comparisons; in other words, the Counted test rejects any approach that walks the whole tree.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Using a 2-3-4 tree (`t = 2`) as the example, why does splitting in advance during insertion avoid backtracking? What does it cost?
2. Given a disk page size, how do you choose `t`? Why do databases almost always use a B+ tree rather than this version?
3. Why do write-heavy systems move to LSM-trees? Compare the two in terms of write amplification.
