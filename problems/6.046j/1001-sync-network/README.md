---
title: 同步網路演算法（Leader、BFS Tree、MIS、Bellman-Ford）
unit: 10 · Distributed Algorithms（PS10）
source: 6.046J Lecture 19, Recitation 10
pdf: materials/6.046j/lectures/lec19.pdf
read: materials/6.046j/lectures/lec19.pdf, materials/6.046j/recitations/rec10.pdf
kind: code
minutes: 140
---

# 1001 · 同步網路演算法

> **實務上為什麼會遇到**：分散式資料庫的 leader、資料中心網路的 spanning tree、把 token 指派給獨立集合的工作排程，都建立在這幾個基本協定上。同步模型把時序的混亂拿掉，讓你先專心看「每一輪傳什麼、誰可以判斷結束」。

全部用 `_shared/06_net_sim.hpp` 的 `netsim::SyncNetwork` 執行，**不要自己寫訊息傳遞**。協定寫成 class：宣告 `using Message = ...`，實作 `send(round, outbox)`、`receive(round, inbox)`、`halted()`（介面見標頭）。process 只知道自己的 port 數，不知道 vertex 編號。

一次只看一關。測試會驗證結果**與訊息量**（`stats.messages`）。

## Stage 1 — Clique 上的 Leader Election

`CliqueLeader(std::int64_t uid)`，UID 互不相同。第一輪廣播 UID，收到全部 UID 後，最大 UID 當 leader（`is_leader()`）。一輪結束、`halted()` 為 true。

## Stage 2 — BFS Spanning Tree 與 Convergecast

`BfsTree(bool is_root)`，連通圖、不知道 n。`search` 訊息從 root 往外擴散。收到第一個 search 時選擇 parent、回覆 `parent`，其餘 search 回覆 `nonparent`。**不要用 UID**。

- `parent_port()`：parent 的 port（root 或尚未加入時為 -1）。
- `children_ports()`：回傳子節點的 ports，升冪。
- `depth()`：root 為 0。
- convergecast：葉節點開始往上送 `done`，root 收齊後 `finished()` 回傳 true，所有節點在送出（或收齊）`done` 之後 `halted()`。

訊息總量必須 ≤ `4E + n`。回合數 O(diam)。

## Stage 3 — Luby 的 Maximal Independent Set

`Luby(std::uint64_t seed, std::uint64_t n_upper)`。每個 phase 兩輪：第一輪 active 節點廣播 `1..n⁵`（用 `n_upper`）內的隨機值，比所有鄰居都大的節點加入 MIS；第二輪加入者廣播通知，收到通知者退出。決定了的節點 halt。`decided()`、`in_mis()`。

## Stage 4 — Synchronous Bellman-Ford

`SyncBellmanFord(bool is_root, std::vector<long long> port_weights, int rounds)`：`port_weights[p]` 是 port p 那條邊的權重（非負）。每一輪對所有 port 廣播目前距離，收到後 relax。跑滿 `rounds` 輪就 halt。`distance()` 回傳距離（不可達為 `kInf`，標頭定義）、`parent_port()`。

## 面試追問

1. 為什麼 BFS tree 這樣做一定是 shortest path tree？如果 search 訊息的延遲不同（不同步）還成立嗎？
2. Luby 演算法期望幾個 phase 結束？每個 phase 砍掉一半 edge 的直覺是什麼？
3. Convergecast 的 `done` 為什麼要等「所有鄰居都回應」才能往上送？少了這個條件會發生什麼事？
