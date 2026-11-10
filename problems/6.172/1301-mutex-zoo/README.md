---
title: 從 Atomic 做出 Mutex（TTAS、競爭式、Peterson）
unit: 13 · Data Synchronization（HW10）
source: 6.172 Lecture 16 Properties of Mutexes；Lecture 17 Peterson's Algorithm、Mutex Using CAS
pdf: materials/6.172/lectures/lec17.pdf
read: materials/6.172/lectures/lec16.pdf
kind: code
minutes: 80
sanitizer: tsan
---

# 1301 · 從 Atomic 做出 Mutex

> **實務上為什麼會遇到**：大多數時候直接用 `std::mutex`，但要看懂 spinlock 為什麼在高競爭下崩潰、為什麼 futex 要先 spin 一下才睡，就得自己做過一次。Peterson 則是理解 memory ordering 最小的例子：只靠 load 和 store 就能互斥，前提是 sequential consistency。

所有共享欄位用模板參數 `Atomic` 宣告（預設 `std::atomic`）。測試用 `_shared/08_dsched.hpp` 的 `dsched::Atomic` 隨機探索排程，也用真的 `std::atomic` 加一個**非 atomic 的計數器**跑 TSan：你的鎖如果沒有建立 happens-before，TSan 就會報 data race。所以 memory order 不能全部寫 relaxed。

## Stage 1 — Test-and-Test-and-Set

`TtasLock<Atomic>`：

- `lock()`：先用 load 等到鎖看起來是空的，才嘗試 exchange 搶鎖（比一直 exchange 少很多 cache-line 爭奪）。
- `try_lock()`：試一次，成功回傳 `true`。
- `unlock()`：放開。

## Stage 2 — 競爭式 mutex（spin 再 yield）

`CompetitiveLock<Atomic>(spin_limit, yield)`：`yield` 是 `std::function<void()>`，預設是 `std::this_thread::yield`。

`lock()`：鎖可用時直接拿走，不呼叫 `yield`。拿不到時，連續失敗 `spin_limit` 次之後呼叫一次 `yield()`，然後重新開始這一輪計數。`spin_limit == 0` 代表第一次失敗就 yield。

Lecture 16 的分析：spin 的時間只要大致等於一次 context switch，總成本最多是最佳策略的 2 倍。

## Stage 3 — Peterson's algorithm

`PetersonLock<Atomic>`：只支援兩個執行緒，`lock(self)` / `unlock(self)`（`self` 是 0 或 1）。照 Lecture 17：想進入時先舉旗表示自己想要，再把優先權讓給對方，對方也想要而且輪到對方時才等待。

只用 load 和 store，不可以用 exchange 或 CAS。memory order 全部用 `seq_cst`。

## 面試追問

1. 為什麼 Peterson 在 x86 上如果不用 fence 會壞？哪個重排（TSO 唯一允許的那種）造成的？（Lecture 17）
2. 比較 test-and-set 和 test-and-test-and-set 在 16 個 core 競爭時的 cache-coherence 流量。
3. 為什麼實務上幾乎沒有人用 Peterson？它需要什麼硬體保證、擴展到 n 個執行緒要付什麼代價？Recitation 9 第 5、6 題的 memory fence 敘述各是對是錯？
