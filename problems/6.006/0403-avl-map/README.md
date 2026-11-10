---
title: AVL Set/Map (order statistics)
unit: 04 · Trees & Heaps (PS4)
source: 6.006 Lecture 7, Recitation 7
pdf: materials/6.006/recitations/rec07.pdf
read: materials/6.006/lectures/lec07.pdf, materials/6.006/recitations/rec06.pdf
kind: code
minutes: 80
---

# 0403 · AVL Set/Map

**Based on:** [Recitation 7 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/16d43dec3488717057d7daff2bac3400_MIT6_006S20_r07.pdf) · **Read first:** [Lecture 7: Binary Trees, Part 2: AVL notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/a2c80596cf4a2b5fbc854afdd2f23dcb_MIT6_006S20_lec7.pdf), [Recitation 6 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/e215840fd7b0e8cb07916a65c17252b2_MIT6_006S20_r06.pdf)

> **Why this comes up in practice:** the `std::map` interface, a database's ordered index, and a leaderboard's "what place am I in?" are all a balanced tree that carries subtree sizes.

## Your task

Use the `AvlTree` from 0402 to implement an ordered key-value Map in `solution.hpp`: the traversal order is the sorted order of the keys. `#include "../0402-avl-tree/solution.hpp"` directly and reuse the engine you wrote; don't rewrite the balancing logic.

```cpp
template <typename K, typename V, typename Aug = NoAugment, typename Compare = std::less<K>>
class AvlMap;
```

`struct Item { K key; V value; }`, and `Node` is `AvlTree<Item, Aug>::Node`. Every operation is O(log n).

Each stage lists the declarations it adds, copied from `solution.hpp`.

## Stage 1 — Set interface

```cpp
template <typename K, typename V, typename Aug = NoAugment, typename Compare = std::less<K>>
class AvlMap {
public:
    struct Item {
        K key;
        V value;
    };
    using Tree = AvlTree<Item, Aug>;
    using Node = typename Tree::Node;

    explicit AvlMap(Compare compare = Compare{});

    Node* find(const K& key) const;
    bool contains(const K& key) const;
    bool insert(K key, V value = V{});
    bool erase(const K& key);
    void set_value(Node* node, V value);
    std::size_t size() const;
    bool empty() const;
    const Tree& tree() const;
    Node* min() const;
    Node* max() const;
};

template <typename K, typename Compare = std::less<K>>
using AvlSet = AvlMap<K, std::monostate, NoAugment, Compare>;  // provided
```

| Operation | Behavior |
| --- | --- |
| `insert(k, v = V{})` | Return `true` for a new key; if the key already exists, **overwrite the value** and return `false` |
| `find(k)` | Return the node, or `nullptr` if the key is absent |
| `contains(k)` / `erase(k)` | `erase` returns `true` if it removed a key |
| `set_value(node, v)` | Replace the node's value and maintain the augmentation |
| `min()` / `max()` | Return the first/last node in `Compare` order, or `nullptr` for an empty tree |
| `size()` / `empty()` / `tree()` | `tree()` returns a const reference to the underlying tree |

`AvlSet<K, Compare>` (provided) is just an `AvlMap` whose value is `std::monostate`.

## Stage 2 — Ordered queries

```cpp
// in class AvlMap<K, V, Aug, Compare>
Node* find_next(const K& key) const;
Node* find_prev(const K& key) const;
```

- `find_next(k)`: the node with the smallest key `> k`; `find_prev(k)`: the node with the largest key `< k`. Return `nullptr` if there is none. `k` itself need not be present.

## Stage 3 — Order Statistics

```cpp
// in class AvlMap<K, V, Aug, Compare>
std::size_t rank(const K& key) const;
Node* select(std::size_t index) const;
```

Use the `size` stored on each node:

- `rank(k)`: how many keys are smaller than `k` (`k` need not be present).
- `select(i)`: the node with the `i`-th smallest key (counting from 0); throw `std::out_of_range` if `i >= size()`.

## Stage 4 — Real-world extension: leaderboard

```cpp
class Leaderboard {
public:
    void upsert(const std::string& player, std::int64_t score);
    bool remove(const std::string& player);
    std::optional<std::size_t> rank_of(const std::string& player) const;
    std::optional<std::string> player_at(std::size_t rank) const;
    std::vector<std::pair<std::string, std::int64_t>> top(std::size_t count) const;
    std::size_t size() const;
};
```

`Leaderboard`: players are identified by string IDs, and scores can be positive or negative.

- `upsert(player, score)`: add a player or update their score.
- `remove(player)`: return whether the player existed.
- `rank_of(player)`: the player's place, where 0 is first place. Higher scores come first, and among equal scores the lexicographically smaller ID comes first. Return `std::nullopt` if the player doesn't exist.
- `player_at(rank)`: the player in that place, or `std::nullopt` if it is out of range.
- `top(k)`: the `(player, score)` pairs of the top `k` places, starting from first place. If `k` exceeds the number of players, return all of them.
- `size()`.

Every operation except `top` (O(k + log n)) is O(log n) (expected, because you need a hash map to find a score from an ID).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Compare a hash map with a balanced BST: which requirements make a hash map insufficient the moment they appear?
2. Why must `rank` and `select` rely on subtree sizes? Could you do it with only `height`?
3. If the leaderboard had to answer "how many players have a score in a given range", what augmentation would you add? (Do you need one?)
