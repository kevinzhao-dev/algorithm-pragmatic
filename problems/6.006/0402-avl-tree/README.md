---
title: AVL Tree Engine (rotations and subtree augmentation)
unit: 04 · Trees & Heaps (PS4)
source: 6.006 Lecture 7, Recitation 7
pdf: materials/6.006/recitations/rec07.pdf
read: materials/6.006/lectures/lec07.pdf
kind: code
minutes: 150
---

# 0402 · AVL Tree Engine

**Based on:** [Recitation 7 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/16d43dec3488717057d7daff2bac3400_MIT6_006S20_r07.pdf) · **Read first:** [Lecture 7: Binary Trees, Part 2: AVL notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/a2c80596cf4a2b5fbc854afdd2f23dcb_MIT6_006S20_lec7.pdf)

> **Why this comes up in practice:** the core of every balanced tree (AVL, Red-Black) is nothing more than rotations plus repairs from a leaf up to the root. Once you've learned that, requirements like order statistics and range queries come down to maintaining one more summary per subtree, and this kind of augmentation is the most common way to score bonus points on data structure questions.

## Your task

Copy over the `BinaryTree` from 0401, then add heights, rotations, and balancing along the way. The Set and Sequence of the next two exercises are both built on this engine, and later exercises reuse it too, so please follow the specified interface.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Nodes with heights and sizes

```cpp
template <typename T, typename Aug = NoAugment>
class AvlTree {
public:
    struct Node {
        explicit Node(T value) : item(std::move(value)), aug(Aug::of(item)) {}
        T item;
        Node* parent = nullptr;
        Node* left = nullptr;
        Node* right = nullptr;
        int height = 0;          // leaf: 0, empty subtree counts as -1
        std::size_t size = 1;    // number of nodes in this subtree
        typename Aug::Value aug;
    };

    class iterator {
    public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        iterator() = default;
        reference operator*() const;
        pointer operator->() const;
        iterator& operator++();
        iterator operator++(int);
        friend bool operator==(const iterator&, const iterator&);
    };

    AvlTree() = default;
    ~AvlTree();  // release every node

    AvlTree(const AvlTree&) = delete;
    AvlTree& operator=(const AvlTree&) = delete;

    Node* root() const;
    std::size_t size() const;
    bool empty() const;
    iterator begin() const;
    iterator end() const;

    static Node* subtree_first(Node* node);
    static Node* subtree_last(Node* node);
    static Node* successor(Node* node);
    static Node* predecessor(Node* node);

    Node* insert_before(Node* anchor, T value);
    Node* insert_after(Node* anchor, T value);
    T erase(Node* node);
};
```

The interface is the same as in 0401 (`root`, `size`, `empty`, `begin`/`end`, `subtree_first/last`, `successor/predecessor`, `insert_before/after`, `erase`), and so is the meaning of a `nullptr` anchor for `insert_before/after`. But `Node` has two more fields, which **must be correct for every node after every operation**:

- `height`: 0 for a leaf; an empty subtree counts as -1.
- `size`: the number of nodes in this subtree.

As before, `erase` may invalidate only the erased node, and it must not swap items. This stage doesn't balance yet.

## Stage 2 — Rotation

```cpp
// in class AvlTree<T, Aug>
Node* rotate_left(Node* node);
Node* rotate_right(Node* node);
```

`rotate_left(x)` (requires `x` to have a right child) / `rotate_right(x)` (requires a left child): O(1). Return the node that takes `x`'s place. The `traversal order` doesn't change, and the `height` and `size` (and, from Stage 4 on, the `aug`) of the two nodes that move must be recomputed. The ancestors' fields are the caller's responsibility; this step doesn't need to update them. The rotation itself must still link the new node into `x`'s old place, under `x`'s old parent or as the tree's root (the tests check `root()` after rotating the root).

## Stage 3 — Staying balanced

Every node must be height-balanced: the heights of its left and right subtrees differ by at most 1 (skew ∈ {-1, 0, 1}).

After every `insert_before/after` or `erase`, walk from the lowest point that changed up to the root, repairing with one or two rotations (a single rotation for the lecture's cases 1/2, a double rotation for case 3), and update `height` and `size` along the way. Every `Node*` must stay valid across rotations.

- Every operation is O(log n).
- After inserting `n` elements in sequence, the height must be ≤ `1.45 lg(n + 2)`.

## Stage 4 — Subtree Augmentation

```cpp
// An augmentation folds a monoid over the traversal order of every subtree.
struct NoAugment {
    struct Value {};
    static Value identity() { return {}; }
    template <typename T>
    static Value of(const T&) {
        return {};
    }
    static Value combine(const Value&, const Value&) { return {}; }
};

// in class AvlTree<T, Aug>
void refresh(Node* node);
```

The second template parameter, `Aug`, describes the summary to maintain on every subtree:

```cpp
struct SumAug {
    using Value = long long;                   // one copy stored in every node
    static Value identity() { return 0; }      // the empty subtree
    static Value of(const T& item);            // the summary of a single item
    static Value combine(const Value& a, const Value& b);  // not necessarily commutative: a comes first in traversal order
};
```

- For every node: `node->aug == combine(combine(aug(left), of(item)), aug(right))`, using `identity()` for a missing child. **`combine` is not guaranteed to be commutative** (string concatenation, matrix multiplication), so if you get the order wrong, the tests fail.
- Every `insert`/`erase`/`rotate` maintains this invariant while calling `combine` only O(log n) times (the tests allow `24 lg n + 24` calls per erase-plus-insert pair); you must not recompute the whole tree.
- `refresh(node)`: after the caller has modified `node->item` directly, recompute the `aug` values on the path from `node` to the root (O(log n), with at most `6 lg n + 8` calls to `combine` in the tests; heights and sizes don't change).

## Stage 5 — Building in O(n), and ownership

```cpp
// in class AvlTree<T, Aug>
AvlTree(AvlTree&& other);             // should be noexcept once implemented
AvlTree& operator=(AvlTree&& other);  // should be noexcept once implemented

static AvlTree build(std::vector<T> items);
```

- `static AvlTree build(std::vector<T> items)`: build an AVL tree whose traversal order equals `items`, in O(n). Call `combine` at most `3n` times.
- Move constructor / move assignment: the moved-from tree becomes empty, and both must be `noexcept`. A self-move-assignment (`t = std::move(t)`) must leave the tree unchanged; the tests check it.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is the height of an AVL tree bounded by `1.44 lg n`? (Write out the Fibonacci recurrence.) Compared with a Red-Black tree, what trade-offs does an AVL tree make?
2. At most how many rotations can an insertion need? Why might a deletion need O(log n) rotations?
3. Why doesn't a rotation change the traversal order? In the implementation, why must you update the child's fields **first** and the parent's afterwards?
