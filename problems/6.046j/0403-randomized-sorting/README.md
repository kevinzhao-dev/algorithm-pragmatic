---
title: Randomized Quicksort 與 Select
unit: 04 · Amortization 與 Randomization（PS4）
source: 6.046J Lecture 6, Recitation 4
pdf: materials/6.046j/lectures/lec06.pdf
read: materials/6.046j/lectures/lec06.pdf, materials/6.046j/recitations/rec04.pdf
kind: code
minutes: 90
---

# 0403 · Randomized Quicksort 與 Select

> **實務上為什麼會遇到**：線上服務裡，只要有人能操控輸入順序，固定 pivot 的 quicksort 就能被打成 O(n²)。隨機化把最壞情況從「輸入的性質」換成「擲骰子的運氣」，這也是 `sort` 和 `nth_element` 實作的基礎。

每個函式接受 `std::uniform_random_bit_generator` 當亂數來源，讓測試結果可重現。`T` 只保證可以複製、移動和 `operator<`。輸入可能全部相同（三向切割是必要的）。

## Stage 1 — Randomized Quicksort

`randomized_quicksort(values, rng)`：每次隨機挑 pivot，三向切割。期望比較次數 O(n log n)，對任何輸入都成立。

小的那一邊遞迴、大的那一邊用迴圈，遞迴深度才能保持 O(log n)。

## Stage 2 — Paranoid Quicksort

`paranoid_quicksort(values, rng)`：一直重選 pivot，直到切出來的小於區與大於區都不超過 `3n/4`，才遞迴下去。回傳**遞迴深度**（`n ≤ 1` 時為 0，否則為 1 加上兩個子問題深度的較大值）。

這個深度對任何亂數都有上界 `⌈log_{4/3} n⌉ + 1`，比較次數的期望仍然是 O(n log n)（每輪期望重試不到兩次）。

## Stage 3 — Randomized Select

`randomized_select(values, k, rng)`：回傳第 `k` 小（0-based），`k` 超出範圍時丟 `std::out_of_range`。期望 O(n)。

結束時 `values` 必須被**分割在第 `k` 名附近**：前 `k` 個都不大於它，`k` 之後的都不小於它。下一關會用到。

## Stage 4 — 實務延伸：Top-K

`top_k(values, k, rng)`：回傳最大的 `k` 個元素，**由大到小排序**。`k` 大於元素數時丟 `std::out_of_range`，`k == 0` 回傳空。可以重排 `values`。

期望比較次數要是 `O(n + k log k)`，也就是 `k` 遠小於 `n` 時不能把整個陣列排序。實務上這就是每分鐘算一次熱門 API 排行榜的做法。

## 面試追問

1. 為什麼隨機 pivot 的分析不需要假設輸入是隨機的？這和 hash table 的「simple uniform hashing」假設差在哪？
2. Paranoid quicksort 的分析為什麼比原版簡單？重選 pivot 的代價是多少，實際上會不會比較慢？
3. `randomized_select` 的最壞情況是多少？`std::nth_element` 怎麼在實作上避免它？
