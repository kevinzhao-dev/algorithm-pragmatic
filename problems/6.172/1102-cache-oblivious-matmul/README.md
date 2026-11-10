---
title: 矩陣乘法：迴圈順序、Tiling 與 Cache-Oblivious
unit: 11 · Caching（HW8）
source: 6.172 Lecture 14；Recitation 8 第 3 題
pdf: materials/6.172/lectures/lec14.pdf
answer: materials/6.172/recitations/r8/solutions.pdf
read: materials/6.172/recitations/r8/problems.pdf
kind: code
minutes: 90
---

# 1102 · 矩陣乘法：迴圈順序、Tiling 與 Cache-Oblivious

> **實務上為什麼會遇到**：同樣 O(n³) 的矩陣乘法，只是換個迴圈順序就能差 10 倍以上。Eigen、BLIS 底層就是這些招式。tiling 要針對快取調「voodoo 參數」，cache-oblivious 的遞迴做法不用調就有近似最佳的 miss 數，這題用模擬器量出來親眼看。

全部寫成樣板，矩陣的型別只提供 `rows()`、`cols()`、`get(i, j)`、`set(i, j, v)`（row-major）。結果**累加到 `c`**（`c(i, j) += ...`），呼叫前 `c` 已歸零。`a`、`b`、`c` 都是 n×n。

測試用 `_shared/08_cache_sim.hpp` 的 `TracedMatrix` 計 miss（每次 `get`/`set` 算一次 element access），所以 miss 數精確可重現，不靠真實硬體。下面敘述裡 M 是快取大小、B 是 line 大小。

## Stage 1 — 迴圈順序

`multiply_ijk(a, b, c)`：內層 `k` 迴圈走 `b` 的一欄，對 row-major 很不友善。

`multiply_ikj(a, b, c)`：只把內兩層交換，內層變成走 `a` 的一列和 `c` 的一列。

## Stage 2 — Tiling

`multiply_tiled(a, b, c, s)`：以 `s × s` 為單位分塊，外三層 `i1, k1, j1` 依序以 `s` 為步長（做到 n 為止），內三層 `i, k, j` 在各自的 tile 內跑。`s` 不必整除 n，`s > n` 也合法。

## Stage 3 — Cache-oblivious 遞迴

`multiply_recursive(a, b, c)`（`n` 保證是 2 的次方）：對半分成四個象限，遞迴 8 次（順序照 Recitation 8 第 3 題的 Rec_Mult：`C11 += A11·B11`、`C11 += A12·B21`、`C12 += A11·B12`、`C12 += A12·B22`、`C21 += A21·B11`、`C21 += A22·B21`、`C22 += A21·B12`、`C22 += A22·B22`），base case 是 `n == 1`。

測試會驗證這個版本不需要任何調參：在不同大小的快取上都幾乎和「專門為那個快取調好的 tiling」一樣好，而 tiling 參數一旦調錯就輸很多；miss 數也會照 `Θ(n³/(B√M))` 隨 M 變化。

## 面試追問

1. 用 Recitation 8 第 3 題的方法推遞迴版的 Q(n)：recurrence、遞迴樹高度、leaf 的 miss 總和。
2. 為什麼 ijk 在 `n > M/B` 時每次存取幾乎都 miss，換成 ikj 卻只要 `Θ(n³/B)`？
3. 真的要跑得快，base case 不會是 1×1。Cache-oblivious 的漸近保證跟實際的最佳化（切換到 BLAS kernel）怎麼一起用？
