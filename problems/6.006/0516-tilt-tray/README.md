---
title: 傾斜托盤（狀態空間搜尋）
unit: 05 · Graph Search（PS5）
source: 6.006 PS5 Problem 5-6
pdf: materials/6.006/psets/ps5/questions.pdf
answer: materials/6.006/psets/ps5/solutions.pdf
read: materials/6.006/lectures/lec09.pdf, materials/6.006/recitations/rec09.pdf
kind: code
minutes: 120
---

# 0516 · 傾斜托盤

> **實務上為什麼會遇到**：model checker、測試案例產生器、規劃問題都長這樣：把系統的每個狀態當成頂點、每個動作當成邊，圖大到不可能先建好，只能邊走邊生成。

自動倉儲的托盤是 `n × n` 的格子：

```cpp
enum class Tilt { Up, Down, Left, Right };
using Board = std::vector<std::string>;  // n rows of n chars: '#' pillar, 'o' puck, '.' empty
```

一次傾斜會讓**所有**小圓盤往同一個方向滑到底，直到撞到邊界、柱子，或另一個已經停下的圓盤。柱子不會動。輸入保證是合法的 `n × n` 盤面。一次只看一關，做完再往下讀。

## Stage 1 — 傾斜一次

`apply_tilt(board, direction)` 回傳傾斜後的新盤面，O(n²)。（n 會大到一次移動一格的模擬來不及。）

## Stage 2 — 最少幾步

`solve_board(start, row, col)`：回傳把**任一個**圓盤滑到格子 `(row, col)` 的最短傾斜序列（`start` 那格已經有圓盤就回傳空的序列）；做不到回傳 `std::nullopt`。`(row, col)` 不在盤面上時丟 `std::out_of_range`。

最短序列不只一個時，回傳任一個都算對。

如果最短解需要 `k` 步，一次移動的成本是 `O(n²)`，你的時間要是 **O(n² · min(4^k, C))**，其中 `C` 是可達盤面數。也就是說：解很淺的時候不可以把整個狀態空間算出來，解很深或無解的時候要在有限時間內停止。

## Stage 3 — 分析狀態空間

- `reachable_configurations(start)`：從 `start` 開始，傾斜任意多次可以到達的盤面有幾種（含 `start`）。
- `count_predecessors(board, direction)`：有幾種盤面（柱子相同、圓盤數相同）傾斜 `direction` 後**恰好**變成 `board`？`board` 本身如果傾斜後會改變，答案是 0。回傳 `std::uint64_t`，測試保證答案放得下。

## 面試追問

1. 為什麼可達盤面數一定不超過 `C(n² − b, s)`（`b` 是柱子數、`s` 是圓盤數）？實際上差多少？
2. 一個盤面最多 4 個後繼，卻可能有 Ω(nˢ) 個前驅。這對「從目標盤面反向搜尋」代表什麼？
3. visited set 每筆要存什麼？如果盤面一大，你會怎麼壓縮 state、用什麼當 hash key？
