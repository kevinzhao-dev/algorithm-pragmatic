---
title: Sorted Array Set（Binary Search 與順序查詢）
unit: 02 · Sorting & Recurrences（PS2）
source: 6.006 Lecture 3, Recitation 3
pdf: materials/6.006/recitations/rec03.pdf
read: materials/6.006/lectures/lec03.pdf
kind: code
minutes: 75
---

# 0201 · Sorted Array Set

> **實務上為什麼會遇到**：read-only 的索引（SSTable 的 index block、靜態設定檔的查找表）常常就是一個排好序的陣列加 binary search：比樹省記憶體、cache 也好。binary search 寫錯邊界也是面試最常見的翻車現場。

Lecture 3 的 Set interface：項目有唯一的 key，可以用 key 查找、也能問「比 k 大的下一個」。用一個依 key 排序的陣列實作，查找全部 O(log n)。**不可用 `std::map`、`std::set`、`std::lower_bound`**，binary search 要自己寫。排序本身這題先用標準函式庫（`std::stable_sort`），0202、0203 才親手寫。

`Key` 只保證有 `<` 和 `==`。測試用 `Counted` 計算比較次數，確認真的是 binary search。

一次只看一關，做完再往下讀。

## Stage 1 — Binary Search

`sorted` 是 `std::span<const T>`，已遞增排序、**可能有重複**：

- `lower_bound_index<T>(sorted, key)`：第一個 `sorted[i] >= key` 的 `i`（都小於 key 時回傳 `size()`）。
- `upper_bound_index<T>(sorted, key)`：第一個 `sorted[i] > key` 的 `i`（都不大於 key 時回傳 `size()`）。

比較次數 ≤ 2·⌈log₂(n+1)⌉ + 2。邊界：空陣列、key 比全部都小／都大、一整串都等於 key。

## Stage 2 — 靜態操作

`SortedArraySet<Key, Value>`，`Item` 是 `std::pair<Key, Value>`：

| 操作 | 行為 | 比較次數 |
| --- | --- | --- |
| `SortedArraySet()` / `explicit SortedArraySet(vector<Item>)` | 空的 / 建立，同 key 時**後出現的取代先出現的** | O(n log n) |
| `size()` | | |
| `find(k)` | 回傳指向項目的 `const Item*`，不存在時 `nullptr` | O(log n) |
| `find_min()` / `find_max()` | 最小／最大 key 的項目，空的時 `nullptr` | O(1) |
| `find_next(k)` | **嚴格大於** `k` 的最小 key 的項目 | O(log n) |
| `find_prev(k)` | **嚴格小於** `k` 的最大 key 的項目 | O(log n) |
| `items()` | `std::span<const Item>`，依 key 遞增 | O(1) |

注意 `k` 不一定存在於集合中。回傳的指標在下一次修改前有效。

## Stage 3 — 動態操作

- `insert(item)`：key 已存在時**取代** value，回傳 `false`；否則新增，回傳 `true`。比較次數 O(log n)（找位置），元素搬動 O(n) 無妨。
- `erase(k)`：移除並回傳該項目（`std::optional<Item>`），不存在回傳 `std::nullopt`。比較次數 O(log n)。

## Stage 4 — 實務延伸：時間序列索引

時間序列資料庫（或 LSM tree 的快照讀）常用 sorted array 索引：

- `range(lo, hi)`：回傳 `std::span<const Item>`，涵蓋所有 `lo ≤ key < hi` 的項目。回傳的是**內部陣列的一段 view**（zero-copy），比較次數 O(log n)。
- `as_of(t)`：`key ≤ t` 的項目中 key 最大的，沒有時回傳 `nullptr`（「時間 t 當下看到的最新版本」）。
- `lo > hi` 時 `range` 回傳空的 span。

## 面試追問

1. `lower_bound` 為什麼寫成 `low < high` 配上半開區間 `[low, high)` 最不容易出錯？`mid` 的算法為什麼要寫 `low + (high - low) / 2`？
2. 這個 Set 的 `insert` 是 O(n) 而 `find` 是 O(log n)：什麼樣的讀寫比例適合它？跟 hash table、平衡樹比，何時會選它？
3. 回傳指標或 span 之後呼叫 `insert`，為什麼這些指標可能失效？`std::vector` 的迭代器失效規則是什麼？
