---
title: Anagram 子字串索引（滑動視窗指紋）
unit: 03 · Hashing & Linear Sorting（PS3）
source: 6.006 PS3 Problem 3-5
pdf: materials/6.006/psets/ps3/questions.pdf
answer: materials/6.006/psets/ps3/solutions.pdf
read: materials/6.006/lectures/lec04.pdf
kind: code
minutes: 60
---

# 0315 · Anagram 子字串索引

> **實務上為什麼會遇到**：基因序列的 k-mer composition、抄襲偵測的 bag-of-words 指紋，都是在長字串上維護「每個長度 k 視窗的字元統計」。關鍵動作只有一個：滑動一格時 O(1) 更新，而不是重算。

所有字串只含小寫英文字母 `a`–`z`。字串 `A` 是 `B` 的 anagram，代表兩者字母的多重集合相同。`B` 在 `A` 中的 anagram substring count，是 `A` 中長度 `|B|` 的**連續**子字串裡，有幾個是 `B` 的 anagram。

## Stage 1 — 索引

`AnagramIndex(text, k)`：`k ≥ 1`（否則丟 `std::invalid_argument`），建構期望 O(|text|)。

- `count(query)`：`query` 的長度必須是 `k`（否則丟 `std::invalid_argument`），回傳 anagram substring count，期望 O(k)。
- `k` 可以比 `text` 長，這時所有查詢的答案都是 0；`k == |text|` 時只有一個視窗。

核心想法：把長度 k 的視窗對應到一個固定大小的「字元頻率表」，把頻率表當 key 存進 hash map。視窗滑動一格時頻率表只改兩個位置，O(1)。

你需要為 `std::array<std::uint32_t, 26>` 寫一個夠好的 hash。

## Stage 2 — 批次查詢

`count_anagram_substrings(text, queries)`：`queries` 中的字串長度都相同（都是 k）。回傳每個查詢的答案，總時間期望 O(|text| + n·k)。`queries` 為空時回傳空 vector。

## Stage 3 — 實務延伸：串流追加

文字是持續進來的 log，索引要能線上更新：

- `append(ch)`：把一個字元接到 `text` 尾端，同時更新索引。均攤期望 O(1)。
- `length()`：目前的文字長度。

append 之後的 `count` 要等於「用完整的文字重建」的結果。為了在 O(1) 時間內知道要從視窗拿掉哪個舊字元，你只需要留最後 `k` 個字元，不需要留整份 `text`；用 O(k) 額外空間（不含 hash map）。

## 面試追問

1. 用「sum of ord」這種更短的 key 取代頻率表，會怎樣？（看看 `ad` 和 `bc`）
2. 把頻率表換成 64-bit 雜湊當 key 省記憶體，代價是什麼？如何估計碰撞機率？
3. 字元集換成完整 Unicode，頻率表還能 O(1) 嗎？什麼資料結構適合？
