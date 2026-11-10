---
title: Trace-Driven 固定區塊配置與 Autotuning
unit: 08 · Storage Allocation（HW6）
source: 6.172 HW6 Checkoff 1–2；Recitation 6 第 1 題
pdf: materials/6.172/homework/hw06/handout.pdf
answer: materials/6.172/recitations/r6/solutions.pdf
read: materials/6.172/recitations/r6/problems.pdf
kind: code
minutes: 45
---

# 0811 · Trace-Driven 固定區塊配置與 Autotuning

> **實務上為什麼會遇到**：調參數最常見的錯誤，是拿單一 workload 調出一個數字，上線就爆。不管是 buffer pool 大小、allocator 的 size class 還是 cache TTL，流程都一樣：先用 trace 重播，再讓機器搜尋，最後用沒看過的 trace 驗證。

作業的 OpenTuner 在這裡換成你自己寫的搜尋器。`Trace` 是一串 `Alloc(id, size)` / `Free(id)` 操作（對應作業的 trace 檔）。

這一題**不需要真的配置記憶體**，只模擬帳本（真實的 free list 已經在 0801 寫過了）。

## Stage 1 — 重播：固定大小的區塊配置

`TraceReport run_fixed_block(trace, block_size)`：模擬一個所有 block 都是 `block_size` 的配置器。

- `Alloc(id, size)`：`size > block_size` 就算失敗（`failed_allocs` 加一），這個 `id` 之後的 `Free` 要被忽略。否則優先重用閒置 block；沒有閒置時 heap 才長大 `block_size`。
- `Free(id)`：把 block 還回閒置清單（本題不必縮 heap）。
- `peak_live_bytes`：過程中「使用者要求的 bytes」總和的最大值（M）。
- `heap_bytes`：最終的 heap 大小（H）。
- `utilization`（U）：`max(M, 40 KiB) / max(H, 40 KiB)`，M 是上面的 peak，H 是最終的 heap 大小。40 KiB 的下限讓極小的 trace 不被雜訊影響。

另外 `performance_index(U, T, Tlibc, w)` 實作作業講義的公式：`w·U + (1 - w)·min(1, T/Tlibc)`。

## Stage 2 — Autotuner

作業問：OpenTuner 憑什麼知道該選 1024 和 4096？答案是目標函數的形狀。

- `objective(report)`：只要有任何 `failed_allocs` 就是 0，否則就是 `utilization`。
- `autotune(trace, candidates)`：逐一試過所有候選大小，回傳 objective 最高的；同分時選最小的 block。`candidates` 可能亂序，且 objective 不一定單調（別用二分搜）。

## Stage 3 — 避免 overfitting

只用一條 trace 調出來的值，不見得適合別的 trace（Recitation 6 第 1 題）。

`autotune_robust(traces, candidates)`：對每個候選值，取它在所有 trace 上的 **最小** objective，回傳這個最小值最大的候選（同分時選最小的 block）。

## 面試追問

1. 作業的 objective 裡，如果一個 trace 有一次配置超大，整個 block size 就被拉高。實務上你會怎麼處理這種尾端請求？（提示：0802 的 large allocation path）
2. 搜尋空間有兩個以上參數時，暴力枚舉就不可行了。OpenTuner 這類工具用什麼策略？你怎麼判斷結果可信？
3. 為什麼 U 要有 40 KiB 的下限？少了它會有什麼後果？
