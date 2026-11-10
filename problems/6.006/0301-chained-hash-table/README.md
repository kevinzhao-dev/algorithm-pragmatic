---
title: Chained Hash Table（universal hashing）
unit: 03 · Hashing & Linear Sorting（PS3）
source: 6.006 Lecture 4, Recitation 4
pdf: materials/6.006/recitations/rec04.pdf
read: materials/6.006/lectures/lec04.pdf
kind: code
minutes: 75
---

# 0301 · Chained Hash Table

> **實務上為什麼會遇到**：Python 的 dict、C++ 的 `unordered_map`、資料庫的 hash join，骨子裡都是這個結構。2011 年的 HashDoS 就是攻擊者挑一批 key 把 chain 灌成 linked list，打爆了一堆語言的預設實作；隨機化的 hash family 正是對策。

自己實作 universal hash 與 chaining hash table，不准用 `std::unordered_map`（chain 可以用 `std::forward_list` 或自己的 linked list）。一次只看一關。

## Stage 1 — Universal hash

`UniversalHash(a, b)` 代表 family 中的一個成員 `h(k) = ((a*k + b) mod p) mod m`，其中 `p = kPrime = 2^61 - 1`。

- 建構時檢查 `1 ≤ a < p`、`0 ≤ b < p`，否則丟 `std::invalid_argument`。
- `operator()(key, buckets)`：`buckets ≥ 1`，key 是任意 `uint64_t`，回傳值要 `< buckets`。

`a*k` 會超過 64 bits，用 `unsigned __int128`。對 Mersenne prime 取餘數有不用 `%` 的快速算法，值得想一下。

## Stage 2 — 基本操作

`ChainedHashMap<K, V, Prehash>`（`Prehash` 把 key 變成 `uint64_t`，預設 `std::hash<K>`）：

| 操作 | 行為 | 複雜度 |
| --- | --- | --- |
| `explicit ChainedHashMap(seed, prehash)` | 用 `seed` 決定 hash 參數 | O(1) |
| `insert(k, v)` | 新 key 回傳 `true`；已存在則**覆蓋**並回傳 `false` | 期望 O(1) |
| `find(k)` | 指向 value 的指標，沒有時 `nullptr` | 期望 O(1) |
| `erase(k)` | 回傳被移除的 value，沒有時 `std::nullopt` | 期望 O(1) |
| `size()` / `empty()` | | O(1) |

不可洩漏：解構時要把 key、value 都釋放掉。

## Stage 3 — 動態調整大小

- `bucket_count()`：目前 bucket 數。`load_factor()`：`size / bucket_count`。
- 任何時刻都要滿足 `size() ≤ bucket_count()`，且 `bucket_count() ≤ 8 × size() + 64`（表要會縮）。
- **成長和縮小的門檻不能相同**：key 數量在門檻附近來回時，不能每次操作都 rehash。

`insert` / `erase` 要均攤期望 O(1)。

## Stage 4 — 對抗性輸入與所有權

- `bucket_of(k)`：key 目前落在哪個 bucket，`< bucket_count()`。
- `max_chain_length()`：目前最長的 chain。
- 不同 seed 要得到不同的分佈。把只有高位不同的 key（例如全是 `2^20` 的倍數）塞進去，最長 chain 要維持很短。
- 如果 `Prehash` 把所有 key 都映到同一個值，還是要正確運作（只是變慢）。
- move constructor / move assignment：要 `noexcept`，被移走的表變空，之後仍可再插入。

## 面試追問

1. 期望 O(1) 的「期望」是對什麼取的？對輸入嗎？攻擊者知道你用的 family 時還成立嗎？
2. `erase` 也要縮表，為什麼？若門檻設在 `n < m/2` 會發生什麼？
3. `prehash` 把字串打成 64 bits 後才進 universal hash：為什麼這不算破壞 universal 的保證？（或它其實破壞了什麼？）
