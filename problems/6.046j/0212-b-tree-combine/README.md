---
title: Combine B-Trees（O(高度差) 合併）
unit: 02 · FFT 與 B-Trees（PS2）
source: 6.046J PS2 Problem 2-2
pdf: materials/6.046j/psets/ps02/questions.pdf
answer: materials/6.046j/psets/ps02/solutions.pdf
read: materials/6.046j/recitations/rec02.pdf
kind: code
minutes: 150
---

# 0212 · Combine B-Trees

> **實務上為什麼會遇到**：依時間分區的索引，舊分區要合併時，真的把資料重寫一遍太貴。如果能只動樹的邊緣、不碰內部節點，合併的成本就和資料量脫鉤。

把 [`0202`](../0202-b-tree/README.md) 的 `BTree` 複製過來繼續做（`lower_bound` 和 `scan` 這題用不到，可以不留）。`Key` 只保證可以複製、移動和 `operator<`，不變量和 0202 相同，測試會走訪 `Node` 檢查。

## 合併的合約

`BTree::combine(left, key, right)`：

- `left`、`right` 有相同的 `t`，否則丟 `std::invalid_argument`；保證 `left` 的所有鍵 `< key <` `right` 的所有鍵（不需檢查，檢查本身就可能比合併貴）。
- 回傳的新樹包含三者全部的鍵。`left` 和 `right` 都被清空（`size() == 0`、`root() == nullptr`），之後仍可使用。
- 只能移動 **`O(t · (|h1 − h2| + 1))`** 個鍵，`h1`、`h2` 是兩棵樹的高度。測試用一個會計數的 `Key`，把鍵的複製、移動和比較都算成工作量，整棵樹重新插入一遍會直接超標。

## Stage 1 — 高度

`height()`：O(1) 回傳根到葉的邊數，空樹是 `-1`，只有根是 `0`。`insert` 和 `erase` 都要維護。

## Stage 2 — 高度相同

`h1 == h2`：把兩個根和 `key` 併成一個節點。放不下時（鍵數超過 `2t-1`）沿中位數拆開，產生新的根，高度加 1。

`left` 或 `right` 可以是空樹。

## Stage 3 — 高度差 1

`|h1 − h2| == 1`：把較矮那棵的根和 `key` 併進較高那棵的最右（或最左）子節點，可能要連鎖拆分到根。

根的鍵數可以很少，這是 B-tree 唯一允許鍵數少於 `t-1` 的節點，所以併進去的節點有哪些鍵數範圍，要算清楚。

## Stage 4 — 任意高度

`combine` 對任意高度都要正確，包含一棵空樹和一棵高樹。

提示：把較高那棵的**右脊**（或左脊）走到和較矮樹同高的位置，併完之後往上拆。沒有 parent 指標時要自己記錄路徑。兩個方向要處理（左高、右高）。

## Stage 5 — 實務延伸：合併相鄰分區

`BTree::concat(left, right)`：沒有分隔鍵，只保證 `left` 的鍵都小於 `right` 的鍵（任一方可以為空）。取出一個鍵當分隔鍵後，用 `combine` 做到。要移動的鍵數量級只能是 `O(t · log n)`。

## 面試追問

1. 原題要求每個節點都記錄高度，但本題只存在樹上。兩種做法的差別在哪？什麼情況才需要每個節點都記？
2. 為什麼 `combine` 的總成本不是 O(log n)，而是 O(|h1 − h2| + 1)？把 `n` 顆樹依序合併成一棵，總成本會是多少？
3. 反過來的操作 `split(T, k)` 怎麼做？如果 `size()` 也要維持 O(1)，還需要在節點上多存什麼？
