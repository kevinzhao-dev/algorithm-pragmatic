---
title: 環狀 Deque 與滑動視窗限流器
unit: 01 · Sequences（PS1）
source: 6.006 Lecture 2, Recitation 2
pdf: materials/6.006/recitations/rec02.pdf
read: materials/6.006/lectures/lec02.pdf
kind: code
minutes: 70
---

# 0104 · 環狀 Deque

> **實務上為什麼會遇到**：工作佇列、網路封包緩衝、滑動視窗統計都要兩端 O(1) 進出。環狀緩衝區（ring buffer）是最常見的做法，比鏈結串列少了每個元素一次 allocation，也更 cache-friendly。

`RingDeque<T>`：用一段循環使用的陣列實作 deque。`T` 可以 default-construct、move（這題重點是環狀索引，raw storage 在 0103 練過，底層可以用 `std::vector<T>`）。

## Stage 1 — 環狀索引

| 操作 | 行為 | 複雜度 |
| --- | --- | --- |
| `RingDeque()` | 空的 | O(1) |
| `insert_first(v)` / `insert_last(v)` | 從前端／後端插入 | 攤提 O(1) |
| `delete_first()` / `delete_last()` | 從前端／後端刪除並回傳；空的時丟 `std::out_of_range` | 攤提 O(1) |
| `get_at(i)` / `set_at(i, v)` | 第 `i` 個（從前端算起）；超出範圍丟 `std::out_of_range` | O(1) |
| `size()`、`empty()`、`capacity()` | | O(1) |

邊界：前端與後端在陣列尾巴繞回時的索引、空與滿的狀態判斷、從 capacity 0 開始。

## Stage 2 — 攤提契約

和 0103 一樣，`capacity()` 不綁定特定值，但必須滿足：

1. 永遠 `size() ≤ capacity() ≤ 4·size() + 16`。
2. 不管在哪一端、用什麼順序操作，**總共 m 次操作的元素搬動（copy + move）≤ 9m + 常數**。
3. 不可 thrash。

提示：擴容時把元素依邏輯順序重排，`head` 從 0 開始；不要做逐一搬動的 O(n²) 操作。

## Stage 3 — 實務延伸：滑動視窗限流器

`SlidingWindowLimiter`：API 閘道常用的「最近 `window` 個時間單位內，最多放行 `limit` 個請求」（例如每 60 秒最多 100 次）。

- `SlidingWindowLimiter(limit, window)`：`limit ≥ 1`，`window ≥ 1`，否則丟 `std::invalid_argument`。
- `allow(t)`：時間 `t` 來了一個請求，回傳是否放行。放行條件是時間區間 `(t − window, t]` 內已放行的請求數 `< limit`；放行時要記錄這個請求。**被拒絕的請求不計入。**
- `active(t)`：回傳區間 `(t − window, t]` 內已放行的請求數。
- `t` 必須非遞減，違反時丟 `std::invalid_argument`。

用 Stage 1 的 deque 存放行時間，兩個函式都要**攤提 O(1)**（時間複雜度與 `limit` 無關）。

## 面試追問

1. 環狀緩衝區為什麼常把容量做成 2 的冪次？怎麼用位元運算取代 `%`，對負數索引有什麼好處？
2. 固定容量的環狀佇列，「滿」和「空」都會讓 `head == tail`，常見的解法有哪些？各有什麼代價？
3. 跟 token bucket 或 fixed window 比，sliding window log 的優缺點是什麼？
