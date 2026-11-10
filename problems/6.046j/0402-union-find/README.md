---
title: Union-Find（鏈結串列到 Path Compression）
unit: 04 · Amortization 與 Randomization（PS4）
source: 6.046J Recitation 3
pdf: materials/6.046j/recitations/rec03.pdf
read: materials/6.046j/recitations/rec03.pdf, materials/6.046j/lectures/lec05.pdf
kind: code
minutes: 100
---

# 0402 · Union-Find

> **實務上為什麼會遇到**：帳號合併、Kruskal 最小生成樹、影像連通區塊。需求只有「把兩堆合併」和「問兩個東西是不是同一堆」，但攤銷之後每次操作幾乎是常數，這個結果每個工程師都該親手驗證一次。

元素編號從 `0` 開始。傳入不存在的元素時丟 `std::out_of_range`。

## Stage 1 — 加權鏈結串列

`ListDsu(n)`：`n` 個單元素集合。每個集合用一條鏈結串列表示，每個元素直接存代表元素。

| 操作 | 行為 | 複雜度 |
| --- | --- | --- |
| `find(x)` | 回傳代表元素 | O(1) |
| `unite(x, y)` | 合併；已在同一集合時回傳 `false`，否則回傳 `true` | O(較小集合的大小) |
| `set_size(x)`、`members(x)` | 集合大小、所有成員（順序不限） | O(1)、O(大小) |
| `rep_updates()` | 累積改寫過幾個元素的代表指標 | O(1) |

**一定要把小的併進大的**。測試用 `rep_updates()` 驗證：合併完所有元素時總次數 `≤ n lg n`，否則就退化成 Θ(n²)。

## Stage 2 — 樹狀森林

`ForestDsu`：`union by rank` 加上 `path compression`。

- `ForestDsu(n)`：預先建立 `n` 個元素；`make_set()` 新增一個元素並回傳編號。
- `find(x)`、`unite(x, y)`（語意同 Stage 1）、`connected(x, y)`、`set_size(x)`（O(α)）、`count()`（目前集合數）、`size()`（元素總數）。
- `depth(x)`：不做壓縮，回傳 `x` 到根的邊數。
- `hops()`：累積 `find` 沿 `parent` 走了幾步（`depth` 不算）。

兩個最佳化都要做。少了 union by rank，`depth` 測試會發現樹變高；少了 path compression，`hops` 測試會發現 `find` 走太多步。

## Stage 3 — 實務延伸：帳號合併索引

先幫 `ForestDsu` 加上 `members(x)`，複雜度 **O(集合大小)**，用不到額外的成員 vector：每個元素放一個環狀 `next` 指標，合併時交換兩邊的 `next` 就能把兩條環接起來。

`DedupIndex`：帳號資料合併。

- `add_record(identifiers)`：新增一筆紀錄（可以有多個 email、電話）並回傳編號（`0, 1, 2, ...`）。只要共用任何一個 identifier，就屬於同一個實體，傳遞成立。`identifiers` 可以為空，也可以重複。
- `same_entity(a, b)`、`entity_count()`。
- `entity_members(record)`：回傳該實體所有紀錄的編號，**遞增排序**。

## 面試追問

1. Stage 1 的「小併大」為什麼保證每個元素最多被改寫 `lg n` 次？用 aggregate、accounting、potential 三種方法各說一次。
2. 只做 path compression 或只做 union by rank，攤銷複雜度各是多少？兩個都做時，反阿克曼函數從哪裡來？
3. 這個資料結構支援「拆開」嗎？如果帳號合併要能復原（undo），你會怎麼改？
