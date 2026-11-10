---
title: LCS 與最小 diff（Hirschberg）
unit: 07 · Dynamic Programming I（PS7）
source: 6.006 Lecture 16
pdf: materials/6.006/lectures/lec16.pdf
read: materials/6.006/lectures/lec16.pdf
kind: code
minutes: 110
---

# 0702 · LCS 與最小 diff

> **實務上為什麼會遇到**：`git diff`、three-way merge、DNA sequence alignment 都是 longest common subsequence 的親戚：兩份序列，保留共同的部分，其餘的就是改動。檔案一大，二維表的記憶體會比時間先把你擋下來。

最長共同子序列：`a` 和 `b` 共同的子序列中最長的一個（不必連續，順序要相同）。

一律寫成函式樣板，接受任何 `std::ranges::random_access_range`，元素只需要 `operator==`。

一次只看一關，做完再往下讀。

## Stage 1 — 長度

`std::size_t lcs_length(const A& a, const B& b)`，O(|a||b|) 時間，每對元素最多比較常數次。

## Stage 2 — 還原一組解

`Alignment lcs_alignment(const A& a, const B& b)`：`Alignment` 是 `std::vector<std::pair<size_t, size_t>>`，每個 pair `(i, j)` 表示 `a[i]` 和 `b[j]` 配對。`i` 和 `j` 都要嚴格遞增，長度等於 LCS。同長度的解有很多，任一組都可以。

## Stage 3 — 把 `lcs_length` 的記憶體降到 O(min(|a|, |b|))

只求長度時，relation 只看前一列，而且可以挑較短的序列當欄。額外配置要在 `O(min(|a|, |b|))` 以內（測試量 heap 高水位）。Stage 2 的 `lcs_alignment` 不用改。

## Stage 4 — 線性記憶體還原解（Hirschberg）

二維 parent pointer 在 10⁵ × 10⁵ 時放不下。實作 `Alignment lcs_alignment_linear(const A& a, const B& b)`：時間仍是 O(|a||b|)，heap 高水位 O(|a| + |b|)。

作法的關鍵：只用 Stage 3 的滾動列，正著跑前半段的 `a`、反著跑後半段的 `a`，就能算出最佳解在中線穿過 `b` 的哪個位置。切成兩塊之後各自遞迴，兩塊的工作量加起來只有原本的一半。

## Stage 5 — 實務延伸：行級 diff

```cpp
enum class DiffKind { Keep, Remove, Add };
struct DiffLine { DiffKind kind; std::string_view text; };

std::vector<DiffLine> diff_lines(std::span<const std::string> before,
                                 std::span<const std::string> after);
```

- 把 `Keep` 和 `Remove` 依序取出，要等於 `before`；把 `Keep` 和 `Add` 依序取出，要等於 `after`。
- `Keep` 的行數要等於 LCS 長度，也就是最小 diff。
- 兩個 `Keep` 之間，同一段改動裡所有 `Remove` 都排在 `Add` 前面（一般 diff 的慣例）。
- `text` 指向傳入的字串，呼叫端要讓它們活得比結果久。

## 面試追問

1. Stage 4 為什麼時間只是常數倍？把每一層遞迴的工作量加起來給面試官看。
2. Stage 3 的滾動為什麼讓 Stage 2 的還原做不到？還有哪些 DP 能用同樣的「中線」手法？
3. `git diff` 實際用的 Myers 演算法複雜度是 O((n+m)D)。什麼輸入下它比 LCS 表快得多？什麼時候沒差？
