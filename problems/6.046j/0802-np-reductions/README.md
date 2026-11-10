---
title: NP 完全性：把 Reduction 寫成程式
unit: 08 · LP 與 NP-Completeness（PS8）
source: 6.046J Lecture 16, Recitation 8
pdf: materials/6.046j/lectures/lec16.pdf
read: materials/6.046j/lectures/lec16.pdf, materials/6.046j/recitations/rec08.pdf
kind: code
minutes: 150
---

# 0802 · 把 Reduction 寫成程式

> **實務上為什麼會遇到**：判斷一個需求是不是 NP-hard，決定了你該找精確解、近似解還是啟發式。把 reduction 當作程式寫一次，你才會真的理解「兩個方向都要對」是什麼意思。

每一關的產出都是三個函式：**reduction**（把問題 A 的實例轉成問題 B 的實例）、**verifier**（檢查證據，NP membership 的部分），以及 **lift**（把 B 的證據還原成 A 的證據，這就是「B 回答 yes ⇒ A 回答 yes」那個方向的證明）。測試會在小的隨機實例上用暴力解驗證兩邊答案相同，並檢查 lift 出來的證據。

圖都是 `Graph{n, edges}`，頂點 `0..n-1`，邊 `{u, v}`（無向、`u != v`、不重複）。

一次只看一關。

## Stage 1 — Verifiers

寫出下列 polynomial-time 驗證器：

| 函式 | 判定內容 |
| --- | --- |
| `is_clique(g, vertices)` | 兩兩相鄰 |
| `is_independent_set(g, vertices)` | 兩兩不相鄰 |
| `is_vertex_cover(g, vertices)` | 每條邊至少有一端在集合內 |
| `is_set_cover(universe, sets, chosen)` | `chosen` 內各 set 的聯集是 `0..universe-1` |

`vertices`、`chosen` 是互不相同的索引。

## Stage 2 — Clique ≤ₚ Independent Set ≤ₚ Vertex Cover ≤ₚ Set Cover

- `IsInstance clique_to_is(const CliqueInstance&)`，`IsInstance` 是 `{g, k}`，`CliqueInstance` 同樣。
- `VcInstance is_to_vc(const IsInstance&)`。
- `SetCoverInstance vc_to_set_cover(const VcInstance&)`，回傳 `{universe, sets, k}`。
- lift：`lift_is_to_clique(src, witness)`、`lift_vc_to_is(src, witness)`、`lift_set_cover_to_vc(src, dst, chosen)`，從目標問題的證據還原出來源問題的證據。

## Stage 3 — Subset Sum ≤ₚ Partition

`PartitionInstance subset_sum_to_partition(const SubsetSumInstance& s)`；`SubsetSumInstance{a, t}`、`PartitionInstance{a}`，`a` 是非負整數。

`lift_partition_to_subset_sum(src, dst, side)`：`side` 是 `dst.a` 中落在其中一邊的索引（集合和等於總和一半）；回傳 `src.a` 的索引子集，和為 `src.t`。

## Stage 4 — Hamiltonian Cycle ≤ₚ Hamiltonian Path

有向圖 `Digraph{n, edges}`。`Digraph ham_cycle_to_path(const Digraph& g)`；`std::vector<int> lift_path_to_cycle(const Digraph& src, const Digraph& dst, std::span<const int> path)`：`path` 是 `dst` 的 Hamiltonian path（頂點序列），回傳 `src` 的 Hamiltonian cycle（頂點序列，長度 `n`，回到起點的邊隱含）。

## Stage 5 — 3DM ≤ₚ Subset Sum（weak NP-hardness）

三維 matching：`ThreeDm{n, triples}`，`triples[i] = {x, y, z}`，`0 ≤ x,y,z < n`。

`SubsetSumInstance three_dm_to_subset_sum(const ThreeDm& t)`；`lift_subset_sum_to_3dm(src, dst, chosen)`：把子集和的 index 子集還原成 triple 的 index 子集。編碼時 base 要夠大，避免進位污染（這題的數字會指數級地大）。

## 面試追問

1. 把 Karp reduction 的 YES⇒YES 和 NO⇒NO 講清楚，為什麼不能只證一個方向？
2. Subset Sum 是 weakly NP-hard，為什麼它有 pseudo-polynomial 演算法而 3-Partition 沒有？
3. 為什麼 NP 的定義只要求 YES 實例有短的證據？co-NP 是什麼？
