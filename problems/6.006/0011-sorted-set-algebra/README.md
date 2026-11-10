---
title: Sorted Set 的交、聯、差（Posting List）
unit: 00 · Warm-up（PS0）
source: 6.006 PS0 Problem 0-1
pdf: materials/6.006/psets/ps0/questions.pdf
answer: materials/6.006/psets/ps0/solutions.pdf
kind: code
minutes: 50
---

# 0011 · Sorted Set 的交、聯、差

> **實務上為什麼會遇到**：搜尋引擎的 inverted index 把每個詞對應到一條排好序的文件 id 清單（posting list），`a AND b` 就是兩條清單的交集，`a NOT b` 就是差集。資料庫的 merge join 也是同一招。

作業原題用 set-builder 定義兩個集合，要你算交集、聯集大小與差集大小（數字見 PDF 第 1 頁）。這一題把它變成程式：集合一律表示成**嚴格遞增**的序列，元素型別只保證有 `<`。

一次只看一關，做完再往下讀。

## Stage 1 — Set-Builder 寫成程式

- `binomial(n, k)`：組合數 C(n, k)；`k > n` 時回傳 0。結果保證放得進 `uint64_t`，但計算中間值不一定放得進（試試 `binomial(67, 33)`）。
- `set_of(domain, f)`：回傳 `{ f(x) | x ∈ domain }`，排序並去除重複。

做完可以順手用這兩個函式建出 PDF 上的 A、B，對照解答驗算。

## Stage 2 — 線性時間的集合運算

`a`、`b` 都是嚴格遞增的 `std::span`。全部要 **O(|a| + |b|)** 次比較，一次掃描，不可用巢狀迴圈：

- `intersect<T>(a, b)`：`a ∩ b`，結果嚴格遞增。
- `union_size<T>(a, b)`：`|a ∪ b|`，不需要真的建出聯集。
- `difference_size<T>(a, b)`：`|a − b|`。

邊界：任一邊是空集合、兩邊完全相同、完全不相交。

## Stage 3 — 大小懸殊時不要逐一走過

`gallop_intersect<T>(a, b)`：行為同 `intersect`，但比較次數要是 **O(m · log(n/m + 1))**，`m` 是較小集合的大小、`n` 是較大集合的大小；哪一邊比較大都要成立。

提示：對小集合的每個元素，在大集合上**從上一次停下來的位置**往後倍增距離（1, 2, 4, …）跳，跳過頭後再 binary search（這叫 galloping 或 exponential search，0213 會再用到）。

## Stage 4 — 實務延伸：多詞查詢

`PostingIndex`：

- `add(term, doc_ids)`：`doc_ids` 嚴格遞增。同一個 term 再次 `add` 就覆蓋舊的。
- `query(must, must_not)`：回傳同時出現在所有 `must` 詞的 list、且不在任何 `must_not` 詞的 list 中的文件 id，嚴格遞增。
  - `must` 是空的或含未知的詞時，回傳空集合；未知的 `must_not` 詞直接忽略。

常見的 AND 查詢裡，一定有某個詞比其他詞罕見得多。查詢必須只在最短的 list 上逐一走過，其餘 list 用 galloping：長度 n 的 list 數量很多時也不可退化成線性掃描（有 `PERF` 檢查）。

## 面試追問

1. `gallop_intersect` 什麼時候會比一路 merge 慢？在實務系統裡你會怎麼根據兩邊大小比例選演算法？
2. 為什麼 `binomial` 的乘除順序寫對就保證中間結果都是整數？
3. Posting list 如果放不進記憶體，或是需要壓縮（delta + varint）時，galloping 還能用嗎？skip list 在這裡扮演什麼角色？
