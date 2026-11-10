---
title: Subset Sum、Bitset 與 Partition（pseudopolynomial）
unit: 08 · Dynamic Programming II（PS8）
source: 6.006 Lecture 18, Recitation 18
pdf: materials/6.006/lectures/lec18.pdf
read: materials/6.006/lectures/lec18.pdf, materials/6.006/recitations/rec18.pdf
kind: code
minutes: 90
---

# 0802 · Subset Sum、Bitset 與 Partition

> **實務上為什麼會遇到**：把工作平均分給兩台機器、在預算內湊出精確的組合，都是 subset sum。O(n·T) 看起來平凡，但 T 是數字的大小、不是輸入的長度，這就是 NP-hard 問題還能有「快」演算法的原因。實際寫的時候，把 bit 當平行運算單元（word-level shift）常常能再快 64 倍。

一次只看一關，做完再往下讀。

## Stage 1 — 判定

`bool subset_sum(std::span<const int> values, int target)`：`values` 皆為正、`target ≥ 0`，問有沒有一個子集合（可為空集合）的總和剛好等於 `target`。O(n·target) 時間。

每個元素最多用一次。

## Stage 2 — 找出一個子集合

`std::optional<std::vector<std::size_t>> subset_sum_witness(std::span<const int> values, int target)`：回傳一組遞增的索引，總和剛好是 `target`；沒有時回傳 `std::nullopt`。同樣 O(n·target) 時間。

這裡的目標是記憶體 O(target) 而不是 O(n·target)：想想「`sum` 第一次變成可達是在處理到哪個元素時」這件事能告訴你什麼。

## Stage 3 — Word-level bitset

```cpp
class SumSet {
public:
    explicit SumSet(std::size_t limit);       // tracks sums in [0, limit]; starts as {0}
    void add(std::size_t value);              // include one more item; O(limit / 64)
    bool contains(std::size_t sum) const;     // O(1); sums above limit are false
    std::size_t count() const;                // how many sums in [0, limit] are reachable
};
```

`add` 不可以逐 bit 處理：用 `std::uint64_t` 的陣列做「整段左移再 OR」。要處理好 `value` 剛好是 64 的倍數、`value > limit`、`limit` 剛好落在 word 邊界。記憶體要在 `limit / 8` bytes 的常數倍以內。

## Stage 4 — 最接近的二分

```cpp
struct Partition { std::int64_t difference; std::vector<std::size_t> left; };
Partition closest_partition(std::span<const int> values);
```

把 `values`（皆為正）分成兩堆，兩堆總和的差最小。`left` 是其中一堆的索引（遞增），`difference` 是兩堆差的絕對值。O(n·Σvalues) 時間。

## Stage 5 — 允許負數

`bool subset_sum_signed(std::span<const int> values, std::int64_t target)`：`values` 可以有負數和 0，空子集合的總和是 0。O(n·(Σ|values|)) 時間。

## 面試追問

1. 為什麼 O(n·T) 不算 polynomial？`T` 的位元數增加 1 時，時間怎麼變？什麼情況下它其實很好用？
2. Stage 2 的「第一次可達」為什麼足以還原解？順序換成任意排列，解還對嗎？
3. 如果 `values` 很少但數字很大（`n = 40`、`T = 10^12`），你會改用什麼作法？（Meet in the middle）
