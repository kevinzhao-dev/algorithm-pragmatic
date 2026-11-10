---
title: Median of Medians（確定性 Select）
unit: 01 · Scheduling 與 Divide & Conquer（PS1）
source: 6.046J Lecture 2
pdf: materials/6.046j/lectures/lec02.pdf
read: materials/6.046j/lectures/lec02.pdf
kind: code
minutes: 90
---

# 0103 · Median of Medians

> **實務上為什麼會遇到**：要算 p50、p99，或從一批候選裡挑前 k 個，不需要把資料整個排序。`std::nth_element` 平均 O(n)，但遇到特定輸入可能退化；要有最壞情況保證，就是這個演算法。

`T` 只保證可以複製、移動和 `operator<`。不要用 `==`。

## Stage 1 — 三向分割

`partition3(values, pivot)`：把 `values` 重排成「小於 pivot、等於 pivot、大於 pivot」三段，回傳 `{less_end, greater_begin}`，也就是後兩段的起點。

每個元素最多比較 2 次。`pivot` 用傳值，因為呼叫端常常從 `values` 裡取值，傳參考會在重排時被改掉。

## Stage 2 — 確定性 Select

`select_kth(values, k)`：回傳第 `k` 小（0-based）的元素，`values` 會被重排但元素不可遺失。`k` 超出範圍時丟 `std::out_of_range`。

要求最壞情況 O(n)，測試用 `Counted` 數比較次數：

- 五組中位數當 pivot，比較次數不超過 `60n`。
- `n` 從 `2^10` 長到 `2^20`，每個元素的平均比較次數不能跟著變多。這條會擋掉 `sort`，它的比例會隨 `lg n` 成長。
- 測試裡有一個自適應的對手（adaptive adversary）：它邊看你比較邊決定元素大小，只要你的 pivot 規則不是真的有保證，就會被逼到 Θ(n²)。隨機 pivot 也過不了。

輸入可能全部相同，或只有少數幾種值。

## Stage 3 — 實務延伸：加權中位數

監控系統常常收到已經彙整過的直方圖，一筆資料是「這個值出現了幾次」。

`weighted_median(samples)`：`W` 是所有 `weight` 的總和，回傳**最小的** `v`，使 `value ≤ v` 的樣本權重總和 `S` 滿足 `2S ≥ W`。總權重保證不超過 `int64`，`2S` 則可能超過，要小心比較方式。

沒有樣本或有 `weight ≤ 0` 時丟 `std::invalid_argument`。**O(n)**，不可以先排序。

## 面試追問

1. 為什麼是 5 個一組？3 個一組的遞迴式為什麼解不到線性？7 個呢？
2. 這個演算法的常數很大，實測下甚至比排序慢。那它存在的意義是什麼？實務上 `nth_element` 怎麼兼顧平均速度和最壞保證？（提示：introselect。）
3. 加權中位數為什麼每輪都能捨棄一半的樣本？如果 pivot 不是真正的中位數，總複雜度會變成多少？
