---
title: 雙游標行緩衝區（Binder Bookmarks）
unit: 01 · Sequences（PS1）
source: 6.006 PS1 Problem 1-3
pdf: materials/6.006/psets/ps1/questions.pdf
answer: materials/6.006/psets/ps1/solutions.pdf
read: materials/6.006/lectures/lec02.pdf, materials/6.006/recitations/rec02.pdf
kind: code
minutes: 90
---

# 0113 · 雙游標行緩衝區

> **實務上為什麼會遇到**：文字編輯器的 gap buffer（Emacs）就是把「空隙」放在游標所在處，讓編輯 O(1)；協作編輯、非線性剪輯軟體的時間軸也都有「在兩個標記之間搬動片段」的操作，同時還要能 O(1) 依行號讀取。

原題是用書籤管理活頁夾的頁面。這裡改成編輯器的行緩衝區：`TwoCursorBuffer<T>` 存一串行（item），有兩個游標 A、B 夾在行與行之間。

**位置用 gap 表示**：gap `g`（`0 ≤ g ≤ n`）代表第 `g` 行的「前面」的縫隙，`g == n` 代表最後一行之後。一個游標的「前一行」就是 index `g − 1` 的那一行。規則：**兩個游標都放好之後，A 必須嚴格在 B 之前**（`gap(A) < gap(B)`）。

任何不合法的操作（索引超出範圍、游標還沒放好就操作、會破壞順序的操作）都丟 `std::logic_error`（含子類別），而且內容與游標都不變。

## Stage 1 — 建立與讀取

| 操作 | 行為 | 複雜度 |
| --- | --- | --- |
| `TwoCursorBuffer(span<const T>)` | 依序建立，游標都還沒放 | O(n) |
| `size()` | 行數 | O(1) |
| `read(i)` | 第 `i` 行（索引不受游標影響）；超出範圍丟 `std::out_of_range` | **O(1)** |
| `place_cursor(c, g)` | 把游標 `c` 放到 gap `g`（可重複呼叫移動它），`c` 是 `Cursor::A` 或 `Cursor::B` | O(n) |
| `cursor(c)` | 目前 gap；還沒放過時丟 `std::logic_error` | O(1) |

## Stage 2 — `shift_cursor(c, d)`

把游標移動一格：`d` 只能是 `+1` 或 `−1`，**O(1)**。行本身的順序不變，`read` 的結果也不變。要在兩個游標都放好後才能用；不可讓 A、B 交叉或相撞，也不可超出邊界。

## Stage 3 — `move_item(c)`

把游標 `c` 的**前一行**搬到**另一個游標的前面**，**攤提 O(1)**：

- `move_item(A)`：把 A 前一行搬到 B 前面。A 的前一行消失，所以 `gap(A)` 減 1，`gap(B)` 不變。
- `move_item(B)`：把 B 前一行搬到 A 前面，`gap(A)` 加 1，`gap(B)` 不變。
- 游標前面沒有行、或搬完會讓 `gap(A) == gap(B)` 時（只有 `move_item(B)` 可能發生）都是不合法的操作。

提示：只有 `read` 需要 O(1) 隨機存取，所以可以把序列想成三段（A 前、A 與 B 之間、B 後），每一段放在合適的容器裡。**測試用 `Probe` 計算元素搬動次數，並用 `PERF` 卡暴力解。**

## Stage 4 — 實務延伸：在游標處編輯

- `insert_before_cursor(c, x)`：在游標 `c` 的前面插入一行，`gap(c)` 加 1；若 `c` 是 A，`gap(B)` 也要加 1。
- `delete_before_cursor(c)`：刪除並回傳游標 `c` 的前一行，規則同上（不能讓 `gap(A) == gap(B)`，前面沒有行時不合法）。

這兩個操作也要**攤提 O(1)**。

## 面試追問

1. 為什麼 linked list 在這題不行？`read(i)` 要 O(1) 和 `move_item` 要 O(1) 分別逼你選什麼？
2. 官方解答用一個長度 3n 的陣列留兩段空隙。跟「兩個 stack 加一個 deque」比，兩種設計各有什麼優缺點（常數、快取、重建頻率）？
3. 攤提 O(1) 的 `move_item`：一次重建的成本多大？它在什麼情況下會被觸發，為什麼相鄰兩次重建之間一定有 Ω(n) 次操作？
