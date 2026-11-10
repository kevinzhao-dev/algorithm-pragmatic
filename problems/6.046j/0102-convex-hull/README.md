---
title: Convex Hull（Divide & Conquer）
unit: 01 · Scheduling 與 Divide & Conquer（PS1）
source: 6.046J Lecture 2
pdf: materials/6.046j/lectures/lec02.pdf
read: materials/6.046j/lectures/lec02.pdf
kind: code
minutes: 150
---

# 0102 · Convex Hull

> **實務上為什麼會遇到**：地理圍欄（geofence）、機器人路徑的安全區、物件碰撞的粗篩，第一步都是把一堆點包成凸多邊形。這類程式最常見的 bug 不在主流程，而在共線、重複點和浮點誤差。

Lecture 2 為了簡化，假設沒有共線、沒有相同 x。**這題全部要處理**。

## 數值與表示法合約

- 座標是整數，`|x|, |y| ≤ 10^9`。`(b−a)×(c−a)` 的每一項不超過 `4·10^18`，相減也在 `int64` 範圍內。**全程不准用浮點數**。
- Hull 的正規形式：頂點**嚴格凸**（共線的中間點要移除），依**逆時針**排列，從字典序（先比 x 再比 y）最小的點開始。退化情形：沒有點回傳空、一個點回傳該點、全部共線回傳兩個端點 `{較小, 較大}`。

## Stage 1 — 判斷方向與找切線

- `orient(a, b, c)`：`c` 在有向線 `a→b` 左邊回傳 `1`，右邊 `-1`，共線 `0`。
- `upper_tangent(left, right)`、`lower_tangent(left, right)`：兩個參數都是正規形式的 hull（至少一點），且 `left` 每一點的字典序都小於 `right` 每一點。回傳切點在兩個 hull 裡的索引。若切線上同時有多個頂點，回傳**最外側**的（`left` 取字典序最小、`right` 取字典序最大）。總共 O(|left| + |right|)。

Lecture 用垂直線上的截距比大小，當 x 相同時會失效。改用 `orient` 判斷，把字典序想成一個極小的 shear，就不需要任何特例。留意一點和兩點的 hull：你的 finger 不能繞圈。

## Stage 2 — 合併

`merge_hulls(left, right)`：前提同上，但任一邊可以是空的。回傳聯集的正規形式 hull，O(|left| + |right|)。

## Stage 3 — 完整演算法

`convex_hull(points)`：`points` 任意順序，可能重複、共線、x 相同。先排序一次，再分治，T(n) = 2T(n/2) + Θ(n)。

## Stage 4 — 實務延伸：地理圍欄

`ConvexRegion(points)` 把點集包成區域，建構後不可依賴傳入的 `span`。

- `hull()`：回傳正規形式的 hull。
- `contains(p)`：`p` 在區域內或邊界上時回傳 `true`，**O(log h)**，`h` 是 hull 的頂點數。退化成點或線段時，只有落在該點或線段上才算在內。

## 面試追問

1. 為什麼兩個 finger 交替移動不會錯過切線？用「不會走過頭」和「停下來時一定是支撐線」兩步說明，並指出 `>=` 和 `>` 差在哪個案例。
2. 座標換成 `double` 會壞在哪裡？如果座標必須是 `double`，業界的解法是什麼（提示：adaptive exact predicates）？
3. 這題也能用 monotone chain 在排序後 O(n) 掃出來。那分治版本還有什麼價值？（想想平行化和外部排序的合併。）
