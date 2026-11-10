---
title: Hash Practice（程式化手算）
unit: 03 · Hashing & Linear Sorting（PS3）
source: 6.006 PS3 Problem 3-1
pdf: materials/6.006/psets/ps3/questions.pdf
answer: materials/6.006/psets/ps3/solutions.pdf
read: materials/6.006/lectures/lec04.pdf, materials/6.006/recitations/rec04.pdf
kind: code
minutes: 30
---

# 0311 · Hash Practice

> **實務上為什麼會遇到**：`gperf` 和編譯器的關鍵字表，就是在固定的一小組 key 上搜尋一個完全沒有碰撞的 hash 函數，換來一次 probe 就查完。

原題是手算兩題；這裡把「手算的步驟」寫成程式，算完後建議把 PDF 第 1 頁的數字丟進去跑，對照 `answer` 的 Problem 3-1。

## Stage 1 — 畫出 Chaining 的結果

`chain_layout(keys, buckets, hash)`：依序把 `keys` 用 `hash(k)` 放進 `buckets` 條 chain，回傳每條 chain 的內容。

- 同一 bucket 中**越晚插入的排在越後面**（跟原題要求的相同）。
- 回傳的 vector 長度一定是 `buckets`，沒有 key 的 bucket 是空 vector。`hash(k)` 回傳值保證在 `[0, buckets)`。

## Stage 2 — 找沒有碰撞的模數

`smallest_injective_modulus(keys, a, b, buckets, limit)`：在 `1..=limit` 中找最小的正整數 `c`，使得函數 `h(k) = ((a·k + b) mod c) mod buckets` 在 `keys` 上沒有碰撞（單射）。找不到回傳 `std::nullopt`。

- `keys` 彼此相異。
- `keys` 的數量比 `buckets` 多時答案一定是 `nullopt`，而且 `limit` 可能大到無法掃（鴿籠原理）。
- 所有 key 都會非負，`a·k + b` 不會溢位 `int64_t`。

## Stage 3 — 實務延伸：Perfect Hash

`PerfectHashSet(keys)`：給定一組固定的 key（`keys.size() ≤ 64`，彼此相異，非負），建構後查詢不需要 probing。

- `table_size()`：slot 數，不超過 `4 × keys.size() + 1`。
- `slot_of(k)`：`k` 對應的 slot，`< table_size()`。對所有存進去的 key，slot 兩兩不同。
- `contains(k)`：只看一個 slot 回答，不存在的 key 也要答對。

建構時可以用 Stage 2 去搜：`buckets = 4n + 1`，固定 `b = 0`，輪流試 `a`，直到找到沒有碰撞的 `(a, c)`。

## 面試追問

1. 為什麼 Stage 2 的「最小 c」要從 1 試起？`c < buckets` 時 hash 值最多只有幾種？
2. 隨機函數把 n 個 key 丟進 m 格沒有碰撞的機率大約是多少？為什麼 `m = 4n` 時 n 才幾十就要試幾千次？
3. Perfect hash 怎麼支援不存在的 key 查詢？為什麼 slot 裡要存 key 本身？
