---
title: 多層快取與 Miss-Ratio Curve
unit: 11 · Caching（HW8）
source: 6.172 Lecture 14（多層快取）；取代 HW2 §2.2 的 Cachegrind
pdf: materials/6.172/lectures/lec14.pdf
read: materials/6.172/homework/hw02/handout.pdf
kind: code
minutes: 80
---

# 1103 · 多層快取與 Miss-Ratio Curve

> **實務上為什麼會遇到**：決定 L2 要多大、Redis 要給多少記憶體、CDN 每個節點要放多少物件，問的都是同一件事：「容量加倍，miss 會少多少？」一個一個容量重跑模擬太慢，stack distance 一次掃過 trace 就能畫出整條曲線。

接續 1101。每一層快取直接用 `_shared/08_cache_sim.hpp` 的 `practice::cachesim::Cache`（介面跟你在 1101 寫的幾乎一樣：`access`、`access_range`、`accesses()`、`misses()`），這題專心在「層」與「曲線」。

## Stage 1 — 兩層（或更多層）快取與多 byte 存取

`CacheHierarchy(levels)`：`levels[0]` 最接近 CPU，每一層是 `LevelConfig{capacity_bytes, line_bytes, ways}`。`levels` 是空的，或各層的 `line_bytes` 不一樣，丟 `std::invalid_argument`（不合法的單層參數則由 `Cache` 丟出）。

- `access(address)`：從第 0 層開始查，**只有 miss 才往下一層**。回傳滿足這次存取的層數索引；全部 miss（打到記憶體）回傳 `levels.size()`。miss 的層都要把這條 line 載入。
- `access_range(address, bytes)`：`[address, address + bytes)` 涵蓋的每條 line 各做一次 `access`，回傳其中打到記憶體的 line 數。`bytes == 0` 什麼都不做。跨 line 邊界的 8-byte 存取會碰到兩條 line；`address + bytes - 1` 剛好是位址空間最頂端時也不可以溢位。
- `accesses(level)`、`misses(level)`：各層看到的存取與 miss 次數。下層只看得到上層的 miss。

## Stage 2 — Miss-Ratio Curve

`miss_counts(addresses, line_bytes, capacities)`：對 fully-associative LRU，回傳與 `capacities` 同長度的陣列，第 i 項是容量 `capacities[i]` 條 line 時的 miss 數。`capacities` 順序不定，容量 0 代表每次都 miss。

做法：對每次存取算 **stack distance**：上一次碰這條 line 之後，碰過多少條**不同**的 line（第一次出現則為無限大）。容量 `C` 的 LRU 恰好在距離 `≥ C` 時 miss。

總時間要 **O(n log n)**（n 是存取次數），不可以對每個容量各模擬一次。提示：在「每條 line 最後一次出現的位置」上標 1，兩次出現之間的區間和就是距離，Fenwick tree 剛好做這件事。

## 面試追問

1. Stack distance 能精確預測 fully-associative LRU，對 set-associative 只是近似，為什麼？
2. 真實的 L2 通常是 inclusive 或 exclusive，這題的模型是哪一種？差別會反映在哪個計數上？
3. 為什麼 `-O2` 的程式 instruction count 變少，cachegrind 量到的 miss rate 卻可能變高？用指令數當成本模型有什麼優缺點？
