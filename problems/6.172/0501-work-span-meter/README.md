---
title: Work／Span Meter（自己做 Cilkscale）
unit: 05 · Fork-Join 與 Reducer（HW4）
source: 6.172 HW4 §2 Recitation（Checkoff 1–3、5）、Recitation R4、Lecture 8
pdf: materials/6.172/homework/hw04/handout.pdf
answer: materials/6.172/recitations/r4/solutions.pdf
read: materials/6.172/homework/hw04/handout.pdf, materials/6.172/lectures/lec06.pdf, materials/6.172/lectures/lec08.pdf, materials/6.172/recitations/r4/problems.pdf
kind: code
sanitizer: tsan
minutes: 120
---

# 0501 · Work／Span Meter

> **實務上為什麼會遇到**：開了 16 個 thread 卻只快 3 倍，問題通常不是排程器，而是程式的 parallelism 本來就只有 3。Cilkscale 做的事就是量 work（T₁）和 span（T∞）。這題自己做一個確定性的版本，之後全部的平行作業都用它驗證複雜度。

> ⚠️ **爆雷警告**：`problems/6.172/_shared/01_exec.hpp` 裡有完整的 `Meter`，後面的題目拿它當測試工具。做完這題之前不要打開。

沒有 Cilk，平行的基本操作是 `practice::fork2(f, g)`（見 `harness/forkjoin.hpp`）。這題有一個 **executor** 的概念：演算法寫成 template，同一份程式碼可以量 cost（`Meter`）也可以真的平行跑（`ForkJoin`）。測試用 ThreadSanitizer 跑，資料競爭會直接失敗。

## Stage 1 — Cost 代數

`struct Cost { work, span; }`，`parallelism()` 為 `work / span`（`span == 0` 回傳 1）。

`series(a, b)`：先做 a 再做 b，work 和 span 都相加。`parallel(a, b)`：同時做，work 相加、span 取最大。

## Stage 2 — Meter

寫一個依序（serial）執行、但精確計算 cost 的 executor：

| 介面 | 語意 |
| --- | --- |
| `charge(n = 1)` | 目前的 strand 做了 `n` 單位工作，同時計入 work 和 span |
| `fork2(f, g)` | 先呼叫 `f()` 再呼叫 `g()`（serial 執行），但計帳時當作平行：`fork2` 本身 charge 1 單位（在 fork 之前的 strand 上），兩邊的 cost 以 `parallel` 合併，加到目前 strand |
| `parallel_for(begin, end, body, grain = 1)` | 遞迴二分（用 `fork2`）到長度 `<= max(grain,1)` 為止，葉節點依序呼叫 `body(i)`；不額外 charge，由 `body` 自己計帳 |
| `cost()` / `spawns()` | 累計的 cost；`fork2` 被呼叫的次數 |

## Stage 3 — Fib 與 coarsening

`fib(exec, n, cutoff)`：`n <= 1` 回傳 `n`（fib(0)=0）。規則要**精確**（測試會比對帳目）：

- 每次函式呼叫 charge 1（包含 base case）。
- `n > cutoff` 時用 `exec.fork2` 計算 `fib(n-1)` 與 `fib(n-2)`；`n <= cutoff`（且 `n >= 2`）時直接一般遞迴（不 fork），這就是 coarsening。

用 `Meter` 量 `fib(24)`：`cutoff` 從 1 拉到 12，spawn 次數少了好幾個量級，span 則會變長（base case 的 serial 鏈全算進 span），但只要 parallelism 仍遠大於核心數，這筆交易就賺到了。**想想為什麼一次 spawn 的真實成本會讓你願意付出這個 span**。

## Stage 4 — 真的平行

`ForkJoinExec`：介面同 Meter，`charge` 什麼都不做，`fork2`／`parallel_for` 轉呼叫 `practice::fork2`／`practice::parallel_for`。

- `parallel_sum(exec, data, grain)`：用 `parallel_for` 加總，不可有資料競爭（想想怎麼避免共用累加器）。回傳總和。每個元素 `charge(1)`。
- `transpose_in_place(exec, grid, n, grain)`：`n×n`（row-major，`grid.size() == n*n`）就地轉置，外層和內層都用 `parallel_for`。每交換一對元素 `charge(1)`。（這對應 HW4 Checkoff 3，用平行迴圈而不是遞迴。）

## Stage 5 — Speedup 預測

`speedup_bounds(cost, p)`：回傳 `{lower, upper}`，分別是貪婪排程器在 `p` 個處理器上的理論保證下限，以及 work／span law 給的上限（`min(p, parallelism)`）。下限用 `T_p ≤ T1/p + T∞`：`T1 / (T1/p + T∞)`。

## 面試追問

1. R4 的五種 fib 版本（`spawn` 放不同位置、兩個都 spawn、都不 spawn）效能怎麼排序？`spawn` 接著馬上 `sync` 為什麼沒有用？（解答在 R4 solutions）
2. 為什麼 Cilkscale 用儀器化量 work/span，而不是看實際跑得多快？
3. 把 `fork2` 的 spawn 成本從 1 調成 100，上面兩個 `cutoff` 的取捨會怎麼變？
