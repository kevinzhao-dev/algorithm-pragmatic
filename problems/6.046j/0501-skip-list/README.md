---
title: Skip List
unit: 05 · Skip Lists、Hashing 與 DP（PS5）
source: 6.046J Lecture 7
pdf: materials/6.046j/lectures/lec07.pdf
read: materials/6.046j/lectures/lec07.pdf
kind: code
minutes: 150
---

# 0501 · Skip List

> **實務上為什麼會遇到**：Redis 的 sorted set、LevelDB 的 memtable 都是 skip list。寫起來比平衡樹簡單，也更容易做到細粒度並行，代價只是平衡性靠機率保證。

實作一個以集合語意儲存 `Key` 的 skip list，`Key` 只保證可以移動和 `operator<`（`insert` 收到的是值，可以直接移動）。

`Node` 是公開結構：`next[i]` 是這個節點在第 `i` 層的下一個節點，塔高為 `next.size()`。`height()` 是目前用到的層數（空串列是 `0`），`first_at(i)` 是第 `i` 層的第一個節點，`first()` 是 `first_at(0)`。測試會走訪 `Node` 檢查每層都排序、每個第 `i+1` 層的節點也在第 `i` 層。

建構子拿 `seed` 初始化內部亂數，同一個 `seed` 要得到同樣的結構（不要用時間或 `random_device`）。擲硬幣決定塔高，機率 `1/2`。

## Stage 1 — 搜尋與插入

| 操作 | 行為 |
| --- | --- |
| `insert(key)` | 回傳是否新增；重複時不做事 |
| `find(key)` | 回傳節點指標，不存在時回傳 `nullptr` |
| `contains(key)`、`size()`、`empty()` | |
| 解構子 | 釋放所有節點，不可用遞迴；三十萬個節點不能爆 stack |
| move 建構與賦值 | 來源變成合法的空串列 |

搜尋成本要是 **期望 O(log n)** 次比較，測試用 `Counted` 量。

## Stage 2 — 刪除

`erase(key)`：存在就刪除並回傳 `true`。節點要從每一層移除，最上面空掉的層要收掉，`height()` 跟著降低，全部刪完回到 `0`。

## Stage 3 — 實務延伸：延遲任務排程器

`JobScheduler(seed)`：用 skip list 維護未到期的任務。

- `schedule(run_at)`：回傳任務編號（`0, 1, 2, ...`）。
- `cancel(job)`：取消尚未執行的任務，回傳是否真的取消了（不存在或已執行回傳 `false`）。
- `pop_due(now)`：取出所有 `run_at ≤ now` 的任務，依 `run_at` 由小到大，同時間依排程順序。
- `pending()`：還沒到期也沒被取消的任務數。

每個操作的期望成本是 O(log n)。

## 面試追問

1. 為什麼「擲硬幣決定高度」能讓搜尋路徑期望是 O(log n)？用往回走的分析（backward analysis）講一遍。
2. 這個結構和平衡 BST 比，並行更新為什麼比較容易？什麼情況下還是會輸？
3. 把機率從 `1/2` 改成 `1/4` 會有什麼影響？Redis 為什麼選 `1/4`？
