---
title: 確定性排程器（把競態變成可重現的 bug）
unit: 12 · Deterministic Execution（HW9）
source: 6.172 Lecture 16 Determinism；HW9 §4 replay 概念；Recitation 9 第 4 題
pdf: materials/6.172/lectures/lec16.pdf
answer: materials/6.172/recitations/r9/solutions.pdf
read: materials/6.172/recitations/r9/problems.pdf
kind: code
minutes: 130
---

# 1201 · 確定性排程器

> **實務上為什麼會遇到**：race condition 最痛苦的地方是不能重現。Folly 的 `DeterministicSchedule`、Rust 的 `loom`、Windows 的 CHESS 都是同一個想法：讓所有執行緒輪流跑，由一個 scheduler 決定誰下一個，於是 interleaving 變成一串可以記錄、重播、窮舉的決定。

這題自己做一個最小版本。之後 1211 以下的並行題都會用同樣的技巧（用 `_shared/08_dsched.hpp` 裡的完整版）來讓測試完全確定性。

> ⚠️ **爆雷警告**：`_shared/08_dsched.hpp` 是這題的完整版解答，做完之前不要打開。

**模型**：每個 body 跑在自己的 `std::thread`，但任一時刻只有一個在跑。body 裡呼叫 `yield_point()` 時交出控制權。**決策點**有兩種：開始時選第一個 thread，以及每次 `yield_point()`、thread 結束或阻塞時，只要還有**兩個以上**可以跑的 thread，就要做一次決定。`schedule` 記下每個決策選了誰。不是決策點時（只剩一個可跑）就不記。

## Stage 1 — 指定 schedule

`run_schedule(bodies, choices)`：依序用 `choices` 的 id 做決策；用完之後每次選最小的 id。`choices` 指到不能跑的 thread 時丟 `std::invalid_argument`。回傳 `RunOutcome`。

body 內 throw 的例外要在 `run_schedule` 回到呼叫者時原樣重丟（`PRACTICE_TODO` 就是例外）。

## Stage 2 — 隨機與重播

- `run_random(bodies, seed)`：每個決策在可跑的 thread 中均勻亂選（`std::mt19937_64`，同一個 seed 必須得到相同的 `schedule`）。
- `replay_schedule(bodies, schedule)`：照著記錄重跑一次，結果必須完全相同。

這就是作業 Replay debugging 的重點：一旦 failure 可以被重播，就可以慢慢查。

## Stage 3 — 阻塞與死鎖偵測

`SchedMutex`：`lock()` 時鎖若是空的就直接拿走（這不是決策點）；鎖被別人拿著時把呼叫者標成**阻塞**（不算可跑），控制權交給其他 thread，鎖放開時喚醒等它的 thread（重新競爭）。`unlock()` 放開鎖之後是一個決策點。`try_lock()` 立刻回傳，拿不到就是 `false`。

如果沒有任何 thread 可以跑、卻還有 thread 沒結束，就是**死鎖**：`RunOutcome::deadlocked = true`，所有 thread 都要結束（用內部例外把阻塞的 thread 收掉，不可以 hang 住）。

## Stage 4 — 窮舉所有 interleaving

`explore_all(make)`：`make()` 每次回傳新的 `Scenario{bodies, check}`。對**每一種**可能的 schedule 都跑一次（DFS 枚舉決策樹），統計 `ExploreSummary{runs, failures, deadlocks, bad_schedule}`：`check` 回傳 `false` 算一次 failure，第一次失敗的 schedule 要記在 `bad_schedule`。

兩個 body、各兩段（中間一個 yield），應該恰好有 C(4,2) = 6 種 schedule；測試也會窮舉 Recitation 9 第 4 題的所有可能結果。

## 面試追問

1. 這個模型假設 sequential consistency。Recitation 9 第 4 題在 x86 上結果一樣嗎？什麼情況下 weak memory 會多出新結果？
2. 窮舉的 schedule 數量會以多快的速度成長？CHESS 的 preemption bounding 為什麼有用？
3. Lecture 16 說「決定性」程式比較好除錯。Recitation 9 第 3 題為什麼是 false：「每個並行存取都用同一個 mutex 保護，序列時有決定性的程式就仍有決定性」？
