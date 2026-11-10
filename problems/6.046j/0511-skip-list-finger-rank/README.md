---
title: Skip List 的 Finger Search 與 Rank
unit: 05 · Skip Lists、Hashing 與 DP（PS5）
source: 6.046J PS5 Problem 5-1
pdf: materials/6.046j/psets/ps05/questions.pdf
answer: materials/6.046j/psets/ps05/solutions.pdf
read: materials/6.046j/lectures/lec07.pdf
kind: code
minutes: 210
---

# 0511 · Skip List 的 Finger Search 與 Rank

> **實務上為什麼會遇到**：查詢常常有區域性：翻到下一頁、看隔壁的排名、時間序列往前走幾筆。從頭搜尋要 O(log n)，手上已經握著附近的節點時，成本只該和距離有關。

把 [`0501`](../0501-skip-list/README.md) 的 skip list 擴充成 `IndexedSkipList<Key>`：每一層都是雙向串列，並記錄每條連結跨過多少個第 0 層節點。`Node` 的內部由你決定，只需要公開 `key()`。節點指標在該節點被刪除之前都必須有效。

`m = 1 + |rank(x) − rank(目標)|`。Stage 1 和 Stage 3 的查詢都要 O(lg m)，跟 `n` 無關。

## 成本模型

`steps()`：累積 `finger_search`、`rank`、`at`、`rank_search` 走過的連結數，換層也算一步。`insert`、`erase`、`find` 不用計。

## Stage 1 — 雙向串列與 Finger Search

基本操作：`insert(key)` 回傳新節點（已存在時回傳 `nullptr`）、`erase(key)`、`find(key)`、`first()`、`next(node)`、`prev(node)`、`size()`、`empty()`。走到盡頭回傳 `nullptr`。解構子不可遞迴。

`finger_search(from, key)`：從 `from` 出發找 `key`，**左右兩個方向都要支援**，不存在時回傳 `nullptr`。

做法分兩段：先往上爬，同時往目標方向走，直到再往上就會超過目標；再往目標走並逐層下降。測試用 `Counted` 量比較次數。

## Stage 2 — 用寬度支援排名

每條連結多存一個寬度（這條連結跨過多少個第 0 層節點），`insert` 和 `erase` 都要維持正確，複雜度不變。

- `rank(node)`：節點的 0-based 排名，O(log n)。
- `at(index)`：回傳第 `index` 個節點，超出範圍回傳 `nullptr`，O(log n)。

## Stage 3 — Rank Search

`rank_search(from, delta)`：回傳排名是 `rank(from) + delta` 的節點，`delta` 可正可負，`delta == 0` 回傳 `from`，超出範圍回傳 `nullptr`。**O(lg m)**，`m = |delta| + 1`，所以不能先呼叫 `rank` 再 `at`。

## Stage 4 — 實務延伸：滑動中位數

`SlidingMedian(seed)`：維護一個依到達順序的視窗。

- `push(value)`：加入新值，允許重複。
- `pop()`：移除最舊的值，空的時候丟 `std::out_of_range`。
- `median()`：回傳排名 `(n−1)/2` 的值（偶數取較小的），空的時候回傳 `std::nullopt`。
- `size()`、`steps()`：`steps()` 回傳內部串列累積的步數。

中位數每次最多移動一格，所以要握著目前中位數的節點，用 `rank_search` 移動。每次更新的平均步數要是常數，每次都從頭 `at` 會被測試擋下。

## 面試追問

1. 為什麼 finger search 的上升階段高度以 O(lg m) 為限？和一般 skip list 的 O(lg n) 分析差在哪？（提示：範圍內只有 `m` 個節點。）
2. 寬度怎麼在插入時更新？為什麼刪除時只要走過的路徑就夠了？
3. B-tree 能不能做到一樣的 finger search？如果要並行，skip list 和 B-tree 哪個比較適合？
