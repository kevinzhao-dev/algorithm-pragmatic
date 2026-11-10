---
title: 插入順序不影響結果的 Hash Table
unit: 12 · Deterministic Execution（HW9）
source: 6.172 HW9 §7 Write-up 4（deterministic hashtable）
pdf: materials/6.172/homework/hw09/handout.pdf
read: materials/6.172/lectures/lec16.pdf
kind: code
minutes: 70
sanitizer: tsan
---

# 1214 · 插入順序不影響結果的 Hash Table

> **實務上為什麼會遇到**：平行程式要能重現，資料結構的最終狀態就不能取決於誰先插入。同樣的性質也讓快照可以逐 byte 比對、讓 build 結果可重現；history-independent 的結構甚至能避免從記憶體佈局洩漏操作順序。

作業的 Write-up 4 只問「怎麼做」，這裡把它做出來。做法是 **prioritized linear probing**：key 本身就是優先權，數值大的優先。

`DeterministicTable<Atomic>(capacity)`：capacity 是 2 的次方，key 不為 0，home slot 是 `key % capacity`，呼叫端保證 key 的數量少於 capacity。所有共享欄位都用模板參數 `Atomic` 宣告（預設 `std::atomic`）。

## Stage 1 — 依優先權排列

`insert(key)`：從 home slot 開始探測，每一格：

- 空的：放進去，結束。
- 同一個 key：什麼都不做，結束。
- 裡面的 key **比手上的小**：兩者交換，改拿被換出來的 key，從**下一格**繼續。
- 裡面的 key 比手上的大：往下一格。

`layout()` 回傳整個 slot 陣列（空格為 0）。不管插入順序如何，同一組 key 的 `layout()` 必須完全相同。

`lookup(key)` 回傳 `std::pair<bool, std::size_t>`：找到與否，以及看過幾個 slot。這個結構的查詢可以提早結束：看到空格、或看到比 `key` 小的 key，就代表 `key` 不存在（想想為什麼）。`contains(key)` 只回傳第一項。

## Stage 2 — 併發插入

把 `insert` 改成 CAS：每一步先讀 slot，決定要放入或交換時用 CAS，失敗就重讀同一格。不需要任何鎖。

只有 insert 的階段內，最終的 `layout()` 必須與序列版本完全相同，**不管執行緒怎麼交錯**。測試會窮舉 interleaving。同一個 key 可能被多個 thread 同時插入，結果裡也只能有一份。

查詢只會在沒有 insert 的階段呼叫。

## Stage 3 — 平行建表

`insert_all(keys)`：用 `harness/forkjoin.hpp` 的 `practice::parallel_for` 平行插入一批 key。不管 worker 數多少，`layout()` 都要和序列版本相同。

## 面試追問

1. 為什麼 `lookup` 看到比自己小的 key 就可以停？這個條件在有交換的時候為什麼仍然成立？
2. Write-up 4：另一種做法是全部插完再排序。跟這題的做法比，各自的取捨是什麼？
3. 這個結構為什麼只能做到 phase-concurrent？如果 insert 和 lookup 同時進行，lookup 可能看到什麼？
