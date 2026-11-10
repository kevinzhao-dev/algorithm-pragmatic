---
title: 最長遞增區段的計數（串流版）
unit: 00 · Warm-up（PS0）
source: 6.006 PS0 Problem 0-6
pdf: materials/6.006/psets/ps0/questions.pdf
answer: materials/6.006/psets/ps0/solutions.pdf
kind: code
minutes: 35
---

# 0016 · 最長遞增區段的計數

> **實務上為什麼會遇到**：監控系統要找「latency 連續上升」最長維持了多久、發生過幾次，再決定要不要告警。資料是不斷流進來的時間序列，不可能把歷史全部留著重新掃。

原題：給定一個整數陣列，找出**嚴格遞增的連續子陣列**中最長的長度，回傳具有這個最長長度的子陣列有幾個（細節見 PDF 第 1 頁）。空陣列回傳 0。

## Stage 1 — 陣列版

`count_longest_runs<T>(values)`：`values` 是 `std::span<const T>`，一次掃描，**O(n)**。空陣列回傳 0。

相鄰元素相等時，遞增區段就斷開（嚴格遞增）。

## Stage 2 — 串流版

資料是一筆一筆進來的。`RunTracker<T, Less>`（比較器預設 `std::less<T>`）：

- `push(value)`：**O(1)**，且不可用任何額外的 heap 記憶體（不能把資料存起來）。
- `longest()`：目前為止最長的嚴格遞增區段長度，還沒有資料時為 0。
- `count()`：長度等於 `longest()` 的區段有幾個，還沒有資料時為 0。
- `current()`：以最新一筆資料結尾的遞增區段長度，還沒有資料時為 0。

`Less` 決定「遞增」的意義：用 `std::greater<>` 就變成追蹤嚴格遞減的區段。

## Stage 3 — 實務延伸：連續上升告警

`StreakAlarm<T, Less>`：

- `explicit StreakAlarm(std::size_t threshold)`：`threshold ≥ 1`，否則丟 `std::invalid_argument`。
- `observe(value)`：回傳 `true` 代表**現在**應該告警，也就是目前的遞增區段**剛好**達到 `threshold` 這個長度。同一段區段繼續增長不會重複告警；區段斷掉之後的新區段可以再次告警。

O(1)、不配置記憶體。

## 面試追問

1. 為什麼一次掃描就夠，不需要枚舉所有子陣列？寫出你的 loop invariant：跑完第 i 個元素時，你維護的三個變數各代表什麼？
2. 如果「連續上升」改成「最近 k 筆都比各自前一筆大」的滑動視窗告警，你的狀態要怎麼改？
3. 這題和「最長遞增**子序列**」（不需要連續）為什麼難度差這麼多？
