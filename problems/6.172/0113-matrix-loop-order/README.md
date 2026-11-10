---
title: 矩陣乘法的迴圈順序
unit: 01 · C++ 基本功與工具（HW1）
source: 6.172 HW1 §6（Write-up 9–10）、Lecture 1
pdf: materials/6.172/homework/hw01/handout.pdf
read: materials/6.172/lectures/lec01.pdf
kind: perf
minutes: 90
---

# 0113 · 矩陣乘法的迴圈順序

> **實務上為什麼會遇到**：線性代數、影像處理、embedding 比對的最內層迴圈，換個巢狀順序、不動任何演算法，就能快十倍。這是第一堂課用來嚇人的例子，也是面試官最常問「為什麼」的那個。

## Stage 1 — RAII 矩陣

`Matrix(rows, cols)`，元素是 `double`，row-major 連續存放，**建構時全部歸零**（HW1 的 bug 之一就是忘了初始化）。`operator()(r, c)` 存取元素，`row(r)` 回傳該列的 `std::span`，`operator==` 比較大小和內容。

`multiply(a, b)`：回傳 `C = A × B`（`a.rows()` × `b.cols()`）。維度不合（`a.cols() != b.rows()`）丟 `std::invalid_argument`。任一維度為 0 的矩陣也要能處理。

## Stage 2 — 六種迴圈順序

```cpp
enum class LoopOrder { IJK, IKJ, JIK, JKI, KIJ, KJI };

template <typename Body>
void for_each_index_triple(std::size_t rows, std::size_t cols, std::size_t depth,
                           LoopOrder order, Body&& body);   // body(i, j, k)

Matrix multiply_ordered(const Matrix& a, const Matrix& b, LoopOrder order);
```

`for_each_index_triple` 依 `order` 的巢狀順序呼叫 `body(i, j, k)`，`i ∈ [0, rows)`、`j ∈ [0, cols)`、`k ∈ [0, depth)`。例如 `IKJ` 是 `for i, for k, for j`。

`multiply_ordered` 用這個函式實作，六種順序結果都要相同（對應 `a.rows()`、`b.cols()`、`a.cols()`）。

## Stage 3 — 用 cache 模型解釋差異

`std::vector<LoopOrder> rank_by_cache_misses(n, line_bytes, sets, ways)`：對 `n × n × n` 的乘法，用 `MiniCache`（`support/mini_cache.hpp`）餵入位址序列，回傳**由 miss 最少到最多**的六種順序。

位址模型：`double` 占 8 byte；`A` 從位址 0、`B` 從 `1<<30`、`C` 從 `2<<30` 開始，都是 row-major。每執行一次最內層的 `C[i][j] += A[i][k] * B[k][j]`，依序存取 `C[i][j]`、`A[i][k]`、`B[k][j]`、`C[i][j]`。

跑完想清楚：最快的兩個順序有什麼共同點？最慢的兩個呢？

## Stage 4 — 量真實的時間

測試用 `n = 512` 的 `multiply_ordered` 比較：

- `IKJ` 至少比 `IJK` 快 4 倍。
- `IKJ` 至少比同一段邏輯關掉最佳化的版本（`-O0` 的效果，用 `[[clang::optnone]]` 在同一個執行檔裡做 baseline）快 4 倍（對應 HW1 Write-up 10）。

自己試試把 `body` 用 `std::function` 傳，看 ratio 掉到多少（為什麼？）。

## 面試追問

1. 為什麼 IKJ 比 IJK 快這麼多，光靠 cache 解釋夠嗎？（提示：最內層能不能向量化？）
2. Stage 3 的模型是位址層級的，沒有 hardware prefetcher。真實機器上 JKI 比模型預測更慘還是更好？
3. 如果 `n` 大到比 L2 還大，你下一步會做什麼？（提示：blocking／cache-oblivious）
