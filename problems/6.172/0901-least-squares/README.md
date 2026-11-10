---
title: Least Squares 與複雜度分類
unit: 09 · Dynamic Analysis（HW7）
source: 6.172 HW7 §3（Linear-regression analysis）
pdf: materials/6.172/homework/hw07/handout.pdf
kind: code
minutes: 70
---

# 0901 · Least Squares 與複雜度分類

> **實務上為什麼會遇到**：「一次 function call 要多久」這種問題沒有直接的儀器能量，只能量很多次總時間，再用 regression 分出各項成本。把量測結果 fit 成成本模型，是效能工程最常用的手段，模型的條件好壞（共線性）也決定了數字能不能信。

作業的數學全部自己寫。後面的 0911–0913 會直接使用 `_shared/08_ols.hpp`（與本題介面相同）。

> ⚠️ **爆雷警告**：`_shared/08_ols.hpp` 就是這題的完整解答，做完之前不要打開。

## Stage 1 — 一元線性回歸

`fit_line(x, y)`：回傳使平方誤差最小的 `y = slope·x + intercept`，以及 `r_squared`（`1 - SS_res/SS_tot`；`SS_tot` 為 0 時定義為 1）。

`x` 和 `y` 長度不同、少於 2 筆，或所有 `x` 相同：丟 `std::invalid_argument`。

## Stage 2 — 相關係數與多元回歸

- `pearson(x, y)`：Pearson 相關係數。任一序列為常數時回傳 0。不合法輸入同 Stage 1。
- `fit_multilinear(rows, y)`：解 `y ≈ rows · β`（**不含**截距項），`rows[i]` 是第 i 筆的所有 regressor。用 normal equations 配上 partial pivoting 的 Gaussian elimination。

作業模型 `T = a·I + b·F` 裡 `I` 可能到 10⁹、`F` 只有 10³，尺度差太多會讓 pivot 判斷失效，所以每一欄要先做 scale（例如除以絕對值最大者）。regressor 線性相依時丟 `std::domain_error`，不可以回傳垃圾。

## Stage 3 — 從計數估計複雜度

把 regression 用在呼叫次數上，就得到一個「複雜度偵測器」。

- `power_law_exponent(n, counts)`：`log(counts)` 對 `log(n)` 的斜率。
- `classify_growth(n, counts)`：回傳 `Growth`。做法：對 `Constant / Logarithmic / Linear / NLogN / Quadratic / Cubic`，各自用最小平方解 `counts ≈ a·f(n)`，算 log 尺度上的 RMS 殘差；`Exponential` 用 `log(counts) = n·log b + log a` 的兩參數 fit。殘差最小者勝出，相差在 `1e-9` 以內時選清單裡排在前面的。

最少需要 3 筆資料，否則丟 `std::invalid_argument`。

## 面試追問

1. 為什麼 normal equations 的數值穩定性不如 QR？什麼時候差別會很大？作業這種兩個 regressor 的情況需要在意嗎？
2. 共線性（collinearity）對回歸結果的影響是什麼？怎麼設計實驗來避免？（作業 Write-up 2）
3. log-log 斜率把 `n log n` 看成多少？為什麼 Stage 3 的做法要拿 `n log n` 自己當候選，而不是只看斜率？
