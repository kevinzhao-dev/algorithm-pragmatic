---
title: Close Stations（Divide & Conquer 找近距離點）
unit: 01 · Scheduling 與 Divide & Conquer（PS1）
source: 6.046J PS1 Problem 1-2
pdf: materials/6.046j/psets/ps01/questions.pdf
answer: materials/6.046j/psets/ps01/solutions.pdf
read: materials/6.046j/lectures/lec02.pdf
kind: code
minutes: 120
---

# 0112 · Close Stations

> **實務上為什麼會遇到**：部署無線基地台、配置機架上的感測器時，規則常是「任兩點距離要超過門檻」。規模一大就不能兩兩比較，分治和掃描帶的技巧也是最近點對、碰撞偵測的標準做法。

給定站點座標，判斷有沒有站點靠得太近，並回傳實例。

## 數值合約

- 座標是整數，`|x|, |y| ≤ 10^9`。
- 「在距離內」定義成 `dx² + dy² ≤ radius_sq`，其中 `radius_sq` 是非負的 `int64`。
- 兩點的距離平方最多 `8·10^18`，放得進 `int64`。**全程不准用浮點數**：測試裡有差距只有 1 的案例，`hypot` 分不出來。
- 站點可能重複（距離 0），x 或 y 也可能相同，原題沒有保證互異，這裡也不保證。

輸入順序任意，回傳值是輸入陣列的索引。

## Stage 1 — 一對

`find_close_pair(stations, radius_sq)`：若存在 `i < j` 且 `i`、`j` 在距離內，回傳任一組 `{i, j}`；否則回傳 `std::nullopt`。

**O(n log n)**。用 divide-and-conquer：依 x 切一半，兩邊都沒找到時，只需檢查分隔線兩側距離 `r` 內的掃描帶，而且每一點只需要跟 y 順序上往後的常數個點比。掃描帶裡的點如果不是已經依 y 排好，你就得在每一層都排序，或是用 merge 順便維護。

## Stage 2 — 三個

`find_close_triple(stations, radius_sq)`：若存在 `i < j < k` 且三點兩兩都在距離內，回傳任一組排序好的 `{i, j, k}`；否則回傳 `std::nullopt`。**O(n log n)**。不能先枚舉所有 close pair 再湊三角形，close pair 的數量可能是 Θ(n²)。

Stage 1 能用常數個後續點的原因，在這裡要重新推導：左右兩半現在**可以**有 close pair，只是沒有 close triple，所以格子裡點的上限變了。

## 面試追問

1. 把平面切成邊長 `r/2` 的格子。為什麼一格裡最多只能有一點（Stage 1）或兩點（Stage 2）？掃描帶中每一點最多要往後看幾個點？
2. 如果可以用 hash table，怎麼把這題做到期望 O(n)？這個做法的壞情況是什麼？對抗性輸入怎麼防？（對照 PS5 的 universal hashing。）
3. 分隔線剛好穿過多個同 x 的點時，為什麼演算法仍然正確？
