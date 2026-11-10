---
title: 判斷樹與 Leaf Peeling（歸納法變演算法）
unit: 00 · Warm-up（PS0）
source: 6.006 PS0 Problem 0-5
pdf: materials/6.006/psets/ps0/questions.pdf
answer: materials/6.006/psets/ps0/solutions.pdf
kind: code
minutes: 60
---

# 0015 · 判斷樹與 Leaf Peeling

> **實務上為什麼會遇到**：網路拓樸、組織架構、設定檔的父子關係常要求「一定是樹」。如果在交換器之間多接一條線，broadcast 封包會繞著 loop 打轉（broadcast storm），網管系統就要找出那條 loop。

作業原題要你用歸納法證明「連通且 `|E| = |V| − 1` 的無向圖沒有環」。證明的核心是：這種圖一定有 degree 為 1 的頂點，拔掉它後規模少 1。這題把這個歸納過程直接寫成演算法，證明本身放在面試追問。

圖用 `n`（頂點編號 `0…n−1`，`n ≥ 1`）和無向邊的清單表示，可能有 self-loop（`u == v`）和重複的邊（parallel edges，兩條都算）。所有函式都必須是 **O(V + E)**，而且不可遞迴（一條長度一百萬的路徑圖，遞迴會把 stack 撐爆）。

## Stage 1 — 三個判斷

- `is_connected(n, edges)`：是否連通。
- `is_acyclic(n, edges)`：是否沒有環。self-loop 和 parallel edges 都算環。
- `is_tree(n, edges)`：連通且無環。

測試會枚舉所有小圖，順便驗證原題的定理。

## Stage 2 — 把證明寫成一段 trace

`peel_order(n, edges)`：若圖是一棵樹，回傳 `n − 1` 筆 `Peel{leaf, neighbor}`，描述歸納證明的過程：

> 每一步 `leaf` 在**目前剩下**的圖中 degree 恰為 1，它唯一的鄰居是 `neighbor`；把 `leaf` 與這條邊拿掉。全部做完只剩一個頂點。

若不是樹，回傳 `std::nullopt`。`n = 1` 且無邊時，回傳空的清單（單一頂點本來就是樹）。測試會逐筆驗證這段 trace，順序不限定。

## Stage 3 — 實務延伸：找出那條 loop

給定一張**連通且恰有 n 條邊**的圖（也就是樹多一條邊，恰好一個環）。`find_cycle(n, edges)` 回傳環上的頂點，依環的順序排列，起點與方向不限。

- self-loop 時回傳單一頂點；一對 parallel edges 時回傳那兩個端點。
- 提示：Stage 2 的 peeling 在這張圖上做到底，**剩下的就只有環**。

## 面試追問

1. 口頭完成原題的歸納證明：為什麼一定存在 degree 1 的頂點？（提示：平均 degree）拔掉後為什麼還是連通且 `|E| = |V| − 1`？
2. 同樣的題目，什麼時候你會改用 union-find？它能做到 peeling 做不到的什麼？
3. Stage 1 的「邊數 + 連通分量數 == 頂點數」判斷法，為什麼在有 parallel edges 時仍然正確？
