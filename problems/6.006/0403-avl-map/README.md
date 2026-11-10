---
title: AVL Set／Map（order statistics）
unit: 04 · Trees & Heaps（PS4）
source: 6.006 Lecture 7, Recitation 7
pdf: materials/6.006/recitations/rec07.pdf
read: materials/6.006/lectures/lec07.pdf, materials/6.006/recitations/rec06.pdf
kind: code
minutes: 80
---

# 0403 · AVL Set／Map

> **實務上為什麼會遇到**：`std::map` 的介面、資料庫的 ordered index、排行榜的「第幾名」，都是一棵帶 subtree size 的平衡樹。

用 0402 的 `AvlTree` 在 `solution.hpp` 實作有序的 key-value Map：traversal order 就是排序好的 key。請直接 `#include "../0402-avl-tree/solution.hpp"`，重用你寫好的引擎，不要重寫平衡邏輯。

```cpp
template <typename K, typename V, typename Aug = NoAugment, typename Compare = std::less<K>>
class AvlMap;
```

`struct Item { K key; V value; }`，`Node` 就是 `AvlTree<Item, Aug>::Node`。每個操作都是 O(log n)。

## Stage 1 — Set 介面

| 操作 | 行為 |
| --- | --- |
| `insert(k, v = V{})` | 新 key 回傳 `true`；key 已存在時**覆蓋 value** 並回傳 `false` |
| `find(k)` | 回傳節點，沒有時 `nullptr` |
| `contains(k)` / `erase(k)` | `erase` 有刪到回傳 `true` |
| `set_value(node, v)` | 改掉節點的 value，並維持 augmentation |
| `min()` / `max()` | 回傳節點，空樹回傳 `nullptr` |
| `size()` / `empty()` / `tree()` | `tree()` 回傳底層樹的 const reference |

`AvlSet<K, Compare>`（已提供）就是 value 為 `std::monostate` 的 `AvlMap`。

## Stage 2 — 排序查詢

- `find_next(k)`：key 最小且 `> k` 的節點；`find_prev(k)`：key 最大且 `< k` 的節點。沒有時回傳 `nullptr`。`k` 本身不一定存在。

## Stage 3 — Order Statistics

用節點上的 `size`：

- `rank(k)`：比 `k` 小的 key 有幾個（`k` 不必存在）。
- `select(i)`：第 `i` 小（0 起算）的節點；`i >= size()` 時丟 `std::out_of_range`。

## Stage 4 — 實務延伸：排行榜

`Leaderboard`：玩家用字串 ID 識別，分數可正可負。

- `upsert(player, score)`：新增玩家或更新分數。
- `remove(player)`：回傳玩家是否存在。
- `rank_of(player)`：名次，0 是第一名；分數高的在前，同分時 ID 字典序小的在前。不存在回傳 `std::nullopt`。
- `player_at(rank)`：該名次的玩家，超出範圍時 `std::nullopt`。
- `top(k)`：前 `k` 名的 `(player, score)`，由第一名開始。`k` 超過人數時回傳全部。
- `size()`。

除了 `top`（O(k + log n)）以外，每個操作都是 O(log n)（期望，因為要用 hash map 從 ID 找到分數）。

## 面試追問

1. 比較 hash map 和 balanced BST：什麼需求一出現，hash map 就不夠用了？
2. `rank` 和 `select` 為什麼一定要靠 subtree size？如果只有 `height`，能做到嗎？
3. 排行榜要支援「分數區間內有幾人」，你會加什麼 augmentation？（需要嗎？）
