---
title: 最短路徑的 DP 視角（Bellman-Ford 與 Floyd-Warshall）
unit: 08 · Dynamic Programming II（PS8）
source: 6.006 Lecture 17
pdf: materials/6.006/lectures/lec17.pdf
read: materials/6.006/lectures/lec17.pdf
kind: code
minutes: 80
---

# 0805 · 最短路徑的 DP 視角

> **實務上為什麼會遇到**：「最多轉 k 次機的最便宜航線」、路由表的 distance-vector、都是 Bellman-Ford 的 DP 表。Floyd-Warshall 則在小而密的圖上拿到全點對最短路徑，幾行就寫完，但負環和 overflow 處理才是真正的考點。

邊用 `Edge{from, to, weight}` 表示（`weight` 可為負，可有平行邊），頂點 `0..vertices−1`。

```cpp
inline constexpr std::int64_t kUnreachable  = std::numeric_limits<std::int64_t>::max();
inline constexpr std::int64_t kMinusInfinity = std::numeric_limits<std::int64_t>::min();
```

一次只看一關，做完再往下讀。

## Stage 1 — 最多 k 條邊

`std::vector<std::int64_t> shortest_within_edges(int vertices, std::span<const Edge> edges, int source, int max_edges)`：`result[v]` 是從 `source` 出發、走**最多 `max_edges` 條邊**到 `v` 的最小 weight（walk，可重複走同一條邊）；走不到時是 `kUnreachable`。O(max_edges · |E|) 時間。

每一輪只能依賴上一輪的結果，否則一輪內就可能走好幾條邊。

## Stage 2 — 全點對最短路徑

```cpp
class AllPairs {
public:
    AllPairs(int vertices, std::span<const Edge> edges);   // O(V^3)
    std::int64_t distance(int from, int to) const;
    std::vector<int> path(int from, int to) const;         // Stage 3
    bool has_negative_cycle() const;                       // Stage 4
};
```

這一關假設沒有負環。`distance(v, v) == 0`，走不到時 `kUnreachable`。O(V³) 時間，O(V²) 記憶體。要小心 `kUnreachable` 不可以被拿去相加。

## Stage 3 — 路徑

`path(from, to)`：回傳一條最短路徑上的所有頂點，含兩端；`from == to` 時是 `{from}`；走不到時回傳空 vector。圖可能有 zero-weight 的 cycle，路徑不可以繞圈，最多 `V` 個頂點。

## Stage 4 — 負環

允許圖上有負環。

- `has_negative_cycle()`：圖上任何地方有負環就回傳 `true`。
- `distance(from, to)`：若存在從 `from` 到 `to` 的 walk 可以任意變小（路徑上可以繞負環），回傳 `kMinusInfinity`；其他情況回傳正確的最短距離或 `kUnreachable`，不受遠處負環影響。
- `path` 在 `distance` 為 `kMinusInfinity` 時回傳空 vector。
- 不可以 overflow：負環會讓距離每一輪翻倍往下掉，要記得夾住。

## 面試追問

1. Stage 1 的滾動陣列為什麼必須複製一份，Bellman-Ford 原版又為什麼可以就地更新？
2. Floyd-Warshall 把第三維（`k`）壓掉為什麼仍然正確？負環存在時這個論點哪裡會失效？
3. 什麼時候應該用 Johnson 而不是 Floyd-Warshall？
