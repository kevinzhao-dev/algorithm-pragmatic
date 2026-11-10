---
title: Hash Table 的鎖切分（striping 與 false sharing）
unit: 12 · Deterministic Execution（HW9）
source: 6.172 HW9 §5 Write-up 1–2（hashlock、fair insert）
pdf: materials/6.172/homework/hw09/handout.pdf
read: materials/6.172/lectures/lec16.pdf
kind: code
minutes: 90
sanitizer: tsan
---

# 1212 · Hash Table 的鎖切分

> **實務上為什麼會遇到**：`ConcurrentHashMap`、Redis 的 slot、資料庫的 latch manager 都在做同一件事：一把大鎖太慢，每一列一把鎖又太佔空間，所以把鎖切成 stripe。切完之後，新的問題是 false sharing 和鎖的取得順序。

把 open-addressing table（線性探測）一步步改成多執行緒版本。key 都不為 0，**同一時間不會有兩個 thread 插入同一個 key**。key `k` 的 home slot 是 `k % capacity`（capacity 是 2 的次方），slot `s` 屬於 stripe `s % stripes`（stripes 也是 2 的次方，且 ≤ capacity）。`insert` 在 table 滿時回傳 `false`，相同 key 再插入時覆寫 value。

全部用模板參數 `Mutex`（預設 `std::mutex`）；測試會換成 `_shared/08_dsched.hpp` 的 `dsched::Mutex` 窮舉 interleaving，也會用 `std::mutex` 跑 TSan。

## Stage 1 — 一把大鎖

`CoarseTable(capacity)`：`insert`、`find`（`std::optional<std::uint64_t>`）都拿同一把鎖。

## Stage 2 — 鎖切分（Write-up 1）

`StripedTable(capacity, stripes)`：一個 slot 用它所屬 stripe 的鎖保護。`insert` 從 home slot 開始探測，**一次只持有一把鎖**：鎖住 slot 的 stripe、看一下這格、放開，再換下一格。

另外提供 `stripe_address(i)`（第 i 個 stripe 的鎖的位址）。stripe 陣列要避免 false sharing：相鄰 stripe 的位址必須至少差 64 bytes（`alignas(64)` 或 padding）。作業問：你的 stripe 陣列有 false sharing 嗎？為什麼？

## Stage 3 — 作業的「fair」插入會出事（Write-up 2）

`FairTable(capacity, stripes)`：作業的 `hashtable_insert_fair` 的做法：沿著探測路徑走時，**先拿到下一格的鎖，才放掉目前這格的鎖**（如果下一格屬於不同 stripe，同 stripe 就不用動）。`find` 比照辦理。

你要**照這個規格寫**，然後看測試示範它為什麼有問題：測試會窮舉 interleaving，找出一個真的會死鎖的排程。（這題故意要求你實作有 bug 的版本。）

## Stage 4 — 用全域順序修掉

`OrderedFairTable(capacity, stripes)`：同樣沿路往前走，但**只有當下一格的 stripe 編號比目前大時才拿著舊鎖換新鎖**；一旦下一格的 stripe 編號較小（繞過 stripe 邊界），先放掉舊鎖再拿新鎖。這樣鎖的取得順序有全域單調性，不可能形成環。

## 面試追問

1. 把 `pthread_mutex_t` 緊密排成陣列會有 false sharing，為什麼這對 lock striping 特別傷？Apple Silicon 的 cache line 是多大？
2. Stage 3 的死鎖最少需要幾個 thread？為什麼作業的 16 個 lock、10 個 thread 不見得會發生？Stage 2 的做法完全沒有死鎖，代價是什麼？
3. 相同 key 被兩個 thread 同時 insert 時，Stage 2 會發生什麼事？要怎麼處理？（提示：放棄 hand-over-hand 後，探測途中別人插入的 key 可能出現在你看過的位置之後。）
