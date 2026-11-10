---
title: Greedy Scheduler 與 Work／Span 定律
unit: 06 · Work/Span 理論（HW5）
source: 6.172 HW5 Write-up 1（CLRS 27.1-3）、Write-up 4；Recitation R4
pdf: materials/6.172/homework/hw05/handout.pdf
answer: materials/6.172/recitations/r4/solutions.pdf
read: materials/6.172/lectures/lec08.pdf, materials/6.172/recitations/r4/problems.pdf
kind: code
minutes: 120
---

# 0611 · Greedy Scheduler 與 Work／Span 定律

> **實務上為什麼會遇到**：`make -j`、CI pipeline、平行資料管線，全都是「一張相依圖 + P 個 worker」。一個貪婪排程器最多比最佳排程慢兩倍，這個保證就是為什麼這些系統都不用去解 NP-hard 的最佳排程。

這題把 CLRS 第 27 章的定理寫成程式，並且用隨機的 DAG 當場驗證它。「strand」是一段單位時間、不可切分的循序程式；DAG 的頂點是 strand，邊 `u → v` 表示 `u` 必須先做完。

## Stage 1 — Work 與 Span

`ComputationDag(n)`：`n` 個 strand（`0..n-1`）。`add_edge(u, v)`：`u` 必須在 `v` 之前。索引超出範圍丟 `std::out_of_range`，`u == v` 丟 `std::invalid_argument`。

- `size()` / `work()`（單位時間，work = strand 數）
- `span()`：最長路徑上的**頂點數**；空 DAG 為 0；有環丟 `std::logic_error`。要用 O(V+E) 的迭代版本（測試會給一條百萬長的鏈，遞迴會把 stack 撐爆）。
- `parallelism()`：`work / span`；空 DAG 回傳 1。

## Stage 2 — Greedy 排程

`greedy_schedule(dag, p, priority)`，回傳 `Schedule{step_of, length}`：`step_of[v]` 是 `v` 執行的時間步（從 0 算），`length` 是總步數（空 DAG 為 0）。

每個時間步：所有前驅都做完（在更早的時間步）的 strand 叫 ready。如果 ready 的數量 `>= p`，選其中 `priority[v]` 最小的 `p` 個（平手取編號小的）；否則全部都跑。`p >= 1`（`p == 0` 丟 `std::invalid_argument`）。有環丟 `std::logic_error`。複雜度 O((V+E) log V)。

測試會在隨機 DAG 上驗證三件事都成立：`T_p ≥ T1/p`（Work Law）、`T_p ≥ T∞`（Span Law），以及 CLRS 27.1-3 的 `T_p ≤ (T1 − T∞)/p + T∞`。

## Stage 3 — 差兩倍的兩個貪婪排程

CLRS 27.1-4（HW5 Write-up 4 的另一種讀法）：同一張 DAG，同樣是貪婪排程，一個可以比另一個慢將近兩倍。

`near_twice_instance(p, k)` 回傳一張 DAG 和兩份 priority（`good_priority`、`bad_priority`），讓 `k` 很大時 `T_bad / T_good` 趨近 `2 - 1/p`。自己想這張 DAG 長什麼樣子（提示：一條長度 `k` 的鏈，加上夠多可以平行做的事）。

## Stage 4 — 實務延伸：Build system

`plan_build(tasks, workers)`：`BuildTask{duration, deps}`（`duration >= 1` 的整數，`deps` 是之前的 task 編號）。回傳 `BuildPlan{total_work, critical_path, makespan}`。

排程規則：任何時刻有空的 worker 就從 ready 的 task 中拿編號最小的開始執行（non-preemptive list scheduling）。`critical_path` 是最長路徑的時間總和。複雜度 O((V+E) log V)。全部 task 完成的時間就是 `makespan`。（這就是 `make -jN`，而且 Graham 的界 `makespan ≤ (T1−T∞)/p + T∞` 在這裡依然成立。）

## 面試追問

1. 不看課本證明 27.1-3：用 complete step 和 incomplete step 分類，寫出 `T_p ≤ T1/p + T∞`，並說明如何進一步收緊成 `(T1−T∞)/p + T∞`。（HW5 Write-up 1）
2. 為什麼 work stealing 的保證（`T_p ≤ T1/p + O(T∞)`）也接近這個界？差別在哪？
3. R4 的 DAG 題（work/span/parallelism）用你的 `ComputationDag` 把它建出來驗證答案。
