---
title: MST 變形（Borůvka、D&C 反例、Cycle-Breaking）
unit: 06 · Augmentation、APSP 與 MST（PS6）
source: 6.046J PS6 Problem 6-2
pdf: materials/6.046j/psets/ps06/questions.pdf
answer: materials/6.046j/psets/ps06/solutions.pdf
read: materials/6.046j/lectures/lec12.pdf
kind: code
minutes: 130
---

# 0612 · MST 變形

> **實務上為什麼會遇到**：Borůvka 是分散式與 GPU MST 的基礎，因為每一輪每個元件各選一條最輕邊，天生可以平行；cycle property 則是「增量維護骨幹」的核心。把這些講義外的 MST 變形都跑一次，你就能分清楚哪些「看起來對」的做法其實錯了。

圖都是 `Edge{u, v, w}`，頂點 `0..n-1`，輸入保證連通（Stage 5 除外）。回傳邊的索引，升冪排序。除了 Stage 1 與 Stage 5，**權重互不相同**。

一次只看一關。

## Stage 1 — MST 唯一性（Part a）

`bool has_unique_mst(int n, std::span<const Edge> edges)`：權重**可以重複**，判斷這張連通圖的 MST 是否唯一。O(E log E)。

## Stage 2 — 批次加邊（Part b，Borůvka）

`BoruvkaResult boruvka(int n, std::span<const Edge> edges)`，`BoruvkaResult{edges, phases}`：每一 phase 對每個目前的元件找出穿過切割的最輕邊，全部一次加入，直到變成一棵樹。`phases` 回傳總 phase 數，必須 ≤ ⌈lg n⌉。O(E log V)。

## Stage 3 — Divide-and-conquer 為什麼錯（Part c）

```cpp
std::vector<int> divide_and_conquer_mst(int n, std::span<const Edge> edges);
```

按題目描述實作：範圍 `[lo, hi)` 的頂點以編號切成 `[lo, mid)` 與 `[mid, hi)`，`mid = lo + (hi - lo) / 2`，子圖只保留兩端都在該半的邊，遞迴求 MST，再加上跨越兩半的最輕邊。遞迴到單一頂點為止。

```cpp
struct DcTrap { int n; std::vector<Edge> edges; };
DcTrap divide_and_conquer_trap();
```

回傳一組權重互不相同的連通圖，使得（照上面的切法）左右兩半各自連通，`divide_and_conquer_mst` 的結果卻不是真正的 MST。

## Stage 4 — Cycle-Breaking（Part d）

`std::vector<int> cycle_breaking_mst(int n, std::span<const Edge> edges)`：只要圖還有環，就找一個環並刪掉上面最重的邊。回傳留下來的邊。O(E²) 可接受。

## Stage 5 — 實務延伸：增量維護骨幹

```cpp
class IncrementalMst {
public:
    explicit IncrementalMst(int n);
    // 第 k 次呼叫的邊 id 為 k（從 0 起算）。回傳因此被踢出 forest 的邊 id：
    // 可能是舊邊，也可能是新邊本身；若這條新邊只是連起兩個元件，回傳 nullopt。
    std::optional<int> add_edge(int u, int v, long long w);
    long long weight() const;            // 目前 forest 的總權重
    std::vector<int> edges() const;      // 目前 forest 的邊 id，升冪
};
```

每次 `add_edge` O(V)。權重互不相同。圖可以不連通，此時維護的是 forest。

## 面試追問

1. 用 exchange argument 證明權重互異時 MST 唯一（Stage 1 的前提）。權重相同時，Stage 1 怎麼判斷的？
2. Borůvka 為什麼每一輪元件數至少減半？每輪可以平行到什麼程度？瓶頸在哪？
3. Divide-and-conquer 錯在哪一步？為什麼跨切割只加一條邊不夠，而 cut property 又沒有被違反？
