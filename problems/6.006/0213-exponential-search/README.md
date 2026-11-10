---
title: Exponential Search 與 Finger Cursor
unit: 02 · Sorting & Recurrences（PS2）
source: 6.006 PS2 Problem 2-3
pdf: materials/6.006/psets/ps2/questions.pdf
answer: materials/6.006/psets/ps2/solutions.pdf
read: materials/6.006/lectures/lec03.pdf
kind: code
minutes: 70
---

# 0213 · Exponential Search

> **實務上為什麼會遇到**：要在幾 TB 的已排序日誌裡找某個時間點，最近的資料最常被查；`git bisect` 找「剛剛才壞掉」的 commit 也一樣：目標通常離某一端很近。Exponential search 讓查詢成本取決於「離得多近」，而不是「總共多大」。

原題是沿著一條長 `n` 的線找目標，每次瞬移到一個位置，裝置會告訴你目標在你的前面還是後面；當目標距離最近的那一端 `k` 時，要在 O(log k) 次內找到。這裡把裝置換成一個 oracle 函式，**測試計算探測次數**。

`Direction` 有 `Less`（目標在這個位置之前）、`Here`（就在這裡）、`Greater`（目標在這個位置之後）。Oracle 是可呼叫物件 `Direction oracle(std::int64_t position)`。**不可以探測範圍之外的位置**（測試會檢查）。

一次只看一關，做完再往下讀。

## Stage 1 — Binary Search

`locate_in_range(low, high, oracle)`：目標在半開區間 `[low, high)`（保證存在，`low < high`）。回傳目標位置，探測次數 ≤ `⌈log₂(high − low + 1)⌉ + 1`。

## Stage 2 — 從起點 Exponential Search

`locate_from_start(n, oracle)`：目標在 `[0, n)`，但通常靠近 0。令目標為 `t`，探測次數要是 **O(log(t + 2))**，且與 `n` 無關（`n` 可以到 2⁶⁰）。

提示：從 0 開始，跳 1, 2, 4, 8… 直到越過目標，再對最後一段做 binary search。

## Stage 3 — 雙向 Exponential Search（原題）

`locate_two_sided(n, oracle)`：目標在 `[0, n)`，可能靠近 0 也可能靠近 `n − 1`。令 `k = min(t, n − 1 − t)`，探測次數要是 **O(log(k + 2))**（`n` 也可以到 2⁶⁰）。兩端要交替往內探，不可先走完一邊。

## Stage 4 — 實務延伸：Finger Cursor

在排好序的欄位資料上，查詢常常一個接一個、彼此靠近（例如 merge join、依序讀取 posting list）。

`FingerCursor<T>`：

- `explicit FingerCursor(std::span<const T> sorted)`：游標停在 0。
- `seek(key)`：回傳第一個 `sorted[i] >= key` 的索引（都小於 key 時回傳 `size()`），並把游標移到那裡。
- `position()`：目前游標位置。

若答案距離目前游標 `d` 個位置，比較次數要是 **O(log(d + 2))**：所以要**雙向** gallop，不可以每次從整個陣列重新 binary search。元素可能重複（要回傳第一個 `>= key` 的）。

## 面試追問

1. 為什麼 O(log k) 而不是 O(log n) 這件事在實務上重要？舉一個你真的會用到的例子。
2. 倍增的底數換成 3 會怎樣？為什麼 2 是好的選擇，換成 1（線性掃描）又會怎樣？
3. Finger search 跟 skip list、B-tree 的 cursor 有什麼相似之處？
