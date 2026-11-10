---
title: 用 Regression 量 Spawn 的成本
unit: 09 · Dynamic Analysis（HW7）
source: 6.172 HW7 §3 Write-up 4–5（cost of a spawn）
pdf: materials/6.172/homework/hw07/handout.pdf
read: materials/6.172/lectures/lec13.pdf
kind: code
minutes: 60
---

# 0913 · 用 Regression 量 Spawn 的成本

> **實務上為什麼會遇到**：決定 parallel 的 grain size 時，你要知道「多開一個 task 比一般函式貴多少」。fork-join 的 overhead 一般就是幾十個 cycle 到幾百個 cycle，量過一次才知道 cut-off 該設多大。

沿用 0912 的流程，把 regressor 換成 spawn 次數。模型是作業的 `T = a·I + b·S`：`I` 是一般工作量（包含一般函式呼叫），`S` 是 `fork2` 的次數，`b` 就是一次 spawn 的成本。

作業的 `cilk_spawn` 在這裡換成 `harness/forkjoin.hpp` 的 `practice::fork2`。`support/workload.cpp` 的 `workload::fib_spawn(n, base)` 只在 `n >= base` 時用 `fork2`，每次 fork 前都會呼叫一次 `spawn_marker()`，這是作業 `__csi_detach` 的等價物：你的 hook 數它的進入次數就是 `S`。

把 workers 設成 1（作業的 `CILK_NWORKERS=1`）：`practice::set_workers(1)`。這樣量到的是 spawn 本身的 overhead，沒有 steal，hook 也是單執行緒的。

Hook 和介面寫在 `profiler.cpp`。regression 請用 `_shared/08_ols.hpp`。

## Stage 1 — 計數（Write-up 4）

`count_run(n, base)`：跑一次 `workload::fib_spawn(n, base)`，回傳 `{instructions, spawns}`：`instructions` 是 `work_units()` 的增量，`spawns` 是 hook 數到的 `spawn_marker` 進入次數。

## Stage 2 — Fit

- `sample_virtual(n, base)`：一筆 `Sample{instructions, spawns, time}`，`time` 是虛擬時鐘的增量。
- `fit_spawn_cost(samples)`：解 `T = a·I + b·S`，回傳 `CostModel{per_instruction, per_spawn}`。regressor 相依時讓 `std::domain_error` 傳出去。

## Stage 3 — 真實量測（Write-up 5）

用 **不含** instrumentation 的 `workload_plain`：

- `time_fib_spawn_ns(n, base, repeats)`：量 `workload_plain::fib_spawn(n, base)`，回傳 `repeats` 次的最小值（奈秒）。
- `design_inputs()`：回傳你的 `(n, base)` 實驗設計，至少 6 組，`n ≤ 24`。想想 `S` 要怎麼跟 `I` 分開變動。
- `measure_spawn_cost(inputs, repeats)`：counts 用 instrumented 版本、時間用 plain 版本，回歸出每次 spawn 的奈秒數。
- `call_cost_ns()`：量一次 `workload_plain::empty_call()` 的奈秒數（這是 opaque 的函式，呼叫不會被 inline）。

`PERF` 會印出結果（`run --perf-only --verbose`），並檢查一次 spawn 至少是一次 function call 的 2 倍。

## 面試追問

1. Stage 3 的比例 `b / call_cost` 告訴你什麼？怎麼用這個數字推出 parallel 的 cut-off 應該設多少？（作業 Write-up 5）
2. workers = 1 時量到的是哪些成本？workers 變多之後，steal 會多出什麼成本，為什麼 regression 比較難分出來？
3. Cilk 有 work-first principle：把成本集中在 steal 路徑，不要放在 spawn 路徑。`forkjoin.hpp` 哪裡違反了這個原則？（Lecture 13）
