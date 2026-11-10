---
title: 故障域分析（誰的備援最划算、一條線斷掉會怎樣）
unit: 05 · Graph Search（PS5）
source: 6.006 PS5 Problem 5-2
pdf: materials/6.006/psets/ps5/questions.pdf
answer: materials/6.006/psets/ps5/solutions.pdf
read: materials/6.006/lectures/lec10.pdf
kind: code
minutes: 80
---

# 0512 · 故障域分析

> **實務上為什麼會遇到**：機房電力、網路拓撲、微服務相依都有「單點故障」的問題。上線前要回答兩件事：錢該花在哪一個上游？哪一條線斷了會拖垮最多下游？

機房裡有 `feeds` 個電力饋線和 `racks` 個機櫃，用雙向線路相連。頂點編號：饋線是 `0 … feeds-1`，機櫃是 `feeds … feeds+racks-1`。

```cpp
struct Topology {
    int feeds;
    int racks;
    std::vector<std::pair<int, int>> links;  // undirected; order arbitrary, may repeat
};
```

保證：`feeds >= 1`；線路兩端不同；每個機櫃**恰好**連得到一個饋線（直接相連或經過其他機櫃）。圖可能有上百萬個頂點，機櫃可以串成很長的一條鏈，**不可以遞迴**。一次只看一關，做完再往下讀。

## Stage 1 — 備援裝在哪

有一筆預算，可以在一個饋線上加裝備援電源。

`choose_backup_feed(t)` 回傳 `BackupChoice{feed, racks_covered}`：哪個饋線底下的機櫃最多、有幾個。同樣多時取編號最小的饋線；沒有任何機櫃時也要回傳（`racks_covered == 0`）。

要 O(V + E)。

## Stage 2 — 這幾條線斷了

`racks_without_power(t, cut_links)`：`cut_links` 是 `t.links` 的索引（可重複），這些線路都斷掉後，回傳再也連不到任何饋線的機櫃編號，由小到大。

注意索引指的是「那一筆」線路：如果同一對頂點有兩條線路，只切一條不會斷電。

要 O(V + E + |cut_links|)。

## Stage 3 — 單點故障盤點

`blast_radius(t)` 回傳長度為 `t.links.size()` 的 vector：第 `i` 個值是**只有第 `i` 條線路斷掉**時會斷電的機櫃數。

每條線路都切一次重算要 O(E · (V + E))，這裡的 E 可以到幾百萬。你要做到 **O(V + E)**。

## 面試追問

1. 這些線路本來就保證「每個機櫃恰好一個饋線」。如果不保證，Stage 1 和 Stage 3 的答案會怎麼變？
2. Stage 3 什麼時候某條線路的答案是 0？用 DFS tree 的語言解釋。
3. 為什麼處理平行線路時要用「邊的編號」而不是「父節點」來跳過來時的路？
