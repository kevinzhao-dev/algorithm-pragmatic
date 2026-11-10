---
title: 非同步網路（Max、BFS 校正、節點計數、Bellman-Ford）
unit: 10 · Distributed Algorithms（PS10）
source: 6.046J Lecture 20, Recitation 10
pdf: materials/6.046j/lectures/lec20.pdf
read: materials/6.046j/lectures/lec20.pdf, materials/6.046j/recitations/rec10.pdf
kind: code
minutes: 130
---

# 1002 · 非同步網路

> **實務上為什麼會遇到**：真實網路沒有 lockstep，訊息會亂序、封包會塞車。這題帶你看到：原本同步時好好的演算法，在非同步下會被迫「校正」，最壞情況訊息量爆炸成指數。

全部用 `netsim::AsyncNetwork` 執行：process 實作 `on_start(ctx)` 與 `on_message(port, msg, ctx)`，用 `ctx.send(port, m)` / `ctx.broadcast(m)` 發訊息。沒有回合，訊息何時到由測試給定的 delay 決定（FIFO 不變）。網路在所有 channel 清空時停止。

一次只看一關。

## Stage 1 — Max flooding

`MaxFlood(long long value)`：每個 process 有初始值。開始時廣播自己的值；收到更大的值就更新並廣播。`value()` 回傳目前最大值。收斂後所有 process 的 `value()` 都是全域最大值，總訊息量 ≤ `2·E·n`。

## Stage 2 — 非同步 BFS（校正式）

`AsyncBfs(bool is_root)`：維護 `distance()`（hop 距離，未知為 `kInf`）與 `parent_port()`。root 開始廣播 0；收到 `d` 且 `d+1` 比目前小就更新 parent、廣播新距離（這就是 asynchronous relaxation）。

## Stage 3 — 節點計數（convergecast）

`CountingTree(bool is_root)`：在任意 spanning tree 上算出節點總數（不要求 BFS）。`root_count()` 在 root 上回傳 `n`（完成前回傳 -1）。訊息量 ≤ `4E + n`，要用 parent/nonparent 回應與 done 計數的 convergecast。收到 search 時回覆是否接受；收齊所有回應（知道誰是 child）和所有 child 的 done(k) 之後，往 parent 回報子樹大小。

## Stage 4 — 非同步 Bellman-Ford 與它的代價

`AsyncBellmanFord(bool is_root, std::vector<long long> port_weights)`：`distance()`、`parent_port()`。每次 dist 改善就在**所有** port 廣播新值（與講義相同）。正權重下最終收斂到正確答案，不論訊息怎麼排程。

## 面試追問

1. 非同步 BFS 為什麼最終收斂、但中途的 `parent` 可能很糟？時間上界 `O(diam·n·d)` 的 `n` 從哪來？
2. Stage 4 的指數下界圖長什麼樣？想想每一段如何用「二進位倒數」製造 2^k 個訊息塞在 channel 裡。
3. 想消除校正有什麼實際做法？（PS10-2 的 layered wave、或 synchronizer）代價是什麼？
