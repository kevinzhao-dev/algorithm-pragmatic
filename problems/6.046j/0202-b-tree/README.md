---
title: B-Tree（插入、刪除、範圍掃描）
unit: 02 · FFT 與 B-Trees（PS2）
source: 6.046J Recitation 2
pdf: materials/6.046j/recitations/rec02.pdf
read: materials/6.046j/recitations/rec02.pdf
kind: code
minutes: 180
---

# 0202 · B-Tree

> **實務上為什麼會遇到**：資料庫索引和檔案系統都用 B-tree，因為一個節點剛好放滿一個磁碟頁，樹高就是磁碟讀取次數。即使在記憶體裡，`std::set` 的指標追逐也比不上一整塊連續的鍵。

實作以 minimum degree `t`（`t ≥ 2`）參數化的 B-tree，`Key` 只保證可以複製、移動和 `operator<`。

## 不變量

測試會直接走訪公開的 `Node` 結構並檢查：

- 非根節點的鍵數在 `[t-1, 2t-1]`；根有 1 到 `2t-1` 個鍵；空樹的 `root()` 是 `nullptr`。
- 內部節點有 `keys.size() + 1` 個子節點，葉節點沒有子節點（`children` 為空）。
- 所有葉節點深度相同，樹高不超過 `⌊log_t((n+1)/2)⌋`。
- `keys` 內遞增，整棵樹的中序走訪嚴格遞增。

## Stage 1 — 搜尋與插入

| 操作 | 行為 |
| --- | --- |
| `BTree(t)` | `t < 2` 時丟 `std::invalid_argument` |
| `contains(key)` | `O(t log_t n)` |
| `insert(key)` | 新增回傳 `true`；已存在回傳 `false` 且不動樹 |
| `size()`、`empty()`、`min_degree()`、`root()` | O(1) |
| move 建構與賦值 | 來源變成合法的空樹（可以繼續使用），`t` 保留 |

插入時建議在往下走的途中，先把滿的節點拆開，就不必再往回走。根被拆開時，樹高加 1。

## Stage 2 — 刪除

`erase(key)`：存在就刪除並回傳 `true`，否則回傳 `false` 且不動樹。

往下走時保證要進入的子節點至少有 `t` 個鍵。鍵在內部節點時要用前驅或後繼換下來。根被合併到沒有鍵時要收掉，樹高減 1；最後一個鍵刪除後 `root()` 回到 `nullptr`。

## Stage 3 — 實務延伸：範圍掃描

- `lower_bound(key)`：回傳不小於 `key` 的最小鍵，沒有時回傳 `std::nullopt`，`O(t log_t n)`。
- `scan(low, high, visit)`：依序對 `[low, high)` 內每個鍵呼叫 `visit(key)`。`k` 是區間內的鍵數，比較次數要是 `O(t log_t n + k)`，也就是 Counted 測試會擋掉「整棵樹掃一遍」的做法。

## 面試追問

1. 用 2-3-4 樹（`t = 2`）當例子，插入時為什麼預先分裂可以避免回溯？代價是什麼？
2. 給定磁碟頁大小，`t` 要怎麼選？為什麼資料庫幾乎都用 B+ tree，而不是這個版本？
3. 寫入密集的系統為什麼轉向 LSM-tree？用寫入放大（write amplification）比較兩者。
