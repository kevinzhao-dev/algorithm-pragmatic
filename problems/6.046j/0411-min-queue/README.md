---
title: Min Queue（攤銷 O(1) 取最小值的 FIFO）
unit: 04 · Amortization 與 Randomization（PS4）
source: 6.046J PS4 Problem 4-1
pdf: materials/6.046j/psets/ps04/questions.pdf
answer: materials/6.046j/psets/ps04/solutions.pdf
read: materials/6.046j/lectures/lec05.pdf
kind: code
minutes: 75
---

# 0411 · Min Queue

> **實務上為什麼會遇到**：要回報「過去一分鐘的最高延遲」時，每次掃整個視窗太貴，heap 的刪除也麻煩。這個資料結構的攤銷論證短到可以在白板上講完，正是面試愛考的類型。

做一個 FIFO 佇列，額外支援查詢目前元素的最小值，每個操作**攤銷 O(1)**。

## Stage 1 — 基本操作

`MinQueue<T, Compare>`，預設 `Compare = std::less<T>`：

| 操作 | 行為 |
| --- | --- |
| `enqueue(value)` | 放到佇列尾端 |
| `dequeue()` | 取出並回傳佇列開頭 |
| `front()` | 回傳佇列開頭 |
| `find_min()` | 回傳目前的最小值（依 `Compare` 判斷） |
| `size()`、`empty()` | 目前元素數 |

佇列為空時 `dequeue`、`front`、`find_min` 丟 `std::out_of_range`。

**原題假設元素互不相同，這裡不假設**。最容易寫錯的是刪除時用值來判斷要不要從輔助結構移除：兩個相同的值不是同一個元素。

## Stage 2 — 攤銷成本與泛型

- 任意 `m` 個操作，總比較次數不超過 `3m`。測試用 `Counted` 量，其中包含一次把整段都淘汰掉的 worst case。
- `Compare` 可以有狀態（lambda 捕捉外部變數）。
- `T` 可以是 **move-only**（例如內含 `unique_ptr`）：輔助結構不能保存 `T` 的副本。

## Stage 3 — 實務延伸：視窗內的高峰值

`PeakTracker(window)`：`window ≥ 1`，否則丟 `std::invalid_argument`。內部用 `MinQueue<int64, std::greater<>>`。

- `record(timestamp, value)`：記錄一筆資料。
- `peak(now)`：回傳時間落在 `(now - window, now]` 內的最大值，沒有資料時回傳 `std::nullopt`。

時間只能往前走，`record` 和 `peak` 的時間參數合起來必須非遞減，否則丟 `std::invalid_argument`。同一時間可以有多筆資料。

## 面試追問

1. 用 potential method 證明攤銷 O(1)：`Φ` 怎麼定？單一 `enqueue` 的 worst case 是多少？
2. 不變量是什麼？為什麼輔助結構一定是遞增的？（原題要求的正確性論證，在這裡用口頭講清楚。）
3. 如果要求**最壞情況** O(1)，或要支援非交換的聚合（例如字串串接），兩個 stack 的做法要怎麼改？
