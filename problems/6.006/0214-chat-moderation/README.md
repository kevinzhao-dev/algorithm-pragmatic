---
title: 聊天室與封鎖（組合資料結構）
unit: 02 · Sorting & Recurrences（PS2）
source: 6.006 PS2 Problem 2-4
pdf: materials/6.006/psets/ps2/questions.pdf
answer: materials/6.006/psets/ps2/solutions.pdf
read: materials/6.006/lectures/lec02.pdf, materials/6.006/recitations/rec03.pdf
kind: code
minutes: 80
---

# 0214 · 聊天室與封鎖

> **實務上為什麼會遇到**：直播聊天室、訊息流的 moderation：把某個使用者禁言時，他過去所有的發言要立刻消失，但不能為了這件事去掃整個訊息流。這類題目的重點是把幾個簡單的結構組合起來，讓每個操作只碰到自己該碰的部分。

原題：一個聊天室有 `n` 位 viewer，每一個操作都要 **worst-case** 複雜度，不可靠 hash 的 expected O(1)（`Viewer` 只保證有 `<` 和 `==`；測試用 `Counted` 檢查比較次數）。你要實作 `ChatRoom<Viewer, Message>`。

## Stage 1 — 發言與讀取

| 操作 | 行為 | 複雜度 |
| --- | --- | --- |
| `explicit ChatRoom(viewers)` | 建立，viewer 不重複（重複時丟 `std::invalid_argument`） | O(n log n) |
| `send(v, m)` | 使用者 `v` 發言。`v` 不存在時回傳 `false`；成功回傳 `true` | O(log n) |
| `recent(k)` | 回傳最新的 `k` 則訊息（不足 `k` 則時回傳全部），**最新的在最前面** | O(k) |
| `message_count()` | 目前可見的訊息數量 | O(1) |

`recent(k)` 的時間只能跟 `k` 有關，和總訊息數無關。

## Stage 2 — 封鎖

- `ban(v)`：封鎖 `v`，並移除 `v` 過去所有的訊息。回傳是否成功（`v` 不存在或已被封鎖時回傳 `false`）。複雜度 **O(n_v + log n)**，`n_v` 是 `v` 目前還可見的訊息數，不可以掃描其他人的訊息。
- `is_banned(v)`：`v` 是否已被封鎖，O(log n)。
- 被封鎖的 viewer 再 `send` 時回傳 `false`，也不會出現新訊息。

`recent(k)` 依然要 O(k)：做法上不可以留下「墓碑」讓 `recent` 需要略過被刪除的訊息（測試會在大量封鎖之後檢查）。

## Stage 3 — 實務延伸：每人保留上限

建構子多一個參數 `ChatRoom(viewers, per_viewer_limit)`（`≥ 1`，否則丟 `std::invalid_argument`）：每位 viewer **只保留最新的 `per_viewer_limit` 則**訊息。`send` 超過上限時，移除該 viewer 自己最舊的那一則，`send` 仍要 O(log n)。只傳一個參數時代表沒有上限。

## 面試追問

1. 為什麼需要「viewer → 他的訊息節點們」這層索引？只用一個雙向串列會差在哪個操作？
2. 為什麼要 worst-case 而不是 expected 複雜度時，sorted array（或 `std::map`）比 `unordered_map` 合適？這在實際系統裡有多重要？
3. 如果 `recent` 改成「某個 viewer 最新的 k 則」，你的結構要怎麼延伸？
4. 多執行緒環境下，`send` 和 `ban` 會競爭什麼？你會怎麼切鎖？
