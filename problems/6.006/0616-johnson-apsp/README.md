---
title: Johnson's Algorithm（全點對最短路徑與路由表）
unit: 06 · Weighted Shortest Paths（PS6）
source: 6.006 PS6 Problem 6-6, Lecture 14
pdf: materials/6.006/psets/ps6/questions.pdf
answer: materials/6.006/psets/ps6/solutions.pdf
read: materials/6.006/lectures/lec14.pdf, materials/6.006/recitations/rec14.pdf
kind: code
minutes: 110
---

# 0616 · Johnson's Algorithm

> **實務上為什麼會遇到**：路由器的 forwarding table、機房間的流量工程、地圖服務的距離快取，都需要「任兩點」的答案。Johnson 把帶負權重的稀疏圖化成 V 次 Dijkstra，比 Floyd-Warshall 的 V³ 省非常多。

輸入是邊的清單，`n` 個頂點（`0 … n-1`），權重可以是負的，可以有平行邊和 self-loop。邊的兩端保證合法。

```cpp
struct Edge { int from; int to; std::int64_t weight; };
inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
```

一次只看一關，做完再往下讀。

## Stage 1 — Potential

`johnson_potentials(n, edges)`：回傳長度為 `n` 的 `h`，使得**每條邊**都滿足 `weight + h[from] - h[to] >= 0`。圖裡任何地方有負環（不管從哪裡走得到）時回傳 `std::nullopt`。

要 O(V · E)。

## Stage 2 — 全點對最短路徑

`johnson(n, edges)`：回傳 `n × n` 的矩陣 `D`，`D[u][v]` 是 `u` 到 `v` 的最短路徑權重，到不了是 `kUnreachable`，`D[u][u]` 是 0。有負環時回傳 `std::nullopt`。

要 O(V² log V + V · E)。稀疏圖上要明顯快過 O(V³) 和 V 次 Bellman-Ford。

## Stage 3 — 實務延伸：路由表

做一個 `RoutingTable`：

- `static std::optional<RoutingTable> build(n, edges)`：有負環時回傳 `std::nullopt`。
- `distance(from, to)`：同 Stage 2，O(1)。
- `next_hop(from, to)`：從 `from` 要往 `to` 時的下一個頂點；`from == to` 或到不了時回傳 `std::nullopt`。O(1)。
- `route(from, to)`：沿著 `next_hop` 一跳一跳走出來的完整路線（含兩端）；到不了回傳 `std::nullopt`，`from == to` 時只有一個頂點。

頂點不存在時丟 `std::out_of_range`。

路由器只知道自己的下一跳，所以 **從任何頂點開始一跳一跳走，都必須真的走到 `to`，而且路徑總權重等於 `distance(from, to)`**。圖裡可能有權重總和為 0 的環。

## 面試追問

1. 為什麼加上超級節點算出來的 `h` 一定能讓所有權重變成非負？為什麼不能直接把最小權重加到每條邊上？
2. 改權重之後，為什麼所有最短路徑都不變？要怎麼從 `D'` 換回 `D`？
3. 有 0 權重的環時，如果每個頂點各自獨立挑一條最短路徑，下一跳可能會出什麼事？你靠什麼避開的？
