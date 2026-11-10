---
title: UID Ring 上的 Leader Election（LCR 與 Hirschberg-Sinclair）
unit: 10 · Distributed Algorithms（PS10）
source: 6.046J Recitation 10
pdf: materials/6.046j/recitations/rec10.pdf
read: materials/6.046j/recitations/rec10.pdf, materials/6.046j/lectures/lec19.pdf
kind: code
minutes: 100
---

# 1003 · UID Ring 上的 Leader Election

> **實務上為什麼會遇到**：環狀 token ring 拓撲、chord 類 DHT 的 ring 維護，以及任何「節點有 unique ID、要選出一個 leader」的場景。同一個問題，訊息量可以差到 O(n²) vs O(n log n)。

用 `netsim::AsyncNetwork`（unit delay 時行為就是同步回合）。環由 `netsim::Topology::ring(n)` 建立：port 0（`kClockwise`）通往順時針鄰居，port 1（`kCounterclockwise`）通往逆時針鄰居，所有 process 共用這個方向感。每個 process 一開始就知道自己的 UID（互不相同），不知道 n。

一次只看一關。測試一律比對 `stats.messages`。

## Stage 1 — LCR（Le Lann–Chang–Roberts）

`LcrProcess(long long uid)`，`is_leader()`。只用順時針 channel：開始時把自己的 UID 往順時針送；收到的 UID 比自己大就繼續順時針轉送，比自己小就丟掉；收到自己的 UID 就成為 leader。

最壞情況的訊息量是 Θ(n²)。測試會用升冪與降冪排列的環，比對精確訊息數。

## Stage 2 — Hirschberg-Sinclair

`HsProcess(long long uid)`，`is_leader()`。分 phase：第 ℓ 個 phase 向兩個方向各送出 `2^ℓ` 跳的 probe。一路上只有 UID 比途中節點大的 probe 才會被轉送、到距離極限時原路折返；若兩側的回應都回來，進下一個 phase；probe 繞一圈回到自己，就成為 leader。總訊息量 O(n log n)（測試上限 `8·n·(⌈lg n⌉+1)`）。

同一個程式必須在 random delay 下也正確（只有一個 leader、且是最大 UID）。

## 面試追問

1. LCR 的最壞與平均訊息量各是多少？為什麼隨機 UID 排列的平均是 O(n log n)？
2. Hirschberg-Sinclair 為什麼每個 phase 的 active 節點數至少減半？為什麼需要 probe 折返，只往前探不夠嗎？
3. 匿名環（沒有 UID）為什麼不可能（deterministic）？和 Stage 2 的 UID 假設差在哪？（對照 1011）
