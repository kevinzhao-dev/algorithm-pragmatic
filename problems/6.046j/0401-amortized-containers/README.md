---
title: Table Doubling 與 Binary Counter（攤銷分析）
unit: 04 · Amortization 與 Randomization（PS4）
source: 6.046J Lecture 5
pdf: materials/6.046j/lectures/lec05.pdf
read: materials/6.046j/lectures/lec05.pdf
kind: code
minutes: 120
---

# 0401 · Table Doubling 與 Binary Counter

> **實務上為什麼會遇到**：`std::vector` 和 LSM-tree 的 compaction 背後都是攤銷分析。單次操作可以很貴，但只要設計得好，任何一段操作序列的總成本都是線性的；設計不好，就會在邊界上反覆觸發重配置。

## Stage 1 — 動態陣列

`DynamicArray<T>`：自己管理未初始化的原始記憶體，不可以用 `std::vector`。`T` 可能沒有 default constructor，所以不能先建好一堆元素再賦值。

- `push_back(value)`：滿了就把 capacity 加倍（`0 → 1 → 2 → 4 ...`）。
- `operator[]`（不檢查範圍）、`size()`、`capacity()`、`empty()`。
- 複製建構深層複製；移動建構把記憶體交出去，來源變成 capacity 0 的空陣列。`operator=` 用傳值實作（copy-and-swap）。

**例外安全**：擴充時如果 `T` 的 move constructor 可能丟例外，要改用複製（`std::move_if_noexcept`）。複製失敗時，要清掉已建好的新元素、釋放新緩衝區，陣列完全回到原狀（strong guarantee）。複製建構也要處理複製到一半失敗。

測試會用計數的元素類型確認每次 `push_back` 的攤銷搬動次數是常數，用 `Tracked` 抓洩漏和重複解構。

## Stage 2 — 縮容與不抖動

`pop_back()`：陣列為空時丟 `std::out_of_range`。

刪除後若 `size ≤ capacity / 4`，把 capacity 砍半，這條規則讓每次重配置都停在半滿。特例是 `size` 變成 0 時直接釋放全部記憶體，capacity 回到 0。

`pop_back` **永遠不丟例外**：如果縮容時複製失敗，就留著舊的表，元素不受影響。

測試會在 resize 的臨界點反覆 pop/push：如果你用 `size ≤ capacity / 2` 就縮，每個操作都會變成 Θ(n)。

## Stage 3 — 實務延伸：Log-structured Index

`SortedRuns<T>`：把資料切成一組已排序的 run，大小都是 2 的冪次，且每種大小最多一個。

- `insert(v)`：放入大小 1 的新 run，出現兩個同大小的 run 就合併，一路進位，就像遞增一個二進位計數器。
- `run_count()`：非空 run 的數量，一定等於 `size()` 的二進位 1 的個數。
- `contains(v)`：對每個 run 做二分搜，比較次數 O(log² n)。
- `sorted()`：回傳所有元素排序好的結果。允許重複。

**成本要求**：連續 `n` 次 `insert` 的總比較次數要是 `O(n log n)`（測試用 `Counted` 量）。

## 面試追問

1. 用 potential method 分析 table doubling：`Φ` 要怎麼定義？縮容規則為什麼要用 `1/4` 而不是 `1/2`？
2. `SortedRuns` 的 `insert` 就是 binary counter。如果同時支援 decrement，binary counter 的攤銷分析為什麼會壞掉？這對應到 LSM-tree 的哪個真實問題？
3. Move constructor 沒有 `noexcept` 時，`std::vector` 擴充會發生什麼事？為什麼？
