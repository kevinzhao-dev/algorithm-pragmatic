---
title: 矩形矩陣乘法的快取分析
unit: 11 · Caching（HW8）
source: 6.172 HW8 §1 Checkoff 1
pdf: materials/6.172/homework/hw08/handout.pdf
read: materials/6.172/lectures/lec14.pdf
kind: code
minutes: 70
---

# 1111 · 矩形矩陣乘法的快取分析

> **實務上為什麼會遇到**：真實資料很少是正方形，tall-and-skinny 的矩陣（例如 PCA 的資料矩陣）極常見。不同維度比例會落在不同的快取區間，分析要逐一討論，這題把那些漸近分析變成跑得起來、對得到的東西。

A 是 m×n、B 是 n×r、C 是 m×r，都用 row-major。延續 1102 的矩陣介面（`rows/cols/get/set`），結果累加到 `c`。

作業要你分析這幾個情況（M = 快取容量、B = line 大小，兩者都以「元素」為單位）：

- naive 三層迴圈（`i`, `j`, `k` 分別跑 m、r、n）在 `n > M/B` 的時候。
- 同樣的 naive 在 `M/r < n < M/B` 的時候。
- cache-oblivious 遞迴。

## Stage 1 — Naive

`multiply_rect_naive(a, b, c)`：`for i in m, for j in r, for k in n`：`c(i, j) += a(i, k) * b(k, j)`。

## Stage 2 — 遞迴

`multiply_rect_recursive(a, b, c)`：把 m、n、r 三維中最大的那一維切半（平手時優先切 m，再來 n），分成兩個子問題遞迴，直到三維都是 1。子問題的切法：

- 切 m：A 和 C 依 row 切開，上下兩個子問題。
- 切 n：A 依 column、B 依 row 切開。
- 切 r：B 和 C 依 column 切開。

## Stage 3 — 分析（Checkoff）

把你的分析寫成兩個函式，測試會拿模擬器（M = 512 elements，B = 8 elements）的實際 miss 數比對：

- `predict_naive_misses(m, n, r, cache)`：回傳 naive 在上述兩個情況的 Θ 估計（不用管常數，但要大致相近）。
- `predict_recursive_misses(m, n, r, cache)`：回傳遞迴版本的 Θ 估計，提示：`Θ(mnr/(B√M) + (mn+nr+mr)/B + m+n+r)`。

`CacheGeometry` 給你 `capacity_elements`（M）、`line_elements`（B）與 `lines()`（M/B）。

naive 的預測與實際要在 2 倍之內，遞迴要在 4 倍之內。為了排除 conflict miss（Lecture 14 講過，row-major 對 2 的次方的維度特別傷），這題的模擬器用 fully associative。

## 面試追問

1. 在 `n > M/B` 的情況下，為什麼 naive 每次存取 B 都會 miss？跟 r 的大小有關嗎？
2. tiling 的版本只能依 m、n、r 的某個固定比例調參，遞迴版本為什麼不用知道這些比例？
3. tall cache 假設（`B² ≤ cM`）在這題的分析裡用在哪裡？少了它什麼會壞掉？
