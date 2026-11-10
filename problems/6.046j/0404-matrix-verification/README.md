---
title: Matrix Multiply 與 Freivalds 驗證
unit: 04 · Amortization 與 Randomization（PS4）
source: 6.046J Recitation 1, Lecture 6
pdf: materials/6.046j/lectures/lec06.pdf
read: materials/6.046j/recitations/rec01.pdf, materials/6.046j/lectures/lec06.pdf
kind: code
minutes: 90
---

# 0404 · Matrix Multiply 與 Freivalds 驗證

> **實務上為什麼會遇到**：把昂貴的矩陣運算丟給 GPU 或不受信任的 worker 算之後，要怎麼便宜地確認結果沒被硬體錯誤或 bug 弄壞？重算一次等於白做，Freivalds 用 O(n²) 驗證 O(n³) 的工作。

`Matrix<Ring>` 是 `n × n` 的方陣，以值語意保存元素。`Ring` 是可以做 `+`、`-`、`*`、`==` 的環：`Ring{}` 是 0，`Ring(1)` 是 1，測試主要用 `std::uint64_t`（自然模 2⁶⁴）。

## Stage 1 — Matrix 與直接相乘

`Matrix(n)` 建立全 0 矩陣，`size()`、`m(i, j)`（0-based）、`operator==`。`multiply_naive(a, b)` 做 n³ 次乘法。尺寸不符時丟 `std::invalid_argument`。

## Stage 2 — Strassen

`multiply_strassen(a, b, cutoff)`：`n ≤ cutoff` 時改用 `multiply_naive`。`n` 不是 2 的冪次時，補 0 到下一個 2 的冪次，算完再切回來。`cutoff == 0` 時丟 `std::invalid_argument`。

- 每一層遞迴只能做 7 次子問題乘法，也就是 `n = 2^k`、`cutoff = 1` 時總乘法次數**剛好** `7^k`。
- 下一層的矩陣比這一層小，Strassen 的七個子問題一定要在子矩陣上遞迴，不是自己算 8 個四分之一。

## Stage 3 — Freivalds 驗證

`verify_product(a, b, c, rng, rounds)`：判斷 `a × b` 是否等於 `c`。

- 若等式成立，**一定**回傳 `true`。
- 若不成立，每一輪至少有 1/2 機率回傳 `false`，所以回傳 `true` 的機率不超過 `2^{-rounds}`。
- 每輪取隨機向量 `r ∈ {0,1}^n`，比較 `A(Br)` 和 `Cr`。每輪只能做 3 次矩陣乘向量，總乘法次數 `≤ 3·rounds·n²`。

Lecture 6 在 mod 2 下分析，這題改成 `std::uint64_t` 的 wrap-around；分析不需要改，只用到 `Dr` 和 `D(r + e_j)` 不可能同時為 0。

## 面試追問

1. 為什麼 7 次遞迴能打敗 8 次？`T(n) = 7T(n/2) + Θ(n²)` 怎麼解？為什麼實務上都要設 cutoff？
2. Freivalds 是 Monte Carlo 還是 Las Vegas？如果把 `r` 的每個分量都改成從更大的範圍均勻挑，誤判機率會怎麼變？
3. 這類「驗證比計算便宜」的思路還能用在哪裡？（想想 Schwartz–Zippel，或你在 0201 看到的檢查方式。）
