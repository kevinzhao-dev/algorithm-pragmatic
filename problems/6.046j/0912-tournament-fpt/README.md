---
title: Tournament Feedback Arc：Kernel 與 FPT 搜尋
unit: 09 · Approximation 與 FPT（PS9）
source: 6.046J PS9 Problem 9-2
pdf: materials/6.046j/psets/ps09/questions.pdf
answer: materials/6.046j/psets/ps09/solutions.pdf
read: materials/6.046j/lectures/lec18.pdf
kind: code
minutes: 160
---

# 0912 · Tournament Feedback Arc

> **實務上為什麼會遇到**：把一堆兩兩比較（A 優於 B）修補成一個一致的排名（模型評測、搜尋結果的 pairwise preference）。「最少翻轉幾個比較才能得到全序」正是 feedback arc set；當不一致很少時，FPT 比暴力好幾個數量級。

`Tournament` 是 `std::vector<std::vector<char>>`：`t[u][v] == 1` 表示 `u → v`；`u ≠ v` 時恰好一個方向為 1，對角線為 0。

**cycle cover** 是一組邊 A，使得每個 directed cycle 至少有一條邊在 A 裡。題目要判斷 T 有沒有大小 ≤ k 的 cycle cover。翻轉 A 中所有邊之後，T 會變成 acyclic（Part a）。

一次只看一關。

## Stage 1 — Triangle（Part a 的基礎）

- `bool is_acyclic(const Tournament& t)`：O(n²)。
- `std::optional<std::array<int,3>> find_triangle(const Tournament& t)`：找 `a → b → c → a`，沒有回傳 `nullopt`。tournament 有 cycle ⟺ 有 triangle。O(n²)。

## Stage 2 — Kernelization（Part b）

```cpp
struct TournamentKernel {
    bool no_instance = false;
    Tournament reduced;
    int k = 0;
    std::vector<int> to_original;            // reduced 的頂點 i 對應原編號
    std::vector<std::pair<int,int>> reversed;  // 已決定翻轉的邊 (u,v)，u→v 是 T 中的方向，原編號
};
TournamentKernel kernelize(const Tournament& t, int k);
```

規則（反覆套用）：Rule 1 — 若有邊同時在 ≥ `k+1` 個 triangle 中，翻轉它並 `k-1`；Rule 2 — 刪除不在任何 triangle 中的頂點。若過程中 `k < 0` 或最後頂點數 > `k(k+2)`，回傳 `no_instance`。

## Stage 3 — FPT 求解（Part c）

`std::optional<std::vector<std::pair<int,int>>> feedback_arcs(const Tournament& t, int k)`：若存在，回傳一組邊 `(u, v)`（T 中 `u→v`，大小 ≤ k），翻轉後 T 變 acyclic；否則 `nullopt`。先 kernelize，再對 kernel 做 bounded search：找一個 triangle，三條邊各分岔翻轉一次，遞迴深度不超過 k（O(3^k·n²)）。

## Stage 4 — 實務延伸：修補排名

`std::optional<std::vector<int>> repair_ranking(const Tournament& t, int max_upsets)`：若能在翻轉至多 `max_upsets` 條邊內達成，回傳最終全序（第 0 名排最前面），使得違反原始比較的次數（upsets）最少；否則 `nullopt`。

## 面試追問

1. 為什麼翻轉「最小」的 cycle cover 就保證 acyclic？（提示：每條 A 中的邊都有自己專屬的 cycle。）
2. Rule 1 的安全性為什麼依賴 `k+1` 這個門檻？Rule 2 為什麼不影響答案？
3. Kernel 大小 `k² + 2k` 是怎麼來的？這題的 FPT 上界和實務上還能怎麼改進（branch on 不相交 triangle packing）？
