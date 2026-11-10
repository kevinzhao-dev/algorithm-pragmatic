---
title: Edit Distance 與拼字建議
unit: 07 · Dynamic Programming I（PS7）
source: 6.006 Recitation 16
pdf: materials/6.006/recitations/rec16.pdf
read: materials/6.006/recitations/rec16.pdf
kind: code
minutes: 100
---

# 0703 · Edit Distance 與拼字建議

> **實務上為什麼會遇到**：搜尋框的 did-you-mean、資料清洗的模糊比對、CI 裡的 snapshot diff 都在問同一件事：要改幾個字元才相等。真實系統只關心「差距有沒有在 k 以內」，答案通常在幾步內就知道了，沒必要算完整張表。

把字串 `from` 改成 `to`，可用 insert、erase、replace 三種操作。一次只看一關，做完再往下讀。

## Stage 1 — 最小成本

```cpp
struct EditCosts { int insert = 1; int erase = 1; int replace = 1; };  // each >= 1
int edit_distance(std::string_view from, std::string_view to, EditCosts costs = {});
```

O(|from||to|) 時間。三種操作的成本可以不同，replace 甚至可能比 insert 加 erase 還貴，要選最便宜的做法。

## Stage 2 — 編輯腳本

```cpp
enum class EditOp { Insert, Erase, Replace };
struct Edit { EditOp op; std::size_t at; char ch; };
std::vector<Edit> edit_script(std::string_view from, std::string_view to, EditCosts costs = {});
```

依序套用腳本，字串要從 `from` 變成 `to`，總成本要等於 Stage 1 的答案。**`at` 是套用這一步當下字串的位置**：`Insert` 在 `at` 之前插入 `ch`，`Erase` 刪掉 `at` 的字元，`Replace` 把 `at` 的字元換成 `ch`（`ch` 在 `Erase` 時可為任意值）。

## Stage 3 — `edit_distance` 降到 O(min(|from|, |to|)) 記憶體

Heap 高水位要在較短字串長度的線性範圍內。

## Stage 4 — 有上限的距離

`std::optional<int> edit_distance_within(from, to, limit)`：單位成本，距離 ≤ `limit` 時回傳距離，否則 `std::nullopt`。

- 時間 O(limit · min(|from|, |to|))，記憶體 O(limit)。只有對角線附近的格子可能在答案上。
- 長度差已經超過 `limit` 時，不用掃就能回答。
- `limit` 可能比字串長很多（例如 10⁹），不可以照著它配置記憶體。

## Stage 5 — 實務延伸：拼字建議

```cpp
struct Suggestion { std::string_view word; int distance; };
std::vector<Suggestion> suggest(std::string_view query,
                                std::span<const std::string> dictionary, int limit);
```

回傳字典裡單位成本距離 ≤ `limit` 的所有字，依距離遞增、同距離依字典序遞增排序。`dictionary` 內的字互不相同，`word` 指向 `dictionary` 裡的字串。

## 面試追問

1. Stage 3 把字串對調以節省記憶體時，insert 和 erase 的成本為什麼要跟著對調？
2. Stage 4 為什麼只看 `|j - i| ≤ limit` 就夠？列出它不適用的成本設定。
3. 字典有上億個字時逐一比對太慢，你會怎麼建索引（BK-tree、trigram index）？各有什麼代價？
