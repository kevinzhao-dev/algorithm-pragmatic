---
title: 兩個維護窗口（有解就給排法、沒解就給證據）
unit: 05 · Graph Search（PS5）
source: 6.006 PS5 Problem 5-3
pdf: materials/6.006/psets/ps5/questions.pdf
answer: materials/6.006/psets/ps5/solutions.pdf
read: materials/6.006/lectures/lec09.pdf, materials/6.006/recitations/rec09.pdf
kind: code
minutes: 80
---

# 0513 · 兩個維護窗口

> **實務上為什麼會遇到**：共用主機的服務不能同批重啟、會互相鎖死的 migration 不能同時跑。排不下的時候，老闆要的不是「不行」兩個字，而是「這幾個互相衝突，所以不行」。

你要在兩個維護窗口內重啟所有服務。輸入是一串衝突：

```cpp
using Conflict = std::pair<std::string, std::string>;
```

`{a, b}` 代表 `a` 和 `b` 不能在同一個窗口重啟。服務名稱是任意字串（可以是空字串、可以很長），**只出現在衝突列表裡的服務才算數**，事先不知道有幾個。同一對可以重複出現或反過來出現；`{x, x}` 代表 `x` 跟自己衝突，永遠排不下。

`n` 是衝突筆數，服務名稱的長度視為常數。一次只看一關，做完再往下讀。

## Stage 1 — 排得下嗎

`can_fit_two_windows(conflicts)`：所有服務能不能分到兩個窗口，且任一衝突的兩端都不在同一窗口。

期望 O(n)。

## Stage 2 — 交出排法或證據

`plan_two_windows(conflicts)` 回傳 `std::variant<Partition, OddCycle>`：

- `Partition{first, second}`：排得下時，每個服務恰好出現在其中一個裡面，同一窗口裡沒有衝突。
- `OddCycle{members}`：排不下時，`members` 是**不重複**的服務名稱，長度為奇數，相鄰兩個（含最後接回第一個）都有衝突。`{x, x}` 的情況回傳只有 `x` 的環。

期望 O(n)。

## Stage 3 — 哪一筆壓垮的

衝突是按時間順序一筆一筆發現的。`first_breaking_conflict(conflicts)`：回傳最小的索引 `i`，使得前 `i + 1` 筆衝突已經排不下；全部都排得下回傳 `std::nullopt`。

一筆一筆重做是 O(n²)，你要做到 **O(n log n)** 或更好。

## 面試追問

1. 為什麼「沒有奇環」等價於「排得下」？你的演算法哪一步用到了這件事？
2. 如果 conflict 是持續串流進來，每來一筆都要立刻回答「還排得下嗎」，你會用什麼資料結構？
3. 想讓兩個窗口的服務數量盡量接近，這題會變成什麼問題？複雜度差多少？
