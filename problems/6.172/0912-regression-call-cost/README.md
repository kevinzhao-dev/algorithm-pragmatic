---
title: 用 Regression 量 Function Call 的成本
unit: 09 · Dynamic Analysis（HW7）
source: 6.172 HW7 §3 Write-up 1–3（linear-regression analysis）
pdf: materials/6.172/homework/hw07/handout.pdf
kind: code
minutes: 90
---

# 0912 · 用 Regression 量 Function Call 的成本

> **實務上為什麼會遇到**：要回答「換成 virtual call 會慢多少」、「多一層 RPC 會貴多少」，直接量 total time 沒用，因為那個成本被其他工作蓋掉了。regression 能把各項成本分開，但前提是你的實驗設計得好，不然 F 和 I 高度相關時，算出來的數字沒有意義。

模型是作業的 `T = a·I + b·F`：`I` 是工作量（迴圈次數），`F` 是呼叫次數，`a`、`b` 分別是一單位工作和一次呼叫的成本。

`support/workload.cpp` 用 `-finstrument-functions` 編譯，包含一個 `fib_base(n, base)`：`n < base` 時用迴圈算（`work_units()` 會累加迴圈次數），否則遞迴。Hook 跟 `count_run` 等函式寫在這個資料夾的 `profiler.cpp`，regression 請直接用 `_shared/08_ols.hpp` 的 `practice::ols::solve`。

workload 也內建一個**虛擬時鐘**：`ticks()` 依 `kUnitCost`、`kCallCost` 累加，不受機器影響。Stage 1–3 用它，答案精確；Stage 4 才量真實時間。

## Stage 1 — 計數（Write-up 1）

`count_run(n, base)`：跑一次 `workload::fib_base(n, base)`，回傳 `{instructions, calls}`，`instructions` 是 `work_units()` 的增量，`calls` 是 hook 數到的 `fib_base` 進入次數。

## Stage 2 — Fit（Write-up 3）

- `sample_virtual(n, base)`：回傳一筆 `Sample{instructions, calls, time}`，`time` 是虛擬時鐘的增量。
- `fit_cost_model(samples)`：用 `solve` 解 `T = a·I + b·F`，回傳 `CostModel{per_instruction, per_call}`。回歸失敗（regressor 相依）時讓 `std::domain_error` 傳出去。
- `counts_correlation(samples)`：`I` 與 `F` 的 Pearson 相關係數。

## Stage 3 — 實驗設計（Write-up 2）

`design_inputs()`：回傳你要拿來做實驗的 `(n, base)` 清單。測試會檢查：

- 至少 6 組，且 `2 ≤ base`、`1 ≤ n ≤ 30`（作業的 base case 大小至少 2）。
- `I` 與 `F` 的相關係數 `|ρ| ≤ 0.9`：要讓兩者分開變動（想想 `n` 與 `base` 各自影響什麼）。
- 雜訊下也要穩：測試會加入確定性的雜訊，比較你的設計和「固定 base」的爛設計，你的估計誤差要小很多。

## Stage 4 — 真實量測（Write-up 2–3）

- `time_fib_ns(n, base, repeats)`：對 `workload_plain::fib_base`（同一份程式碼但**沒有** instrumentation）量 `repeats` 次，回傳最小值，單位奈秒。
- `measure_real_cost(inputs, repeats)`：對每組輸入，用 instrumented 的版本數 counts、用 plain 版本量時間（作業說的避免 perturbation），做 regression。

`PERF` 只做基本的合理性檢查（兩個係數都要是正的），但會把數字印出來。想看結果就直接跑 `run --perf-only --verbose`：一次 function call 在你的機器上是幾個 loop iteration 的成本？

## 面試追問

1. `T` 要用 min、mean 還是 median？三者各自適合什麼樣的雜訊？（作業 Write-up 2）
2. 為什麼 counts 和 time 要分開用兩個 binary 量？如果就用同一個 instrumented binary 量時間，係數會怎麼偏？
3. 只用一個 `base` 做實驗時，`I` 和 `F` 為什麼幾乎完全相關？這對 `a`、`b` 的標準誤有什麼影響？
