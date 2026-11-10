---
title: Text Justification（最佳換行）
unit: 07 · Dynamic Programming I（PS7）
source: 6.006 Recitation 15
pdf: materials/6.006/recitations/rec15.pdf
read: materials/6.006/recitations/rec15.pdf
kind: code
minutes: 60
---

# 0706 · Text Justification

> **實務上為什麼會遇到**：TeX 的段落排版就是這個 DP（Knuth–Plass），瀏覽器預設的 greedy 換行常常留下醜陋的短行。任何「把一串東西切成幾段、每段有成本」的問題（chunk 切割、分批送出）都是同一種 suffix DP。

有 `n` 個字的寬度 `widths[i]`，每行可用寬度 `line_width`。一行放連續的幾個字，字之間隔一個空白，所以用掉 `used = Σ widths + (字數 − 1)`，必須 `used ≤ line_width`。除了**最後一行**，每行的 badness 是 `(line_width − used)³`；最後一行不計分。目標是總 badness 最小。

一次只看一關，做完再往下讀。

## Stage 1 — 最小 badness

`std::optional<std::int64_t> min_badness(std::span<const int> widths, int line_width)`

- 任何一個字比 `line_width` 還寬，回傳 `std::nullopt`；空輸入回傳 0。
- O(n²) 時間（同一行的字寬用 prefix sum，別每次重算；行放不下就該停止嘗試）。
- 範圍：`line_width ≤ 10⁴`、`n ≤ 10⁵`、`widths[i] ≥ 1`。

## Stage 2 — 還原換行位置

`std::optional<std::vector<std::size_t>> line_starts(std::span<const int> widths, int line_width)`：回傳每一行第一個字的索引，遞增，第一個是 `0`（空輸入回傳空 vector）。總 badness 要等於 Stage 1 的答案，同分時任一種都可以。

## Stage 3 — 實務延伸：排版一段文字

`std::optional<std::vector<std::string>> wrap_text(std::string_view text, int line_width)`：以連續的 whitespace（`' '`、`'\t'`、`'\n'`）切字，用 Stage 2 的結果排版，每行的字用一個空白連接。沒有任何字時回傳空 vector；某個字放不下時回傳 `std::nullopt`。

## 面試追問

1. 為什麼 suffix（或 prefix）就夠當子問題？如果 badness 依賴「上一行的長度」（例如相鄰行差太多要扣分），要怎麼擴充狀態？
2. Greedy 在什麼 badness 函數下剛好最佳？為什麼立方次方會讓它失敗？
3. `n` 到百萬時 O(n²) 撐不住。Knuth–Plass 實際上怎麼砍掉候選的斷點？
