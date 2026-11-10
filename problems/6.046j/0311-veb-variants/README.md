---
title: vEB 的兩種變形（Policy-based 設計）
unit: 03 · van Emde Boas（PS3）
source: 6.046J PS3 Problem 3-1
pdf: materials/6.046j/psets/ps03/questions.pdf
answer: materials/6.046j/psets/ps03/solutions.pdf
read: materials/6.046j/lectures/lec04.pdf
kind: code
minutes: 150
---

# 0311 · vEB 的兩種變形

> **實務上為什麼會遇到**：同一個資料結構常常要試多種切割策略和取捨。把變動的部分抽成 policy，讓編譯器在零成本的情況下換掉，比複製程式碼或加 runtime 分支都乾淨。

把 [`0301`](../0301-van-emde-boas/README.md) 的結構改寫成 `template <typename Split, bool ExcludeMax> class Veb`，介面和 0301 相同（`size`、`empty`、`min`、`max`、`contains`、`insert`、`erase`、`successor`、`predecessor`、`node_count`、`visits`），範圍規則也相同。

`Split::high_bits(bits)` 決定一層切出幾個高位元（`bits ≥ 2` 時回傳 `[1, bits-1]`），低位元是剩下的。`visits()` 的定義和 0301 一樣，每個操作最多進入 `2·L + 2` 個節點，`L` 是沿較大那一半遞迴的層數。

這題的目標是**改寫成一個 template**，不要 `Veb` 裡寫三份。用 `if constexpr (ExcludeMax)` 分開不同的情況。

## Stage 1 — 三分之一切分

`Veb<CubeRootSplit, false>`：`u^{1/3}` 個 cluster，每個 `u^{2/3}` 大小。

`CubeRootSplit::high_bits` 已經幫你處理了 `bits == 2`，否則高位元會是 0 導致無窮遞迴。另外測試會確認遞迴深度真的比原本的 `SqrtSplit` 多，也就是你真的照著 policy 切分。

## Stage 2 — 最大值也不存進 cluster

`Veb<SqrtSplit, true>`：除了 `min`，`max` 也只存在節點上，不存進任何 cluster。

- 只有一個元素時 `min == max`；只有兩個元素時 cluster 全空，`node_count() == 1`。**第三個元素才會進 cluster。**
- `insert` 遇到比 `max` 大的 key，舊的 `max` 要被擠進 cluster，跟 `min` 對稱。
- `successor` 找不到下一個 cluster 時，答案可能是 `max`，這是最容易漏的一步。`predecessor` 對稱。
- `erase` 要處理「元素恰好兩個」和「summary 為空」的特殊情況，以及刪到 `max` 時要從最後一個 cluster 拉新的 `max` 上來。

## Stage 3 — 兩者並存與任意 Policy

`Veb<CubeRootSplit, true>` 要能跑；任何符合 `high_bits` 規約的 policy 也要能跑（測試自訂了一個固定切高 4 位元的 policy）。不要假設 `Split` 只有這兩種，也不要把「max 不進 cluster」的判斷寫成只對某個 policy 成立。

## 面試追問

1. 原題問 `INSERT`、`DELETE`、`SUCCESSOR` 的複雜度要怎麼變。對 `u^{1/3}` 這個切法，遞迴關係是什麼？層數從 `log₂ log₂ u` 變成多少？常數差多少？
2. 把 `max` 也排除，漸進複雜度完全沒變，那換到的是什麼？省下來的節點和多寫的分支，哪個比較划算？
3. 用 `if constexpr` 和用虛擬函式（strategy pattern）分別有什麼取捨？這裡為什麼前者比較對？
