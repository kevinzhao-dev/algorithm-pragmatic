---
title: 最佳象限切片（max-prefix 擴充）
unit: 04 · Trees & Heaps（PS4）
source: 6.006 PS4 Problem 4-6
pdf: materials/6.006/psets/ps4/questions.pdf
answer: materials/6.006/psets/ps4/solutions.pdf
read: materials/6.006/lectures/lec07.pdf, materials/6.006/recitations/rec07.pdf
kind: code
minutes: 90
---

# 0416 · 最佳象限切片

> **實務上為什麼會遇到**：挑選兩個門檻（例如「年齡 ≤ X 且消費 ≤ Y 的使用者投放廣告」）讓期望收益最大，或在監控資料裡找讓某個帶正負號的指標總和最大的 quadrant。把「掃描」從 O(n²) 降到 O(n log n)，靠的是在平衡樹上維護 max-prefix。

平面上有 `n` 個帶正負號價值的點（`Topping { x, y, t }`）。選一個切點 `(X, Y)`，切片的值是所有 `x ≤ X` 且 `y ≤ Y` 的點的 `t` 總和（不含任何點的切片值為 0）。找出值最大且為正的切片。

## Stage 1 — Max-Prefix 擴充 AVL

用 0403 的 `AvlMap` 或 0402 的 `AvlTree` 加上一個 augmentation（建議 `#include "../0403-avl-map/solution.hpp"`）。

`PrefixTree` 維護一組 `key → value`（兩者都是 `int64_t`），依 key 排序：

- `insert(key, value)`：新 key 回傳 `true`；已存在時覆蓋 value 並回傳 `false`。O(log n)。
- `erase(key)`：有刪到回傳 `true`。O(log n)。
- `value_of(key)`、`size()`。
- `max_prefix()`：**O(1)**。定義 `prefix(k)` 為所有 `key ≤ k` 的 value 總和；回傳 `(k*, prefix(k*))`，其中 `k*` 是某個**存在於樹中**的 key，使 `prefix(k*)` 最大。樹為空時回傳 `std::nullopt`。

提示：每個子樹的摘要要存「總和」和「最佳前綴的值與 key」，這是一個 monoid（想想 `combine(a, b)` 怎麼算，尤其是最佳前綴落在 `b` 裡的情形）。全部都是負數時 `prefix` 也可能是負的，這時 `k*` 是讓 prefix 最不負的 key。

## Stage 2 — 最佳切片

`tastiest_slice(toppings)`：假設所有點的 `x` 互不相同、所有 `y` 也互不相同。回傳任一個 `Slice{X, Y, t}`：`X` 是某個點的 `x`，`Y` 是某個點的 `y`，`t` 是這個切片的值，且 `t` 是所有切片中的最大值。

不存在值為正的切片時回傳 `std::nullopt`。

**O(n log n)**。依 `x` 排序後依序把點加到 Stage 1 的樹上（key 是 `y`、value 是 `t`），每加一個就讀一次 `max_prefix()`。

## Stage 3 — 重複的座標

同一個 `x` 或同一個 `y` 可能出現多次（`Topping` 互不相同，只有座標會重複）：

- 新增 `add(key, delta)`：key 不存在時等同於 `insert(key, delta)`；否則 value 加上 `delta`。O(log n)。
- 同一個 `x` 的點必須**全部加入之後**才能讀 `max_prefix()`，否則會把「只含部分 `x` 相同的點」算成一個切片。同一個 `y` 的點要加到同一個 key 上。

## 面試追問

1. 為什麼最佳切片一定可以取在某個點的 `x` 和某個點的 `y` 上？
2. 掃描時為什麼依 `x` 排序就夠了，而不是對 `(x, y)` 雙重枚舉？複雜度差多少？
3. 如果點會持續新增或刪除，要隨時維持最佳切片，這套方法還行嗎？缺什麼？
