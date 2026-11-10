---
title: Reducer：不用鎖的並行累積
unit: 05 · Fork-Join 與 Reducer（HW4）
source: 6.172 HW4 §3.3 Reducers（Write-up 7–8）
pdf: materials/6.172/homework/hw04/handout.pdf
read: materials/6.172/lectures/lec06.pdf, materials/6.172/homework/hw04/handout.pdf
kind: code
sanitizer: tsan
minutes: 180
---

# 0513 · Reducer：不用鎖的並行累積

> **實務上為什麼會遇到**：平行地累加計數器、串日誌、收集結果，第一個直覺是「加鎖」或「每個 thread 一個 buffer，最後接起來」。後者會弄亂順序，而很多資料（一份有序的 log、依索引的輸出）要的就是序列語意。Cilk 的 reducer hyperobject 給的就是：沒有鎖、沒有 race、結果和序列執行一模一樣。

一個 **Monoid** `M` 提供：`value_type`、`M::identity()`（單位元素）、`M::reduce(left, right)`（把 `right` 併入 `left`，順序很重要，呼叫端保證結合律）。

`Reducer<M>` 用 `practice::fork2` 建構（見 `harness/forkjoin.hpp`）。這題**不准用 mutex**，也不要用「依 `worker_index()` 分配 view，最後依 worker 編號接起來」：那對 commutative 的 `+` 碰巧正確，但對字串串接就是錯的（想想 work stealing 的分工和 worker 編號無關）。

## Stage 1 — 每個分支一個 view

| 介面 | 行為 |
| --- | --- |
| `Reducer<M>(initial)` | 根 view 的初值是 `initial`（預設建構子用 `M::identity()`）|
| `view()` | 回傳目前 strand 的 view（參考）|
| `fork2(f, g)` | 平行執行 `f` 和 `g`，語意上和「先 `f` 再 `g`」完全相同：`f` 接續目前的 view，`g` 拿一個新的 view（`M::identity()`），`fork2` 回來時把 `g` 的 view `reduce` 進 `f` 的 view |
| `parallel_for(begin, end, body, grain = 1)` | 用 `fork2` 遞迴切半，`body(i)` 用 `view()` 更新 |
| `take()` | 平行區段結束後取出最終值 |

重點是 `view()` 怎麼知道現在是哪個 strand：一個 `thread_local` 指標，`fork2` 在執行每個分支時設定它、結束時還原（RAII）。

驗收用字串串接：不管幾個 worker，結果都要和循序相同。全部在 ThreadSanitizer 底下跑。

假設同一時間只有一個 `Reducer` 的平行區段在跑（每個 `Reducer` 各自用 `fork2` 就好，不需要處理兩個 reducer 交錯）。

## Stage 2 — 懶惰建立 view（不 steal 就沒有成本）

Cilk 只有在真的發生 steal 時才建新 view；序列執行時 reducer 的成本幾乎是 0。改良：`g` 開始時，如果 `f` 已經跑完，就直接接用 `f` 的 view，不需要新建也不需要 reduce（想想為什麼這樣還是序列語意，以及要用什麼同步才能安全地知道 `f` 跑完了沒）。

測試：`workers = 1` 時，`Reducer<M>(initial)` 的整個計算不可呼叫任何一次 `M::identity()` 或 `M::reduce()`（單一 worker 永遠不會發生 steal，所以連一個新 view 都不需要）。

## Stage 3 — 實務延伸：用 reducer 收集 N-Queens 的解

`queens_ordered(n, serial_depth)`：回傳所有解（`std::vector<std::uint64_t>`，`board` 編碼同 0512：每列 4 bits，列 0 在最低位），候選欄由小到大。`row < serial_depth` 的節點用 `reducer.fork2` 平行，其餘循序。平行與循序（`serial_depth = 0`）的結果必須**逐項相同**，包含順序。

## 面試追問

1. 為什麼 reducer 只要 monoid（結合律 + 單位元），不要求交換律？Stage 3 的順序保證靠的是哪一條性質？
2. 如果 `M::identity()` 很貴（例如新配置一個大 buffer），Stage 2 的改良差多少？（HW4 Write-up 8）
3. HW4 的 reducer 用 hash table 找 view，你這裡用 thread_local，差別是什麼？多個 reducer 同時用時，哪一種會有問題？
