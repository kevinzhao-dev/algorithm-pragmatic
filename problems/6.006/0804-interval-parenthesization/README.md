---
title: 括號化與 Matrix Chain（區間 DP）
unit: 08 · Dynamic Programming II（PS8）
source: 6.006 Lecture 17
pdf: materials/6.006/lectures/lec17.pdf
read: materials/6.006/lectures/lec17.pdf
kind: code
minutes: 85
---

# 0804 · 括號化與 Matrix Chain

> **實務上為什麼會遇到**：`numpy.linalg.multi_dot`、`opt_einsum` 和 SQL optimizer 的 join ordering 都在決定「運算的順序」。子問題是連續區間，答案靠列舉最後一刀切在哪；正負號一進來，子問題還得把「最小值」一起記下，因為兩個負數相乘可能變成最大值。

輸入是一串運算元和它們之間的運算子：`operands.size() == ops.size() + 1`，`ops` 的每個字元是 `'+'` 或 `'*'`。你可以任意加括號（改變計算順序），但運算元和運算子的左右順序不能動。保證任何加括號方式的計算結果都放得進 `std::int64_t`。

`operands` 空、`ops` 長度不對或含其他字元時丟 `std::invalid_argument`。

一次只看一關，做完再往下讀。

## Stage 1 — 非負運算元的最大值

`std::int64_t max_value(std::span<const std::int64_t> operands, std::string_view ops)`：`operands` 都 ≥ 0，回傳加括號之後能算出的最大值。O(n³) 時間。

## Stage 2 — 允許負數

`operands` 可以是負數（函式不變）。只記最大值不夠了：想想兩個區間的最大值相乘，什麼時候不是整體的最大值。

## Stage 3 — 還原括號

`std::string best_parenthesization(std::span<const std::int64_t> operands, std::string_view ops)`：回傳達到最大值的一種加括號方式，格式固定為：

- 單一運算元：寫成十進位，負數帶 `-`，例如 `-3`。
- 每次運算都包一層括號：`(左式+右式)` 或 `(左式*右式)`，沒有空白。

例如 `(1+(2*-3))`。

## Stage 4 — 實務延伸：Matrix Chain

`dims` 代表 `n = dims.size() − 1` 個矩陣相乘，第 `i` 個矩陣大小是 `dims[i] × dims[i+1]`（`dims` 皆為正，`dims.size() ≥ 2`，否則丟 `std::invalid_argument`）。把大小 `a×b` 和 `b×c` 兩個矩陣相乘的成本算作 `a·b·c`。

```cpp
std::int64_t min_multiplications(std::span<const std::int64_t> dims);
std::string best_multiplication_order(std::span<const std::int64_t> dims);
```

回傳最小總成本，以及達成它的相乘順序，格式：單一矩陣寫 `M{i}`（`i` 是十進位索引），相乘寫 `(左式*右式)`，例如 `((M0*M1)*M2)`。O(n³) 時間。

## 面試追問

1. Stage 2 的子問題為什麼是 O(n²) 而不是更多？relation 要看幾種組合？
2. 把「最後一刀」當子問題和用「第一刀」，哪個在這裡比較自然？還原解時的樹長什麼樣？
3. `opt_einsum` 在張量數量很多時不會跑完整的 DP。它改用什麼？犧牲了什麼？
