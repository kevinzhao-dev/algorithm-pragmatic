---
title: 效能量測稽核（Work／Span 定律當 LP）
unit: 06 · Work/Span 理論（HW5）
source: 6.172 HW5 Write-up 2、3、4（CLRS 27.1-5）
pdf: materials/6.172/homework/hw05/handout.pdf
read: materials/6.172/lectures/lec08.pdf
kind: code
minutes: 100
---

# 0612 · 效能量測稽核

> **實務上為什麼會遇到**：同事貼出 3 個核數的跑分，說「平行度大約 20」。你能不能只靠這幾個數字指出「這三個量測不可能同時是真的」，或算出 parallelism 的可能範圍？這是做容量規劃時用得上的現實推理。

把 HW5 的手算寫成程式。假設程式是確定性的，由貪婪排程器跑在理想的平行電腦上。未知數是 `T1`（work）和 `T∞`（span）。每個量測 `Measurement{processors p, seconds T}` 給出三個限制：

- Work Law：`T ≥ T1 / p`
- Span Law：`T ≥ T∞`
- 貪婪上界（CLRS 27.1-3）：`T ≤ (T1 − T∞) / p + T∞`

此外 `T1 ≥ T∞ ≥ 0`。這些限制在 `(T1, T∞)` 平面上圍出一個（可能為空的）凸多邊形。

提示：線性限制的交點就是頂點，二維的 LP 用「列舉所有直線交點、留下可行的」就夠了。

## Stage 1 — 一致性與範圍

`audit_measurements(measurements)`，回傳 `Audit{consistent, min_work, max_work, min_span, max_span, min_parallelism, max_parallelism}`。

- `measurements` 不可為空，`processors >= 1`，`seconds > 0`，否則丟 `std::invalid_argument`。
- `consistent == false` 時其他欄位不用看。
- `T∞` 可以任意小時，`min_span` 為 0、`max_parallelism` 為 `+∞`（`std::numeric_limits<double>::infinity()`）。比較數值用不超過 `1e-6` 的相對誤差。

對應 HW5：Write-up 2 求 parallelism 上下界，Write-up 3／4 找出不一致的量測。請自己手算 HW5 的三組數字驗證（`T4 = 100, T64 = 10` 的 parallelism 範圍是多少？）。

## Stage 2 — 指出是哪幾個量測打架

`minimal_conflict(measurements)`：回傳**最小**的互相矛盾子集（索引遞增），有多組同樣大的回傳字典序最小者；全部一致時回傳空陣列。（Helly 定理保證矛盾子集最多 3 個；單一量測永遠一致。）

## Stage 3 — 外插到新核數

`predict_runtime(measurements, p)`：回傳 `RuntimeBounds{low, high}`：所有可能的 `(T1, T∞)` 中，`T_p` 的最小可能值（`max(T1/p, T∞)` 的最小值）和最大可能值（`(T1−T∞)/p + T∞` 的最大值）。量測本身不一致時丟 `std::domain_error`。

注意 `max(T1/p, T∞)` 的最小值不一定在頂點，還可能在直線 `T1 = p·T∞` 和多邊形邊的交點上。

## 面試追問

1. Work Law、Span Law 為什麼是「必要」條件，而貪婪上界只在「貪婪排程器」假設下成立？如果排程器不貪婪，這個稽核還成立嗎？
2. 為什麼 `max_parallelism` 可以是無限大？再多一個量測就能把它收緊的條件是什麼？
3. 真實機器上 cache 和 NUMA 會讓這個模型怎麼失準？你會怎麼在報告裡標注？
