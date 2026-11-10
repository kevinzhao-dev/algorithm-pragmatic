---
title: 距離向量路由（幾跳就收斂的最短路徑）
unit: 06 · Weighted Shortest Paths（PS6）
source: 6.006 PS6 Problem 6-2
pdf: materials/6.006/psets/ps6/questions.pdf
answer: materials/6.006/psets/ps6/solutions.pdf
read: materials/6.006/lectures/lec12.pdf, materials/6.006/recitations/rec12.pdf
kind: code
minutes: 70
---

# 0612 · 距離向量路由

> **實務上為什麼會遇到**：Distance-vector 協定（RIP 這一類）就是一輪一輪交換路由表的 Bellman-Ford，真實網路的直徑很小，所以幾輪就收斂了。「最多轉 k 次機的最便宜機票」也是同一種問題，而且換個寫法答案就會錯。

輸入是邊的清單，`n` 個頂點（`0 … n-1`），權重可以是負的。起點不存在時丟 `std::out_of_range`。

```cpp
struct Edge { int from; int to; std::int64_t weight; };
inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
```

一次只看一關，做完再往下讀。

## Stage 1 — 不知道 k 的收斂

保證：對每個從 `s` 可達的頂點 `v`，都存在某條 min-weight path 只用 `k` 條邊以內。**`k` 不會告訴你**，但它可能遠小於 V。

`converge_routes(n, edges, s)`：回傳 `dist`，到不了是 `kUnreachable`。要 O(V + k · E)，圖有上百萬條邊、`k` 只有幾十時不能跑滿 V 輪。

## Stage 2 — 跳數限制

`hop_limited_routes(n, edges, s, hops)`：回傳「最多走 `hops` 條邊」的最小權重。這裡**沒有 Stage 1 的保證**，也可能有負環，走法可以重複經過頂點（是 walk）；`hops == 0` 時只有 `s` 是 0，其他都是 `kUnreachable`。

要 O(hops · (V + E))。

## Stage 3 — 量測收斂需要的跳數

`convergence_hops(n, edges, s)`：保證同 Stage 1，回傳最小的 `k`，使得每個可達頂點都有一條最短路徑只用 `k` 條邊以內。只有 `s` 自己可達時回傳 0。

要 O(V + k · E)。

## 面試追問

1. Stage 1 為什麼可以用「這一輪沒有任何更新就停」當條件？如果 `k` 本來就知道，可以少做什麼？
2. 為什麼 Stage 2 不能直接用 Stage 1 的迴圈做 `hops` 輪？舉一個具體的小例子。
3. 真實的 RIP 為什麼要有跳數上限？count-to-infinity 跟這裡的負環有什麼相似之處？
