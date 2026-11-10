---
title: 避免死鎖（轉帳、哲學家、release-sort-reacquire）
unit: 12 · Deterministic Execution（HW9）
source: 6.172 Lecture 16 Deadlock；Recitation 9 第 1 題
pdf: materials/6.172/lectures/lec16.pdf
answer: materials/6.172/recitations/r9/solutions.pdf
read: materials/6.172/recitations/r9/problems.pdf
kind: code
minutes: 90
sanitizer: tsan
---

# 1202 · 避免死鎖

> **實務上為什麼會遇到**：資料庫的列鎖、檔案系統的 inode 鎖、任何「拿兩把鎖」的轉帳式操作都有死鎖問題。最常見的解法是全域順序，但鎖集合在執行時才知道時就要用 release-sort-reacquire，這也是軟體 transactional memory 的基礎。

這題的所有類別都用模板參數 `Mutex`（預設 `std::mutex`）。測試一部分用真的 `std::mutex` 跑 TSan 壓力測試，一部分換成 `_shared/08_dsched.hpp` 的 `dsched::Mutex`，用確定性排程**窮舉 interleaving**，死鎖會被精確偵測到。你的程式碼只要用 `Mutex` 這個型別參數、不要自己用 sleep 或假設排程順序就好。

## Stage 1 — 轉帳（Recitation 9 第 1 題）

`Bank<Mutex>(balances)`：每個帳戶一把鎖。

- `transfer(from, to, amount)`：鎖住兩個帳戶，餘額不足時回傳 `false` 且不變動，否則搬移並回傳 `true`。`from == to` 時不做事，回傳 `true`（不可以對同一把鎖加鎖兩次）。
- `balance(i)`、`accounts()`：查詢（只有在沒有並行 transfer 時才會被呼叫）。

要在所有 interleaving 下都不會死鎖：鎖的取得順序要有全域順序（固定先鎖較小的索引）。

## Stage 2 — 哲學家用餐（Lecture 16）

`DiningTable<Mutex>(n)`：n 根筷子 n 個人。`eat(philosopher, meal)`：拿到左右兩根筷子（筷子 i 和 (i+1) mod n）之後呼叫 `meal()`，然後放回。不可以死鎖，且左右相鄰的人不可以同時吃。

## Stage 3 — Release-Sort-Reacquire（Lecture 16 Algorithm L）

鎖集合執行時才知道的情況。`LockTable<Mutex>(n)`：

- `table.run(body)`：執行一個交易，`body` 是可重複執行的 callable，參數是 `Transaction&`；`body` 可能被呼叫多次（中途重來），所以不要把副作用放在第一次 `access` 之前。交易結束（或 `body` 丟例外）時放掉所有鎖。
- `txn.access(i)`：這筆交易需要第 i 把鎖。規則照 Algorithm L：

  1. 已持有 `i`：什麼都不做。
  2. 目前持有的鎖都比 `i` 小（或沒持有任何鎖）：直接阻塞式取得 `i`。
  3. 否則先 `try_lock(i)`；成功就拿到了。失敗時：放掉所有比 `i` 大的鎖，阻塞式取得 `i`，再依序把放掉的鎖拿回來，然後**重新開始交易**（`body` 從頭再跑，但已持有的鎖保留）。

每次重來持有的鎖都比上一次多，所以重來的次數有上限。

## 面試追問

1. 死鎖需要哪四個條件？你的轉帳和哲學家分別打破了哪一個？
2. 為什麼 Algorithm L 的第 3 步要「重新開始交易」，而不能拿回鎖之後就繼續？
3. Recitation 9 第 2 題：一個用 CAS 實作的 n 執行緒 mutex 為什麼不需要 Ω(n) 的空間？
