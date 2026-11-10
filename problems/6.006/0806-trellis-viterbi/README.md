---
title: Trellis DP、Viterbi 與棋盤放置（狀態擴充）
unit: 08 · Dynamic Programming II（PS8）
source: 6.006 Lecture 17, Recitation 17
pdf: materials/6.006/lectures/lec17.pdf
read: materials/6.006/lectures/lec17.pdf, materials/6.006/recitations/rec17.pdf
kind: code
minutes: 100
---

# 0806 · Trellis DP、Viterbi 與棋盤放置

> **實務上為什麼會遇到**：HMM 的 Viterbi decode、CRF 的 inference、語音辨識的解碼，把「每個時間點有幾種狀態、相鄰狀態之間有成本」攤開就是 trellis 上的最短路徑。Lecture 17 的 piano fingering 也是同一個形狀。先寫一個通用的 solver，之後的問題只需要把狀態定義對。

**Trellis**：`positions` 個時間點，每個時間點挑一個 `states` 裡的狀態 `s_i`，總成本是

`Σ node(i, s_i) + Σ edge(i, s_i, s_{i+1})`（`i` 從 0 開始，`edge` 是第 `i` 與第 `i+1` 個時間點之間的成本）。

Cost 的型別由 `node` 的回傳值決定（整數或 `double`）。一次只看一關，做完再往下讀。

## Stage 1 — 最小成本

```cpp
template <typename NodeCost, typename EdgeCost>
auto min_trellis_cost(std::size_t positions, std::size_t states, NodeCost&& node, EdgeCost&& edge);
```

`positions == 0` 回傳 `Cost{}`；`states == 0`（且 `positions > 0`）丟 `std::invalid_argument`。每個 `edge(i, s, t)` 最多呼叫一次：總共不超過 `(positions − 1) · states²` 次呼叫（測試會計數，所以不能重算）。

## Stage 2 — 還原狀態序列

`std::vector<std::size_t> best_trellis_path(positions, states, node, edge)`：回傳每個時間點的狀態，總成本要是最小值（同分任選），呼叫次數限制相同。

## Stage 3 — `min_trellis_cost` 降到 O(states) 記憶體

Heap 高水位要與 `positions` 無關。

## Stage 4 — Viterbi

```cpp
struct Hmm {
    std::vector<double> start;                      // start[s] = P(first state is s)
    std::vector<std::vector<double>> transition;    // transition[s][t] = P(t next | s now)
    std::vector<std::vector<double>> emission;      // emission[s][o] = P(observe o | s)
};

std::vector<std::size_t> viterbi_decode(const Hmm& hmm, std::span<const std::size_t> observations);
```

回傳聯合機率最大的狀態序列。空的 `observations` 回傳空 vector。序列很長時機率乘積會 underflow，一定要改在 log 域計算，並且處理好機率為 0 的轉移（用 Stage 1–2 的 solver 應該只要幾行）。

## Stage 5 — 實務延伸：2×n 棋盤放置

`2 × n` 的棋盤，每格有整數價值（可為負）。可以放任意多艘不重疊的 `1×2` 艘（水平或垂直），分數是被覆蓋格子價值的總和，也可以都不放。

```cpp
struct Ship { std::size_t row; std::size_t column; bool vertical; };
struct ShipPlan { std::int64_t score; std::vector<Ship> ships; };

ShipPlan best_ship_plan(std::span<const int> top, std::span<const int> bottom);
```

`top`、`bottom` 長度必須相同（否則丟 `std::invalid_argument`）。`Ship` 覆蓋 `(row, column)` 與 `(row, column + 1)`（水平）或 `(0, column)` 與 `(1, column)`（垂直，此時 `row == 0`）。

把它化成 Stage 1–2 的 trellis：`positions = n + 1`，第 `x` 個時間點的狀態是「第 `x` 欄有哪些格子已經被從左邊來的水平艘占據」（0 到 3 的 bitmask）。O(n) 時間（狀態數是常數）。

## 面試追問

1. Trellis 上的問題和「在 layered DAG 上找最短路徑」是同一件事嗎？這跟你在前面單元學的 DAG relaxation 有什麼關係？
2. `edge` 的呼叫次數為什麼跟 `states²` 成正比？如果轉移只在相鄰狀態之間（例如手指只移動一格），可以省掉什麼？
3. Stage 5 的狀態為什麼只需要一欄？如果棋盤是 `10 × n`，broken profile DP 怎麼延伸這個想法？
