---
title: 可重現的除錯流程（seed、replay、shrink）
unit: 12 · Deterministic Execution（HW9）
source: 6.172 HW9 §2–§4 Checkoff 1–3（環境相依、non-deterministic hashtable、replay debugging）
pdf: materials/6.172/homework/hw09/handout.pdf
read: materials/6.172/lectures/lec16.pdf
kind: code
minutes: 70
---

# 1211 · 可重現的除錯流程

> **實務上為什麼會遇到**：線上偶發的 bug 最難的不是修，而是重現。把輸入全部收進一個 seed，失敗就可以穩定重現；再把失敗的操作序列縮到最短，bug 通常一眼就看得出來。property-based testing 和 fuzzer 都是這套流程。

作業的 `hashtable` 用 `malloc` 回傳的指標當 key，再加上 ASLR、環境變數，同一個執行檔每次跑的結果都不一樣，而且裡面藏了一個線性探測沒有繞回開頭的 bug，只在某些 seed 才會出事（`support/buggy_table.hpp` 有一個同樣 bug 的版本，出事時 `corrupted()` 會變 true，不會真的寫出界）。

這題把作業的除錯流程做成程式碼。`Op` 是一個操作：`{Kind::Insert, key, value}` 或 `{Kind::Find, key, 0}`。key 都不為 0。

## Stage 1 — 讓一切只由 seed 決定（Checkoff 1）

`generate_ops(seed, count)`：回傳 `count` 個操作，**只由 seed 決定**（`std::mt19937_64`）：約七成是 `Insert`（新的隨機 key，value 不為 0），其餘是 `Find`（一個之前插入過的 key，沒有就隨便一個）。

`run_ops(table, ops)`：依序執行，`Table` 提供 `insert(key, value)` 和 `find(key)`（回傳 `std::optional<std::uint64_t>`）。回傳每個 `Find` 的結果。

測試會用不同的 heap 狀態跑兩次，確認結果一模一樣。你的程式不可以依賴位址、時間或環境。

## Stage 2 — 找到會壞的 seed（Checkoff 2）

`find_failing_seed(first, last, ops_per_run, fails)`：對 `[first, last)` 由小到大每個 seed 產生操作序列，第一個讓 `fails(ops)` 回傳 `true` 的 seed 就是答案；都不會壞就回傳 `std::nullopt`。

## Stage 3 — Shrink

`shrink_ops(ops, fails)`：`ops` 會讓 `fails` 回傳 `true`。回傳一個仍然會失敗、長度盡量短的子序列（保留相對順序）：結果必須是 **1-minimal**，也就是拿掉任何一個操作後 `fails` 就回傳 `false`。如果傳進來的 `ops` 本來就不會失敗，原樣回傳。

## Stage 4 — 修好它（Checkoff 3）

`ProbingTable<Hash>(capacity)`：open addressing + linear probing，`Hash` 可替換（預設為 multiplicative hash，測試會換成把所有 key 擠到同一格的版本）。

- `insert(key, value)`：回傳是否成功，table 全滿時回傳 `false`；相同 key 就覆寫。
- `find(key)`：回傳 value。
- `size()`。

重點是 `insert` 和 `find` **都要**繞回開頭（作業提醒：同一個 bug 常常出現在另一個長得一樣的函式裡）。

## 面試追問

1. 作業的 `undef.c`：未初始化變數、ASLR、環境變數的長度，分別怎麼影響輸出？要讓程式完全確定性，你會消除哪些輸入？
2. 為什麼只修 `insert`、忘了修 `find`，測試還是可能大多數時候都過？這種 bug 要怎麼在 code review 裡抓到？
3. Stage 3 的 1-minimal 和最短解是一樣的嗎？delta debugging 比你 Stage 3 的做法好在哪裡？
