---
title: Open Addressing（linear probing 與刪除）
unit: 03 · Hashing & Linear Sorting（PS3）
source: 6.006 Lecture 4, Recitation 4
pdf: materials/6.006/recitations/rec04.pdf
read: materials/6.006/lectures/lec04.pdf
kind: code
minutes: 70
---

# 0302 · Open Addressing

> **實務上為什麼會遇到**：Redis、Google 的 `absl::flat_hash_map`、V8 的 property table 都用 open addressing，因為一條連續陣列比 chaining 的節點對 cache 友善得多。代價全在刪除：一不小心就把還在表裡的 key 弄丟，或讓 tombstone 慢慢拖垮整張表。

用 linear probing 實作 `OpenAddressingSet<K, Hash>`：所有資料都放在一條 slots 陣列裡，碰撞時往下一格找。不准用 `std::unordered_set`。

## Stage 1 — 插入與查詢

| 操作 | 行為 |
| --- | --- |
| `insert(key)` | 新 key 回傳 `true`，已存在回傳 `false` |
| `contains(key)` | 是否存在 |
| `size()` | 目前的 key 數 |
| `capacity()` | slots 的數量；隨時要 `size() < capacity()`（probe 才找得到空格停下來） |

- `Hash` 可能很爛。libc++ 的 `std::hash<int>` 就是 identity，如果 capacity 是 2 的冪次又直接取低位，所有 `1 << 20` 的倍數都會擠在同一格；表內部要再 **mix** 一次。
- 太滿時要 rehash，隨便你選門檻，但要均攤期望 O(1)。

## Stage 2 — 刪除（tombstone）

`erase(key)`：存在則移除並回傳 `true`，否則回傳 `false`。

這關的陷阱：

- 不能直接把 slot 清空：同一條 probe 路徑後面的 key 會找不到。
- `insert` 遇到 tombstone 不能馬上停，要繼續往後確認 key 沒有出現在更後面，否則同一個 key 會被放兩次。
- 確認不存在後，新 key 應該放回路徑上遇到的第一個 tombstone。

## Stage 3 — Tombstone 不能累積

- key 不斷進出時，tombstone 不能無限累積，否則「查一個不存在的 key」會掃過整張表。解法自選：把 tombstone 算進 load 並定期重建，或改用 backward-shift deletion。
- 刪到很少 key 時要縮表：`capacity() ≤ 8 × size() + 64`。
- 重建不能遺失任何 key。

## Stage 4 — 實務延伸：去重視窗

用你的 set 做 `DedupWindow<K>`，訊息佇列重送時用來過濾重複訊息：

- `explicit DedupWindow(ttl)`：`ttl ≥ 1`，否則丟 `std::invalid_argument`。
- `offer(key, now)`：`now` 不遞減。key 在 `(now - ttl, now]` 內被**接受過**則回傳 `false`（重複，不影響計時）；否則接受並回傳 `true`，並從 `now` 開始算 `ttl`（跟 Redis `SET key NX EX ttl` 同語意）。
- `size()`：視窗內仍記得的 key 數。過期的 key 要真的被刪掉，記憶體才不會無限長。

## 面試追問

1. Linear probing 的 cluster 為什麼會越滾越大？double hashing、Robin Hood 各自怎麼解決？
2. Tombstone 跟 backward-shift deletion 各有什麼取捨？
3. 為什麼 open addressing 的 load factor 上限比 chaining 低？load 逼近 1 時查詢成本會怎麼變？
