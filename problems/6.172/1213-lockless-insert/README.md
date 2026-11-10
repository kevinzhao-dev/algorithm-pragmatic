---
title: 不用鎖的 Hash Table 插入（兩個字、一次 CAS）
unit: 12 · Deterministic Execution（HW9）
source: 6.172 HW9 §6 Write-up 3（lockless hashtable）
pdf: materials/6.172/homework/hw09/handout.pdf
read: materials/6.172/lectures/lec17.pdf
kind: code
minutes: 80
sanitizer: tsan
---

# 1213 · 不用鎖的 Hash Table 插入

> **實務上為什麼會遇到**：硬體通常只保證單一 word 的 CAS，但一筆資料常常有兩個欄位。「先用 CAS 佔位，再發布其餘內容」是 lock-free hash map、ring buffer、log 系統的共同骨架，難的永遠是讀者在兩步之間看到什麼。

一筆 entry 有 key 和 value 兩個 64-bit 欄位，但一次 CAS 只能動一個 word。key 不為 0、value 不為 0；home slot 是 `key % capacity`（capacity 是 2 的次方）；線性探測、不刪除。

用模板參數 `Atomic`（預設 `std::atomic`）宣告所有共享欄位：測試會換成 `_shared/08_dsched.hpp` 的 `dsched::Atomic`，窮舉 interleaving；另外也用真的 `std::atomic` 跑 TSan。

## Stage 1 — 單執行緒語意

`LocklessTable<Atomic>(capacity)`：

- `insert(key, value)`：table 滿時回傳 `false`；key 已存在時覆寫 value。
- `find(key)`：回傳 value，不存在回傳 `std::nullopt`。

## Stage 2 — 併發插入

把 Stage 1 改成無鎖：

- `insert`：用 CAS 把 key 寫進空 slot 來**佔位**，成功之後才寫 value（兩步之間別的 thread 看得到 key、看不到 value）。CAS 失敗就看是不是自己的 key，否則往下探測。
- `find`：key 相符但 value 還是 0，代表寫入尚未完成，回傳 `std::nullopt`。

**不可以有 false negative**：一個 `insert` 回傳之後，**任何**後續的 `find` 都要找得到它，即使探測路徑上有另一個 slot 正停在「佔了位、還沒寫 value」的狀態。作業的 `hashtable_lookup` 遇到 size 為 0 的 slot 就放棄，那段程式碼就有這個問題，你的版本要避開。

同一個 key 不會被兩個 thread 同時 insert。

## Stage 3 — 發布非 atomic 的內容

`PublishedTable<Atomic>(capacity)`：value 換成一個**不是 atomic 的** `std::string`。

- `insert(key, text)`：CAS 佔位，把字串搬進 slot，再用一個 atomic 的 ready 旗標發布。key 已存在時回傳 `false`、不動現有內容；table 滿也回傳 `false`。
- `find(key)`：ready 之後才讀字串，回傳它的副本；否則 `std::nullopt`。

memory order 要自己選。發布端和讀取端如果沒有形成 happens-before，讀 `std::string` 就是 data race。TSan 只看得到這次執行真的發生的存取，測試通過不代表 memory order 一定對，要自己想清楚。

## 面試追問

1. Stage 2 的 CAS 和 value 的 store 各需要什麼 memory order？把 value 改成 relaxed 會壞在哪裡？（作業 Write-up 3）
2. 作業的 `hashtable_lookup` 回傳 NULL 的那條路徑為什麼是 false negative？要修的話，改成繼續探測和改成等待，各有什麼取捨？
3. 如果要支援 delete，這個設計會遇到什麼問題？tombstone 和 Robin Hood 各自怎麼處理？
