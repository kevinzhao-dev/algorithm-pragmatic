---
title: 配額分派（最少退單與增量重排）
unit: 07 · Network Flow（PS7）
source: 6.046J PS7 Problem 7-3
pdf: materials/6.046j/psets/ps07/questions.pdf
answer: materials/6.046j/psets/ps07/solutions.pdf
read: materials/6.046j/lectures/lec14a.pdf
kind: code
minutes: 110
---

# 0713 · 配額分派

> **實務上為什麼會遇到**：GPU 型號配額分給訓練任務、廣告庫存分給投放需求、機位分給旅客。每個 request 能接受若干種資源，每種資源有配額，目標是拒絕最少的 request。這題同時做離線批次與增量重排。

`accepts[i]` 是 request i 可接受的資源種類（`0..m-1`，可能為空），`quota[j]` 是資源 j 的數量。一個 request 至多拿一份資源。

一次只看一關。

## Stage 1 — 離線最佳分派

```cpp
struct Plan {
    std::vector<int> choice;   // choice[i] = 分到的資源，或 -1
    int rejected = 0;          // 沒拿到的 request 數，必須最小
};
Plan best_plan(std::span<const std::vector<int>> accepts, std::span<const int> quota);
```

每個 request 只能拿自己接受的資源、每種資源分出去的份數不超過 `quota[j]`。O(E·f) 即可。

## Stage 2 — 短缺說明

當一定有人被拒絕時，營運想知道為什麼。

`Shortage shortage_report(accepts, quota)`，`Shortage{requests, resources}`：
- 回傳一組 request 子集 `requests`（升冪）、資源子集 `resources`（升冪），滿足：`requests` 中每個 request 接受的資源都在 `resources` 內，且 `Σ quota(resources) < requests.size()`。
- `best_plan` 的 `rejected == 0` 時回傳兩個空 vector。

## Stage 3 — 實務延伸：增量配置器

```cpp
class Allocator {
public:
    explicit Allocator(std::vector<int> quota);
    // 加入一個 request，回傳 request id（從 0 遞增）。內部要重排別人的分派，
    // 讓目前的 request 全體仍然拒絕最少。每次 O(E)。
    int add_request(std::vector<int> accepts);
    void cancel(int id);                 // 取消 request（已取消的 id 不再有效）
    int choice_of(int id) const;         // 目前分到的資源，或 -1
    int rejected() const;                // 目前未取消的 request 中被拒絕的數量
};
```

`cancel` 釋放的資源要立刻讓被拒絕的 request 有機會補上；操作後仍要是最佳分派。

## 面試追問

1. 為什麼這個問題等於 bipartite matching，而不必真的把資源複製成 quota 個節點？
2. Stage 3 新增 request 時，為什麼只需要一條 augmenting path？取消 request 時又為什麼只需要一條？
3. 如果 request 有優先序（寧可拒絕低優先的），演算法還成立嗎？（min-cost flow / matroid）
