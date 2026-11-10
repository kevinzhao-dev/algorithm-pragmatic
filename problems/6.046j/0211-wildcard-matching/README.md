---
title: Wildcard Matching（用 FFT 做字串比對）
unit: 02 · FFT 與 B-Trees（PS2）
source: 6.046J PS2 Problem 2-1
pdf: materials/6.046j/psets/ps02/questions.pdf
answer: materials/6.046j/psets/ps02/solutions.pdf
read: materials/6.046j/lectures/lec03.pdf
kind: code
minutes: 90
---

# 0211 · Wildcard Matching

> **實務上為什麼會遇到**：序列分析（DNA 片段比對）和日誌掃描常見帶萬用字元的固定樣式。KMP 這類演算法處理不了 `*`，一旦不能靠失敗函數，把比對改寫成卷積就是標準解法。

文字 `text` 不含萬用字元，樣式 `pattern` 中的 `*` 恰好比對**一個**任意字元，其他字元要完全相同。回傳所有比對位置 `j`，也就是 `pattern` 對上 `text[j..j+m-1]`，遞增排序。

共通規則：

- 含字母表以外的字元時丟 `std::invalid_argument`（`*` 只能出現在 `pattern`）。
- 空 `pattern` 比對所有位置 `0..n`（含 `n`）。`pattern` 比 `text` 長時回傳空。

## Stage 1 — 直接比對

`match_naive(text, pattern)`：字母表是 `{a, b}`，最壞 O(nm)。

## Stage 2 — 轉成卷積

`match_fft(text, pattern)`：字母表同上，**O((n+m) log (n+m))**。把 [`0201`](../0201-fft-polynomial/README.md) 寫好的 `fft` 複製過來。

把 `a` 編成 `1`、`b` 編成 `-1`、`*` 編成 `0`，反轉樣式，相乘一次，乘積的第 `m-1+j` 項就是樣式在位置 `j` 的「符合程度」。請自己想清楚什麼情況算命中，以及為什麼。

**數值合約**：係數只有 `0、±1`，乘積的絕對值不超過 `m`，比 0201 的合約寬鬆很多，四捨五入一定正確。但只比較 `llround` 的結果，不要用浮點數直接比大小。

測試有最壞情況：每個位置都要比到最後一個字元才失敗，直接比對會超時。

## Stage 3 — 任意字母表

`match_alphabet(text, pattern, alphabet)`：`alphabet` 是字母表（不可為空、不可含 `*`、不可重複，否則丟 `std::invalid_argument`）。

用二元編碼把每個字元轉成 `k` 個 `{a, b}`，然後呼叫 Stage 2。`*` 要展開成 `k` 個 `*`。編碼後比對可能落在字元中間，要過濾掉：編碼後的第 3 位對應到原文哪一格？

## 面試追問

1. 正確的命中條件是什麼？為什麼 `1` 和 `-1` 的編碼可以用乘積判斷，改成 `1` 和 `0` 就不行？
2. `alphabet` 很大時（例如整個 Unicode），二元編碼把長度放大 `log σ` 倍。不靠編碼的做法是什麼？（提示：`Σ P[j](P[j] − T[j])²`。）
3. 如果 `pattern` 很短但 `text` 非常長，要如何切塊把 FFT 的成本從 `O(n log n)` 降到 `O(n log m)`？
