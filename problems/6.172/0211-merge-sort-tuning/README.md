---
title: Merge Sort 逐步調校
unit: 02 · Profiling（HW2）
source: 6.172 HW2 §3 Homework: Sorting（Write-up 1–7）
pdf: materials/6.172/homework/hw02/handout.pdf
read: materials/6.172/homework/hw02/handout.pdf, materials/6.172/lectures/lec02.pdf
kind: perf
minutes: 240
---

# 0211 · Merge Sort 逐步調校

> **實務上為什麼會遇到**：效能工程九成時間都在做這件事：一次只改一個變因，每改一步都量一次，把假設換成數字。這題拿一個平凡的 stable merge sort，一路套用 inlining、pointer、coarsening、記憶體重用，看每一招到底值多少。

每個 stage 新增一個獨立的函式，前一版**留在檔案裡**（不要改寫它），這正是 HW2 的流程，也是 baseline 比較的來源。所有函式都是 **stable** merge sort，簽名如下（`Compare` 預設 `std::less<>`）：

```cpp
template <typename T, typename Compare = std::less<>>
void sort_x(std::span<T> data, Compare cmp = {});
```

不可使用 `std::sort`、`std::stable_sort`、`std::inplace_merge`。

## Stage 1 — Baseline

`sort_a`：把 HW2 的 `sort_a.c` 翻成 C++。遞迴到單一元素，merge 時用兩個暫存陣列。空陣列、單一元素都要正確。比較次數不可超過 `n⌈lg n⌉`。

## Stage 2 — Inlining（§3.1）

`sort_i`：比較子用 template 參數傳入，讓編譯器能內聯。**`inline` 關鍵字在 `-O2` 幾乎沒差**：hot loop 本來就會被內聯，真正擋路的是「編譯器看不到被呼叫的函式」。

測試的 baseline（`support/sort_baselines.hpp` 的 `erased_sort`）是 `qsort` 式介面：每次比較都是間接呼叫、每次搬元素都是 `memcpy(size)`。你的 `sort_i` 要在同一個執行檔裡比它快至少 **1.1 倍**（用 16-byte record 比較；兩邊都在每次 merge 配置暫存，這個成本後面的 stage 才處理，實際差距通常在 1.3 倍上下。量一下自己的數字）。

## Stage 3 — Pointer 迭代與 aliasing（§3.2）

- `sort_p`：`merge` 改用指標走訪，行為與 `sort_i` 相同，正確即可。量一下：指標版和索引版差不多。編譯器不笨，兩種寫法多半產生同樣的機器碼。**指標真正值錢的地方是下一項。**
- `add_to_all(ByteView& view, uint8_t delta)`：把 `view` 的每個 byte 加上 `delta`（wrap）。`ByteView` 是 `{uint8_t* data; size_t size}`。
  baseline 寫成 `for (i < view.size) view.data[i] += delta`：透過 `uint8_t*` 寫入可能 alias 到 `view` 自己的欄位，編譯器不得不每一圈重新載入、也不能向量化。把 `data`、`size` 先拷到區域變數（hoisting）就解開了。至少要比 baseline 快 **4 倍**。

（想一想 `std::vector<uint8_t>` 的 `for (i < v.size()) v[i] = ...` 為什麼也中招，而 `std::vector<uint32_t>` 不會。）

## Stage 4 — Coarsening（§3.3）

`sort_c(data, cmp, cutoff = 16)`：子陣列長度 `<= cutoff` 就改用 insertion sort（要 stable）。`cutoff <= 1` 等同遞迴到底。要比固定的 `fine_grained_sort` baseline 快至少 **1.25 倍**。

## Stage 5 — 只用一份暫存（§3.4）

`sort_m(data, cmp, cutoff = 16)`：`merge` 只把左半段拷到暫存，用原陣列當另一半的來源。測試用替換全域 `operator new` 的方式量尖峰暫存量，不可超過 `(n/2 + 1)` 個元素（可少許超出，例如 16 byte 級的額度）。

寫完想一下：這個寫回的順序為什麼安全？編譯器能幫你做這個最佳化嗎？

## Stage 6 — 重用暫存（§3.5）

`sort_f(data, cmp, cutoff = 16)`：整個排序只配置**一次**暫存，所有遞迴共用（`operator new` 計數必須是 1），尖峰暫存量同 Stage 5。要比固定 baseline 快至少 **1.4 倍**。

自己用 `sample` 看一次 Release 版，把每個 stage 的時間列成表。

## 面試追問

1. 如果 `sort_i` 改成遞迴函式也標 `inline`，編譯器會發生什麼事？內聯遞迴函式的代價是什麼？cachegrind／指令數量得到的訊號和實際時間會矛盾嗎？
2. 用指令數當時間的代理：比較 `sort_a` 的 `-O0` 和 `-O2`，指令數變了多少倍，時間呢？兩者不一致的地方代表什麼？
3. Stage 5 寫回原陣列時，為什麼不會蓋掉尚未讀取的右半部？
