---
title: 區間 DP 工具箱（回文、最優 BST、取硬幣）
unit: 06 · Augmentation、APSP 與 MST（PS6）
source: 6.046J Lecture 10, Recitation 5
pdf: materials/6.046j/lectures/lec10.pdf
read: materials/6.046j/lectures/lec10.pdf, materials/6.046j/recitations/rec05.pdf
kind: code
minutes: 100
---

# 0602 · 區間 DP 工具箱

> **實務上為什麼會遇到**：diff 工具的 LCS、編譯器的 matrix-chain 與 register allocation、靜態字典的最優查找樹，全都是「把區間切開，猜切點」的同一招。面試題不會告訴你要用 DP，你要自己看出子問題是 `(i, j)`。

講義四步驟：定義子問題 → 寫遞迴 → 由小到大計算 → 回溯出答案。每關都要真的走完第 4 步。

## Stage 1 — 最長回文子序列（subsequence）

`std::string longest_palindromic_subsequence(std::string_view s)`：回傳 `s` 的任一條最長回文子序列（順序相同、可跳字）。空字串回傳空字串。時間 O(n²)。

## Stage 2 — 最優 BST

鍵 `0..n-1` 由小到大排好，權重 `weights[i]`。找出 BST 使 `Σ weights[i]·(depth(i)+1)` 最小（根深度 0）。

```cpp
struct BstPlan {
    long long cost = 0;              // 0 when there are no keys
    int root = -1;                   // -1 when there are no keys
    std::vector<int> left, right;    // left[i] / right[i]: child key of i, -1 if none
};
BstPlan optimal_bst(std::span<const long long> weights);
```

時間 O(n³)。邊界：沒有鍵、只有一個鍵。

## Stage 3 — 兩人取硬幣

一排硬幣，兩人輪流從最左或最右取一枚，兩人都用最優策略。`long long first_player_value(std::span<const long long> values)` 回傳先手保證拿到的最大金額。時間 O(n²)。空序列回傳 0。

## Stage 4 — 再兩個切法

**堆塔**。`Block{length, width, height}` 不可旋轉；`A` 可放在 `B` 上方的條件是 `A.length ≤ B.length` 且 `A.width ≤ B.width`。`long long tallest_tower(std::span<const Block> blocks)` 回傳最高塔高，每塊至多用一次，無塊時 0。

**布林括號**。`expr` 由 `T`/`F` 與運算子 `&`、`|`、`^` 交錯組成（例如 `T^F&T`，保證合法），`count_true(expr)` 回傳有多少種括號方式使結果為真，模 `1'000'000'007`。時間 O(n³)。

## 面試追問

1. 最優 BST 的 greedy（每次選權重最大的鍵當根）為什麼錯？舉出最小反例。Knuth 優化如何把 O(n³) 降到 O(n²)？
2. 取硬幣：為什麼偶數枚時先手一定不輸？O(n²) 的 DP 比「選奇數位或偶數位」的策略多拿到什麼？
3. 布林括號：為什麼要同時維護 true 與 false 兩張表？只留一張會發生什麼事？
