---
title: 依成本模型選排序法
unit: 02 · Sorting & Recurrences（PS2）
source: 6.006 PS2 Problem 2-2
pdf: materials/6.006/psets/ps2/questions.pdf
answer: materials/6.006/psets/ps2/solutions.pdf
read: materials/6.006/lectures/lec03.pdf
kind: code
minutes: 45
---

# 0212 · 依成本模型選排序法

> **實務上為什麼會遇到**：真實系統的成本模型很少是「比較和搬動都一樣貴」。flash 的寫入貴、跨機器比較大物件貴、資料幾乎已排序（時間序列、線上更新）也常見。選對演算法靠的是先想清楚：**貴的是什麼操作？**

作業的三個情境各自讓一種基本排序（selection、insertion、merge）勝出。這題把「為什麼」變成可驗證的預算：**函式依自己選的演算法實作，測試計算貴的操作次數，超過預算就失敗。** 這三關的 selection、insertion、merge sort 都可以直接搬 0202、0203 的版本改寫。

## Stage 1 — 寫入很貴

一個序列 `seq` 只提供 `size()`、`get_at(i)`（便宜）、`set_at(i, v)`（寫入，非常貴）。

`sort_expensive_writes(seq)`：把 `seq` 排成遞增，**`set_at` 呼叫次數 ≤ 2(n − 1)**（每次交換算兩次寫入）。元素可以比較大小與移動/複製。讀取次數不限制（可以 O(n²)）。

## Stage 2 — 比較很貴

`sort_pointers(pointers, less)`：`pointers` 是 `std::span<const T*>`，指向 `n` 個物件。把**指標**重新排列，使得指向的物件依 `less(const T&, const T&)` 遞增（物件本身不動，可能有相等）。

每次 `less` 的成本遠高於移動指標（物件很大或比較要經過網路），所以**比較次數 ≤ 1.5·n·⌈log₂ n⌉ + n**。

## Stage 3 — 資料幾乎排好

`repair_nearly_sorted<T>(items, less)`（預設 `std::less<T>`）：`items` 是某個排好序的陣列，經過**未知次數 k 次**相鄰元素交換後的結果。把它排回遞增。

比較次數 ≤ `2n + 2k`，也就是對幾乎排好的資料要接近線性（`k` 不會傳給你）。

## 面試追問

每一關都要能講出取捨，這才是原題的重點：

1. Stage 1 為什麼不用 insertion 或 merge？`set_at` 的成本要大到多少，selection sort 才值得？
2. Stage 2 為什麼是 merge 而不是 selection？如果比較便宜、但要穩定排序物件本身（會搬動大物件），答案會怎麼變？
3. Stage 3 為什麼一次相鄰交換最多只增加一個 inversion？`k = log log n` 時 insertion sort 的時間是多少？
