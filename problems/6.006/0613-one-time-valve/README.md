---
title: 重力管線的一次性閥門（狀態改變地形）
unit: 06 · Weighted Shortest Paths（PS6）
source: 6.006 PS6 Problem 6-3
pdf: materials/6.006/psets/ps6/questions.pdf
answer: materials/6.006/psets/ps6/solutions.pdf
read: materials/6.006/lectures/lec11.pdf, materials/6.006/recitations/rec11.pdf
kind: code
minutes: 80
---

# 0613 · 重力管線的一次性閥門

> **實務上為什麼會遇到**：「某個動作會永久改變接下來可以走的路」在排程和配置變更裡很常見，例如切換 feature flag 之後整個呼叫圖都變了。關鍵是想清楚：動作前和動作後，你走的其實是同一張圖嗎？

一個靠重力輸水的管線網路，檢修機器人順著水流前進。

```cpp
struct Pipe { int a; int b; std::int64_t length; };       // length > 0
struct HeadDrop { int junction; std::int64_t new_head; }; // new_head < head[junction]

struct PipeNetwork {
    std::vector<std::int64_t> head;  // water head of each junction
    std::vector<Pipe> pipes;
    std::vector<HeadDrop> drops;     // at most one per junction; none at `valve`
    int valve;                       // the junction with the relief valve
};
```

規則：

- 機器人只能走 `head` **嚴格往下**的方向：從 `x` 經過管線到 `y` 需要當下 `head[x] > head[y]`。同高的兩端完全不能走。
- 當機器人正在 `valve` 這個交點時，可以選擇觸發閥門（最多一次，不花時間）。觸發後，`drops` 裡所有交點的 `head` 立刻永久變成 `new_head`，之後都用新的 `head` 判斷能不能走。不一定要觸發。

`V = head.size()`、`E = pipes.size()`，兩者都可能到一百萬以上。起點或終點不存在時丟 `std::out_of_range`。一次只看一關，做完再往下讀。

## Stage 1 — 不會改變的管線

`shortest_drift(network, source, sink)`：這一關 `drops` 永遠是空的（閥門沒有作用）。回傳從 `source` 到 `sink` 的最短總長度，到不了回傳 `std::nullopt`；`source == sink` 時是 0。

要 O(V + E)。

## Stage 2 — 閥門

`drops` 不再是空的。同樣回傳最短總長度：機器人可以在路上經過 `valve` 時決定要不要觸發。

要 O(V + E)。

## Stage 3 — 交出路線

`plan_drift(network, source, sink)` 回傳 `DriftPlan`：

```cpp
struct DriftPlan {
    std::int64_t length;
    std::vector<int> route;              // junctions in order, source to sink
    std::optional<std::size_t> trigger;  // index in route at which the valve fires, if ever
};
```

`route[*trigger]` 必須是 `valve`。沒有路線時回傳 `std::nullopt`。最短路線不只一條時，回傳任一條都算對。

## 面試追問

1. 為什麼這題不需要 Dijkstra，也不需要 Bellman-Ford？你的演算法靠的是圖的什麼性質？
2. 如果閥門可以觸發很多次，或者有好幾個不同的閥門，你的建模會怎麼變？複雜度呢？
3. 為什麼不能把「觸發前」和「觸發後」的方向放在同一張圖上、靠權重來區分？
