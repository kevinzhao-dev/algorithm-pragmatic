---
title: Bellman-Ford（負環偵測與回報）
unit: 06 · Weighted Shortest Paths（PS6）
source: 6.006 Lecture 12, Recitation 12
pdf: materials/6.006/lectures/lec12.pdf
read: materials/6.006/recitations/rec12.pdf
kind: code
minutes: 110
---

# 0602 · Bellman-Ford

> **實務上為什麼會遇到**：路由協定（distance-vector）的收斂、匯率套利的偵測、限制式系統（system of difference constraints）的求解，骨架都是 Bellman-Ford。現實資料帶著負權重時，Dijkstra 根本不能用。

輸入是邊的清單。頂點編號 `0 … n-1`，權重可以是負的。起點不存在時丟 `std::out_of_range`。一次只看一關，做完再往下讀。

```cpp
struct Edge { int from; int to; std::int64_t weight; };
inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
inline constexpr std::int64_t kNegativeInfinity = std::numeric_limits<std::int64_t>::min();
```

## Stage 1 — 最短距離

`bellman_ford(n, edges, s)`：回傳 `dist`，到不了的頂點是 `kUnreachable`。這一關保證從 `s` 可達的範圍內沒有負環。

要 O(V · E)，但在邊的順序很糟、圖又很長時也要撐得住。圖可能有平行邊和 self-loop（權重非負的 self-loop 不算負環）。

## Stage 2 — 負環影響的範圍

拿掉上一關的保證：現在從 `s` 可達的範圍內可能有負環。`bellman_ford(n, edges, s)` 要改成：

- 只要 `v` 可以經過某個負環走到（`s` 走得到那個負環，那個負環走得到 `v`），`dist[v] = kNegativeInfinity`。
- 其他頂點的 `dist` 維持正確的有限值或 `kUnreachable`。

從 `s` 走不到的負環不影響答案。

## Stage 3 — 把負環交出來

`find_negative_cycle(n, edges, s)`：回傳一個從 `s` 可達的負環（頂點序列，不重複，相鄰兩個加上最後接回第一個都有邊，總權重 < 0，平行邊取最輕的），沒有就回傳 `std::nullopt`。

要 O(V · E)。

## Stage 4 — 實務延伸：匯率套利

```cpp
struct Quote { int from; int to; double rate; };  // 1 unit of from buys `rate` units of to
```

`find_arbitrage(currencies, quotes)`：找一串貨幣，沿著 `quotes` 換一圈後資產會變多（匯率乘積 > 1）。回傳貨幣序列（不重複，換匯方向是依序、最後接回第一個），沒有套利機會回傳 `std::nullopt`。`rate <= 0` 丟 `std::invalid_argument`。

- 市場不一定連通，機會可能藏在任何角落。
- 匯率都是浮點數，有些報價表「理論上乘起來剛好等於 1」，不可以因為捨入誤差就回報假的機會；乘積超過 1 + 1e-6 的機會則必須回報。兩者之間的情況不測試。

## 面試追問

1. Bellman-Ford 為什麼 V - 1 輪就夠？這跟「最短路徑是 simple path」有什麼關係？
2. 為什麼負環上的頂點不一定是第 V 輪被更新的頂點？
3. 如果要的是「最賺的套利」而不只是「有套利」，這題還能用 Bellman-Ford 解嗎？
