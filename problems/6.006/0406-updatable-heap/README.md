---
title: 可更新優先權的 Heap（handle）
unit: 04 · Trees & Heaps（PS4）
source: 6.006 Lecture 8, Recitation 8
pdf: materials/6.006/recitations/rec08.pdf
read: materials/6.006/lectures/lec08.pdf
kind: code
minutes: 75
---

# 0406 · 可更新優先權的 Heap

> **實務上為什麼會遇到**：Dijkstra 的 decrease-key、作業系統排程器調整優先權、限流器重新計算下一次放行時間：這些場景都需要「手上握著 handle，O(log n) 改掉某個元素的優先權」。`std::priority_queue` 做不到，它不知道元素在陣列裡的位置。

`UpdatableHeap<T, Compare>` 是最大堆（依 `Compare`）。跟 0405 不同的是：`push` 回傳一個穩定的 `Handle`，之後可以隨時用它查詢或修改該元素，而元素在陣列中的位置會一直變。

核心做法：**heap 陣列和 handle 表要互相同步**。handle 記著自己對應到 slot，slot 記著它在 heap 陣列裡的位置；每次 swap 都要同時更新這兩邊。

`Handle` 是可比較、可預設建構的值型別，內容由你設計。

## Stage 1 — push / pop 與 handle

- `push(v)`：回傳 handle，O(log n)。
- `top()` / `top_handle()`：O(1)；空堆時丟 `std::out_of_range`。
- `pop()`：移除並回傳堆頂（空堆丟 `std::out_of_range`）。
- `contains(h)`：這個 handle 目前是否有效。**預設建構的 handle 永遠無效。**
- `get(h)`：取值；無效的 handle 丟 `std::invalid_argument`。
- `size()` / `empty()`。

## Stage 2 — 改優先權

`update(h, v)`：把 `h` 的值改成 `v`，O(log n)，可能往上也可能往下；`h` 無效時丟 `std::invalid_argument`。

## Stage 3 — 任意刪除與失效偵測

- `erase(h)`：刪除並回傳該元素，O(log n)，`h` 無效時丟 `std::invalid_argument`。
- `pop` 或 `erase` 之後，舊的 handle 必須永遠保持無效，**即使內部的 slot 被新元素重用**（想一下 generation counter）。
- `build(items)`：丟掉現有內容，用 `items` 重建，O(n)（比較次數不超過 `3n`），回傳每個 item 的 handle（順序與 `items` 相同）。

## Stage 4 — 實務延伸：截止時間排程器

`DeadlineScheduler` 管理有名稱的工作，任務名稱唯一：

- `submit(name, deadline)`：新工作回傳 `true`；名稱已存在則回傳 `false`、不做事。
- `reschedule(name, deadline)`：改截止時間（保留原本的提交順序），找不到回傳 `false`。
- `cancel(name)`：移除工作，找不到回傳 `false`。
- `next()`：取出最早該執行的工作名稱；沒有工作時 `std::nullopt`。同截止時間時，**先 submit 的先做**。
- `peek()`：查看但不取出。`size()`。

每個操作 O(log n)（期望，因為要用 hash map 從名稱找 handle）。

## 面試追問

1. Dijkstra 用 lazy deletion（重複 push，pop 時跳過過期項）和 decrease-key，各有什麼取捨？
2. handle 為什麼不能直接存陣列索引？generation counter 能防止什麼 bug？
3. 你的 slot 表會無限增長嗎？`erase` 之後的 slot 怎麼處理？
