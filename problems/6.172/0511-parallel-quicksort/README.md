---
title: 平行 Quicksort 與 Race 修復
unit: 05 · Fork-Join 與 Reducer（HW4）
source: 6.172 HW4 §2.2–2.3（Checkoff 4–5）
pdf: materials/6.172/homework/hw04/handout.pdf
read: materials/6.172/lectures/lec07.pdf
kind: code
sanitizer: tsan
minutes: 100
---

# 0511 · 平行 Quicksort 與 Race 修復

> **實務上為什麼會遇到**：幾乎所有手刻的平行演算法第一版都有 determinacy race，而且大部分時候「看起來會動」。會用 ThreadSanitizer、看懂報告、指出是哪兩個存取打架，比背任何 lock 的 API 都重要。

先看 race：

```bash
D=problems/6.172/0511-parallel-quicksort
clang++ -std=c++20 -g -O1 -fsanitize=thread -fno-omit-frame-pointer \
  -Iharness -I$D/support $D/lab/race_lab.cpp -o /tmp/race_lab
/tmp/race_lab 2>&1 | head -30
```

讀 TSan 報告（有 race 的程式之後常常直接 crash，那是 race 的後果，報告已經足夠）：哪兩個位址、哪兩個 strand 同時碰到？`support/racy_quicksort.hpp` 的 bug 在哪？想清楚再往下。

這題的演算法都寫成 `template <typename Exec>`，測試會用 `exec::Meter`（`problems/6.172/_shared/01_exec.hpp`：確定性地算 work／span）和 `exec::ForkJoin`（真的平行，由 TSan 把關）。

## Stage 1 — Partition

`partition_around(data, pivot_index, cmp)`：把 `data[pivot_index]` 當 pivot，分割後回傳 pivot 的最終位置 `p`，滿足 `data[p]` 是原 pivot，左邊 `<= pivot`、右邊 `>= pivot`（用 `cmp`）。`std::span<T>`，`T` 可比較。

## Stage 2 — 無 race 的平行 quicksort

`quicksort(exec, data, cmp = {})`：pivot 取「`first`、`middle`、`last` 三者的中位數」（測試靠這個讓 Meter 結果可重現）。遞迴呼叫的兩邊必須**不相交**（不包含 pivot 位置）。每拜訪一個元素比較一次就 `charge(1)`，分割 `n` 個元素的成本是 `n`。

用 `exec::ForkJoin` 排序不會有 TSan 報告；用 `exec::Meter` 看得到 parallelism（work／span）。HW4 Checkoff 5 問的正是這個數字：把它對 `n` 畫一下，為什麼不是線性？

## Stage 3 — Coarsening

`quicksort(exec, data, cmp, cutoff)`：長度 `<= cutoff` 的子陣列用 insertion sort（一樣 charge 比較次數），不再 fork。fork 次數要隨 `cutoff` 明顯下降。

## 面試追問

1. HW4 的 qsort-race 其實是 race 嗎？還是 benign？「看起來對」在 TSan 底下為什麼依然不算？
2. 三數取中在 Meter 上得到 O(n) 的 span 嗎？對什麼輸入會退化？要怎麼讓 span 真的下降？（提示：平行 partition）
3. 這裡的 pivot 分界為什麼不能用「左邊 `<=`、右邊 `>=` 且兩邊都含等於 pivot 的元素」？
