---
title: Changeable Priority Queue 與 Dijkstra
unit: 06 · Weighted Shortest Paths（PS6）
source: 6.006 Lecture 13, Recitation 13
pdf: materials/6.006/lectures/lec13.pdf
read: materials/6.006/recitations/rec13.pdf
kind: code
minutes: 120
---

# 0603 · Changeable Priority Queue 與 Dijkstra

> **實務上為什麼會遇到**：地圖導航、網路路由（OSPF）、遊戲尋路全都建立在 Dijkstra 上，而它的效能幾乎完全取決於 priority queue。真正的線上服務還會在意一件事：每次查詢不能把整張圖的狀態都重設一遍。

一次只看一關，做完再往下讀。

## Stage 1 — 可以改 key 的 Priority Queue

實作 `IndexedMinHeap<Key>`，裡面的項目用 `0 … capacity-1` 的 `id` 識別，一個 `id` 同時只能出現一次。`Key` 只保證有 `operator<`，可以移動和複製。

| 操作 | 行為 | 複雜度 |
| --- | --- | --- |
| `IndexedMinHeap(capacity)` | `capacity < 0` 丟 `std::invalid_argument` | O(capacity) |
| `empty()` / `size()` | | O(1) |
| `contains(id)` | `id` 不合法丟 `std::out_of_range` | O(1) |
| `key(id)` | 回傳目前的 key；不在 heap 裡丟 `std::out_of_range` | O(1) |
| `push(id, key)` | 已經存在時丟 `std::invalid_argument`；`id` 不合法丟 `std::out_of_range` | O(log n) |
| `pop_min()` | 移除並回傳 key 最小的 `{id, key}`；空的時候丟 `std::out_of_range` | O(log n) |
| `decrease_key(id, key)` | 新 key 比目前小才更新並回傳 `true`，否則不動並回傳 `false`；`id` 不在 heap 裡丟 `std::out_of_range` | O(log n) |
| `clear()` | 清空 | **O(目前的元素數量)**，不是 O(capacity) |

## Stage 2 — Dijkstra

```cpp
struct Arc { int to; std::int64_t weight; };
using WeightedGraph = std::vector<std::vector<Arc>>;
inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
inline constexpr int kNoParent = -1;
```

`dijkstra(g, s)` 回傳 `ShortestPaths{dist, parent}`：到不了是 `kUnreachable`；`parent` 是最短路徑樹，`s` 和到不了的頂點是 `kNoParent`。起點不存在丟 `std::out_of_range`，圖中任何邊的權重為負丟 `std::invalid_argument`。

用你的 `IndexedMinHeap`。稀疏圖上要 O(E log V)。

## Stage 3 — 實務延伸：查詢引擎

```cpp
class RouteEngine {
public:
    explicit RouteEngine(WeightedGraph graph);  // takes ownership; negative weight throws invalid_argument
    std::optional<std::int64_t> distance(int from, int to);
    std::optional<std::vector<int>> route(int from, int to);  // the vertices from `from` to `to`
};
```

查詢很多次、大多數起訖點都離得很近。到不了回傳 `std::nullopt`；頂點不存在丟 `std::out_of_range`。

**每次查詢的成本只能跟實際探索到的頂點數有關**：探索到終點就要停，也不可以為了清狀態把整張圖掃一遍（一百萬個頂點、幾十萬次查詢）。

## 面試追問

1. Dijkstra 碰到負邊時具體壞在哪一步？能不能舉出一個最小反例？
2. 什麼時候用 Array（O(V²)）比 binary heap 好？什麼時候 lazy deletion 比 `decrease_key` 好？
3. `RouteEngine` 的內部暫存要怎麼設計，才能讓多個執行緒同時查詢？
