---
title: Binary Tree and Traversal Order
unit: 04 · Trees & Heaps (PS4)
source: 6.006 Lecture 6, Recitation 6
pdf: materials/6.006/recitations/rec06.pdf
read: materials/6.006/lectures/lec06.pdf
kind: code
minutes: 100
---

# 0401 · Binary Tree and Traversal Order

**Based on:** [Recitation 6 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/e215840fd7b0e8cb07916a65c17252b2_MIT6_006S20_r06.pdf) · **Read first:** [Lecture 6: Binary Trees, Part 1 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/376714cc85c6c784d90eec9c575ec027_MIT6_006S20_lec6.pdf)

> **Why this comes up in practice:** `std::map`, a database's B+Tree, and the OS's VMA tree all rest on the same idea: the in-order traversal of the nodes defines the order, which turns changing the set into local pointer operations. This exercise drills these fundamentals until you no longer have to think about them.

## Your task

This exercise is the common foundation for the AVL tree, Set, and Sequence exercises that follow. Implement an owning `BinaryTree<T>` (each node has `parent`, `left`, and `right`), and think about every operation in terms of **traversal order (in-order)**; this exercise does **no balancing**.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Navigation (O(h))

```cpp
template <typename T>
class BinaryTree {
public:
    struct Node {
        explicit Node(T value) : item(std::move(value)) {}
        T item;
        Node* parent = nullptr;
        Node* left = nullptr;
        Node* right = nullptr;
    };

    static Node* subtree_first(Node* node);
    static Node* subtree_last(Node* node);
    static Node* successor(Node* node);
    static Node* predecessor(Node* node);
};
```

These are all static functions, and they can be called on any node:

- `subtree_first(n)` / `subtree_last(n)`: the first/last node of `n`'s subtree in traversal order. If `n` is `nullptr`, return `nullptr`.
- `successor(n)` / `predecessor(n)`: the next/previous node in the whole tree, or `nullptr` if there is none.

## Stage 2 — Insertion and traversal

```cpp
// in class BinaryTree<T>
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

BinaryTree() = default;
~BinaryTree();  // release every node without recursion

BinaryTree(const BinaryTree&) = delete;
BinaryTree& operator=(const BinaryTree&) = delete;

Node* root() const;
std::size_t size() const;
bool empty() const;
Node* insert_before(Node* anchor, T value);
Node* insert_after(Node* anchor, T value);
iterator begin() const;
iterator end() const;
```

| Operation | Behavior |
| --- | --- |
| `insert_before(a, v)` | If `a` has no left child, the new node becomes `a`'s left child; otherwise it becomes the right child of the last node in `a`'s left subtree |
| `insert_after(a, v)` | The mirror image |
| `insert_before(nullptr, v)` / `insert_after(nullptr, v)` | Attach at the end / at the start of the whole tree, respectively |
| `root()` / `size()` / `empty()` | |
| `begin()` / `end()` | Forward iterators; walking the whole tree is O(n) (`std::forward_iterator` is checked) |

Nodes are created with `new` and then owned by the tree. The destructor must free every node and **must not recurse**: on a tree that has degenerated into a linked list, recursive destruction overflows the stack.

## Stage 3 — Deletion

```cpp
// in class BinaryTree<T>
T erase(Node* node);
```

`erase(node)`: remove `node` from the tree, return its value, and preserve the traversal order of the remaining nodes. O(h).

**Only the erased `node` is invalidated; every other `Node*` must still point to the same item.** So you can't swap the item with the successor's the way the lecture's Python version does; relink pointers to move nodes into the right place instead (the `successor` is a node you already have in hand, which is cleaner than moving items).

Edge cases: a leaf, a node with one child, a node with two children, erasing the root, and a `successor` that is exactly the right child.

## Stage 4 — Building in O(n)

```cpp
// in class BinaryTree<T>
BinaryTree(BinaryTree&& other);             // should be noexcept once implemented
BinaryTree& operator=(BinaryTree&& other);  // should be noexcept once implemented

static BinaryTree build(std::vector<T> items);
```

- `static BinaryTree build(std::vector<T> items)`: build a tree whose traversal order equals `items`, in O(n) and with height `⌊lg n⌋` (except for the empty tree), moving the items into the nodes.
- Move constructor / move assignment: the moved-from tree becomes empty, and both must be `noexcept`. A self-move-assignment (`t = std::move(t)`) must leave the tree unchanged; the tests check it.

## Stage 5 — Set interface: Binary Search Tree

```cpp
template <typename K, typename Compare = std::less<K>>
class BstSet {
public:
    explicit BstSet(Compare compare = Compare{});

    bool insert(K key);
    bool contains(const K& key) const;
    bool erase(const K& key);
    std::size_t size() const;
    bool empty() const;
    std::optional<K> find_min() const;
    std::optional<K> find_max() const;
    std::optional<K> find_next(const K& key) const;
    std::optional<K> find_prev(const K& key) const;
    std::vector<K> to_vector() const;
    int height() const;
};
```

Use `BinaryTree<K>` to implement `BstSet<K, Compare>`: the traversal order is the sorted order of the keys. Every operation is O(h):

- `insert(k)` (returns `true` for a new key), `contains(k)`, `erase(k)` (returns `true` if it removed a key), `size()`, and `empty()`.
- `find_min()` / `find_max()`: return the first/last key in `Compare` order as a `std::optional<K>` (`std::nullopt` when the set is empty).
- `find_next(k)`: the smallest key `> k`; `find_prev(k)`: the largest key `< k`. `k` itself need not be present.
- `to_vector()`: all the keys, sorted. `height()`: `-1` for an empty tree, otherwise the number of edges from the root to the deepest leaf (O(n) is fine).

Remember that this tree degenerates: if you insert keys in increasing order, its height is `n - 1`. The next exercise fixes that.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is `successor` O(h) in the worst case, while walking the whole tree takes only O(n)?
2. The lecture's delete is simple to write because it swaps items. In C++, what does relinking pointers instead cost you, and what does it buy you?
3. Can a `std::set` iterator still be used after an `erase`? Can your design offer the same guarantee?
