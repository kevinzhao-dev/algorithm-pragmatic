---
title: Perfect Hashing（FKS 兩層雜湊）
unit: 05 · Skip Lists、Hashing 與 DP（PS5）
source: 6.046J Lecture 8
pdf: materials/6.046j/lectures/lec08.pdf
read: materials/6.046j/lectures/lec08.pdf
kind: code
minutes: 100
---

# 0503 · Perfect Hashing

> **實務上為什麼會遇到**：路由表、語言關鍵字表、唯讀的特徵開關，資料在建好之後就不再變動，但查詢量極大。與其接受平均 O(1)，不如在建表時多花點時間，換來最壞情況只碰兩格記憶體。

用 [`0502`](../0502-universal-hashing/README.md) 的 `LinearHash` 概念（可以把程式複製過來），做一個靜態字典 `PerfectHashMap<V>`。

## Stage 1 — 建表

`PerfectHashMap(items, seed)`：`items` 是 `(key, value)`，key `< kHashPrime`。key 重複或超出範圍時丟 `std::invalid_argument`。**重複的 key 如果沒先擋掉，不管怎麼挑雜湊函數都會一直碰撞，程式就會永遠跑不完。**

第一層把 `n` 個 key 雜湊到 `n` 個桶，第 `j` 桶有 `l_j` 個 key。不斷重抽第一層函數，直到 `Σ l_j² ≤ 4n`。

`total_slots()`：回傳第二層的總格數，也就是 `Σ l_j²`，必須 `≤ 4n`。`seed` 決定所有隨機選擇，同一輸入與種子要得到相同結果。

## Stage 2 — 第二層與查詢

第 `j` 桶配置 `l_j²` 格，為它挑一個第二層函數，不斷重抽直到桶內 **完全沒有碰撞**。

- `find(key)`：回傳值的指標，不存在回傳 `nullptr`，`contains` 同理。不在建表集合裡的 key（包括 `≥ kHashPrime`）都要正確回報不存在，所以格子裡要存 key 才能比對。
- `probes()`：累積檢視過多少格子，**每次查詢不得超過 2**（第一層一格、第二層一格），不能有鏈結。測試會驗每一次。

## Stage 3 — 實務延伸：字串字典

`StaticStringMap<V>(items, seed)`：key 是字串，重複時丟 `std::invalid_argument`。

用隨機底數的多項式雜湊把字串轉成 61 位元指紋，再交給 `PerfectHashMap`。兩點容易忽略：

- 指紋有機率碰撞：建表時發現不同字串指紋相同，就換一個底數重來。
- `find(key)` 要比對完整字串，否則不在字典裡的字串可能撞到指紋而誤判為存在。
- 長度不同的 NUL 字串也要能區分（想想指紋的起始值和位元組的編碼）。

## 面試追問

1. 第一層為什麼 `Σ l_j²` 的期望是 O(n)？用指示變數和 universal 的定義算一次。第二層為什麼一定要 `l_j²` 而不是 `l_j`？
2. 建表的期望時間是多少？隨機重試的次數為什麼可以用幾何分布估計？
3. 如果 key 偶爾要更新，動態完美雜湊（Dietzfelbinger）的思路是什麼？這個結構與 Cuckoo hashing 比，取捨在哪裡？
