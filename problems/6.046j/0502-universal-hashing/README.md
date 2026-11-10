---
title: Universal Hashing（抵抗 Hash Flooding）
unit: 05 · Skip Lists、Hashing 與 DP（PS5）
source: 6.046J Lecture 8
pdf: materials/6.046j/lectures/lec08.pdf
read: materials/6.046j/lectures/lec08.pdf
kind: code
minutes: 100
---

# 0502 · Universal Hashing

> **實務上為什麼會遇到**：雜湊表一旦被攻擊者知道雜湊函數，就能造出全部撞在同一格的 key，把服務打回 O(n²)。這就是 hash flooding，也是 Python、Rust 預設亂數化雜湊的原因。

## Stage 1 — Dot-Product Family

`DotProductHash(slots, rng)`：`slots` 是質數且 `< 2^31`（否則丟 `std::invalid_argument`）。把 key 看成 `slots` 進位的數字，產生相同位數的隨機向量，`h(k) = a·k mod slots`。

- 要涵蓋全部 64 位元：key 的最高位數字也要參與運算。
- 測試會直接量碰撞率：對固定一對 key，獨立抽很多次函數，碰撞機率不能明顯超過 `1/slots`。

## Stage 2 — 實務上的 Family

`LinearHash(slots, rng)`：`h(k) = ((a·k + b) mod p) mod slots`，`p = 2^61 − 1`，`a ∈ [1, p)`、`b ∈ [0, p)`。`slots ≥ 1`（否則丟 `std::invalid_argument`）。Key 保證 `< p`。

## Stage 3 — 雜湊表

`UniversalHashMap<V>`：用鏈結（每格一個 `std::vector`）解決碰撞。`key ≥ p` 時所有操作丟 `std::invalid_argument`。

| 操作 | 行為 |
| --- | --- |
| `insert(key, value)` | 新增回傳 `true`；已存在時覆寫值並回傳 `false` |
| `find(key)` | 回傳值的指標，不存在回傳 `nullptr` |
| `erase(key)` | 回傳是否真的刪除 |
| `size()`、`empty()`、`bucket_count()` | |
| `longest_chain()` | 最長的格子長度（診斷用，可以 O(m)） |

**load factor 保持 ≤ 1**，滿了就把 bucket 數加倍，並用**新的隨機函數**重新雜湊。沒有這一步，攻擊者一旦試出函數的結構，下一輪重建也不會更安全。測試會用全部相差 `2^40` 倍數的 key 灌爆，檢查最長鏈 `≤ 3 lg n`。

## Stage 4 — 實務延伸：抗攻擊的 Rate Limiter

`RateLimiter(limit, window, seed)`：固定窗口計數器，`limit`、`window` 都要為正，否則丟 `std::invalid_argument`。

- `allow(client, now)`：時間切成長度 `window` 的格子（`floor(now / window)`，負時間也要對），同一格內同一 client 最多放行 `limit` 次，超過回傳 `false`。
- `client_count()`：見過幾個 client。`longest_chain()` 同 Stage 3。

client id 由外部決定，是會被攻擊者控制的輸入。

## 面試追問

1. Universal 的定義是什麼？為什麼 `h(k) = k mod m` 或 `h(k) = (a·k) mod m` 都不算？
2. 雜湊函數的 Dot-product 為什麼要求 `m` 是質數？`LinearHash` 為什麼 `m` 不用是質數，卻要多一個 `b`？
3. Rate limiter 的 map 會一直長大，真實系統你會怎麼處理？固定窗口和 sliding log、GCRA 比，取捨是什麼？
