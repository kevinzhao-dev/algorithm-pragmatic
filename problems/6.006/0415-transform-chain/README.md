---
title: 矩陣連乘的即時更新（augmented sequence）
unit: 04 · Trees & Heaps（PS4）
source: 6.006 PS4 Problem 4-5
pdf: materials/6.006/psets/ps4/questions.pdf
answer: materials/6.006/psets/ps4/solutions.pdf
read: materials/6.006/lectures/lec07.pdf, materials/6.006/recitations/rec07.pdf
kind: code
minutes: 55
---

# 0415 · 矩陣連乘的即時更新

> **實務上為什麼會遇到**：機械手臂和 scene graph 的正向運動學、GPU 的轉換鏈、WebGL 的 matrix stack，都是「一長串不可交換的轉換相乘，只改其中一個，要立刻拿到整串乘積」。把 O(n) 重算砍到 O(log n)，靠的就是 subtree augmentation。

一條機械手臂由一串 joint 組成，每個 joint 是一個矩陣 `M`，整條手臂的轉換是有順序的乘積 `M_0 · M_1 · … · M_{n-1}`。矩陣型別 `M` 提供 `M::identity()` 和 `operator*`，**結合但不可交換**；`operator*` 當作 O(1)。

`TransformChain<M>` 的介面與複雜度（`n` 是目前 joint 數）：

## Stage 1 — 更新與全段乘積

- `explicit TransformChain(std::vector<M> joints)`：O(n)，`operator*` 呼叫不超過 `3n`。
- `size()`。
- `update_joint(k, M)`：把第 `k` 個 joint 換掉，O(log n) 次 `operator*`；`k` 超出範圍丟 `std::out_of_range`。
- `full_transformation()`：回傳目前的整條乘積，**O(1)，不能呼叫 `operator*`**。沒有 joint 時回傳 `M::identity()`。

可以 `#include "../0404-avl-sequence/solution.hpp"`，把 matrix 當作 sequence item，定義一個 `Aug`（`combine` 就是相乘）；你在 0404 已經把麻煩的部分都做完了。

（這一關 joint 的數量和順序都不變，所以靜態結構就夠用；Stage 3 才需要動態版本。）

## Stage 2 — 區段乘積

`segment(l, r)`：回傳 `M_l · … · M_{r-1}`，O(log n) 次 `operator*`；`l == r` 回傳 `identity`；`l > r` 或 `r > n` 丟 `std::out_of_range`。

`full_transformation` 比 `segment(0, n)` 更便宜，是因為根節點已經存好答案了。

## Stage 3 — 重新組裝手臂

- `insert_joint(k, M)`：在位置 `k` 插入新 joint（`0 ≤ k ≤ n`），O(log n)。
- `remove_joint(k)`：移除並回傳第 `k` 個 joint，O(log n)。

兩者超出範圍時都丟 `std::out_of_range`。這一關結束後，`full_transformation` 和 `segment` 必須仍然正確、仍然只用這些乘法次數。

## 面試追問

1. 乘積不可交換時，`combine(left, right)` 的呼叫順序錯了會發生什麼？你的測試怎麼抓到它？
2. 如果 joint 永遠不會增減，完整二元樹（隱式陣列）跟 AVL 比，有什麼好處？
3. 真實的轉換用 `double` 會累積誤差：O(log n) 次更新跟每次重算相比，數值上誰比較準？
