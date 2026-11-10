---
title: Set-Associative LRU 快取模擬器
unit: 11 · Caching（HW8）
source: 6.172 Lecture 14（快取結構、3C miss）；Recitation 8 第 1–2 題
pdf: materials/6.172/lectures/lec14.pdf
answer: materials/6.172/recitations/r8/solutions.pdf
read: materials/6.172/recitations/r8/problems.pdf
kind: code
minutes: 120
---

# 1101 · Set-Associative LRU 快取模擬器

> **實務上為什麼會遇到**：cachegrind 在這台機器上沒有，但快取行為是可以模擬的。自己寫一個確定性的快取模型，就能問「這個資料結構換一種 layout 到底差多少 miss」，而且結果可重現。false sharing 的直覺也是從這裡建立的。

這題的模擬器接下來的題目也會用到（它們用 `_shared/08_cache_sim.hpp`，介面相同）。先自己寫一次，之後才知道 miss 數字代表什麼。多層快取與 miss-ratio curve 在 1103。

> ⚠️ **爆雷警告**：`_shared/08_cache_sim.hpp` 是完整的模擬器，等於這題和 1103 大部分的解答，做完之前不要打開。

## Stage 1 — Direct-mapped

`CacheSimulator(capacity_bytes, line_bytes, ways)`：capacity = sets × ways × line。`line_bytes` 必須是 2 的次方，`sets` 也必須是 2 的次方，`ways ≥ 1`，否則建構子丟 `std::invalid_argument`。

- `access(address)`：存取一個位址，命中回傳 `true`。
- `access_range(address, bytes)`：把 `[address, address + bytes)` 涵蓋到的每條 line 各存取一次；`bytes == 0` 什麼都不做。
- `accesses()`、`misses()`、`hits()`：累計次數。`access_range` 跨 3 條 line 就算 3 次 access。

這關只測 `ways == 1`：同一個 set 只有一條 line，line 內的兩個位址要算命中，同 set 不同 line 要互相踢掉。`line_bytes == 1` 或只有一個 set 時，offset 或 index 的位數是 0；位址 `~0` 也是合法的位址，不可以拿任何 tag 值當「空」的記號，`access_range` 碰到位址空間頂端也不可以溢位。

## Stage 2 — Set-associative LRU

一般的 `ways`：每個 set 最多 `ways` 條 line，滿了就踢掉最久沒用的（LRU）。命中也要更新最近使用。`ways == capacity / line` 時就是 fully associative。一次存取的成本只能跟 `ways` 有關，不可以隨總 line 數變慢（`PERF` 會用一百萬個 set 檢查）。

## Stage 3 — 3C 分類

每次 miss 都歸類，寫進三個計數器：

- `cold_misses()`：這條 line 從沒被存取過。
- `conflict_misses()`：不是 cold，但如果是**同樣容量的 fully-associative LRU** 快取就不會 miss。
- `capacity_misses()`：不是 cold，fully-associative 也救不了。

所以你要在內部維護一個同容量的 fully-associative 影子快取。`cold + conflict + capacity == misses` 恆成立。

## Stage 4 — 多核心與 false sharing

`CoherentCaches(cores, capacity_bytes, line_bytes, ways)`：每個 core 一份私有快取（`CacheSimulator` 的規則）。

- `read(core, address)` / `write(core, address)`：命中回傳 `true`，miss 把 line 載入該 core。
- `write` 會讓**其他** core 上同一條 line 失效（無效化 invalidation，把它們清掉）。
- `coherence_misses()`：一個 core 因為先前被別人的 write 無效化，所以 miss 的次數（全部 core 加總，cold 不算）。

Recitation 8 第 2 題的 `sums[chunk_id]` 陣列在測試裡直接驗：相鄰元素在同一條 line 時，coherence miss 會爆量；每個元素自己一條 line 時，一次都不會有。

## 面試追問

1. 同容量下，set-associative 比 direct-mapped 少了哪一種 miss？為什麼 cold miss 不會少？（Recitation 8 第 1 題）
2. 真實 L1 大多是 8-way 而不是 fully associative，取捨是什麼？
3. 把 Stage 4 的計數換成「時間」，false sharing 的代價大概是幾個 cycle？如果一個 thread 只寫一個 `int`，能用什麼手段修？（`alignas`、padding、per-thread 計數加 reducer）
