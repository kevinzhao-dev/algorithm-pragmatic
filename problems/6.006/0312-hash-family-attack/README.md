---
title: 攻擊 Hash Family（誰能保證碰撞）
unit: 03 · Hashing & Linear Sorting（PS3）
source: 6.006 PS3 Problem 3-2
pdf: materials/6.006/psets/ps3/questions.pdf
answer: materials/6.006/psets/ps3/solutions.pdf
read: materials/6.006/lectures/lec04.pdf, materials/6.006/recitations/rec04.pdf
kind: code
minutes: 50
---

# 0312 · 攻擊 Hash Family

> **實務上為什麼會遇到**：分散式系統的路由 hash 如果是公開的，而 key 又由使用者決定（自選的 session id、URL），攻擊者就能把流量全灌到同一台機器。這題就是當攻擊者：對每個 family 想辦法**在 hash 函數被抽出來之前**挑好一批必定相撞的 key。

服務會在一個 family 中隨機抽一個 hash 函數把 key 打到 `n` 個 bucket，但**key 由攻擊者先選好**。key 是 `[1, u)` 中的正整數，所有函數裡 `n ≥ 2`。原題是證明題，這裡把攻擊寫成程式，答案用窮舉所有函數驗證。

## Stage 1 — Linear family

`h_ab(k) = (a·k + b) mod n`，`a, b ∈ {0, …, n-1}` 且 `a ≠ 0`。

`break_linear_family(n, u)`：回傳兩個相異 key，對 family 裡**每一個**函數都相撞；若 `[1, u)` 裡不存在這樣的 key 對，回傳 `std::nullopt`。

## Stage 2 — Bucket family

`h_a(k) = (⌊k·n/u⌋ + a) mod n`，`a ∈ {0, …, u-1}`。

`break_bucket_family(n, u)`：規格同上。`n` 最大到 `2^31`，`u` 可以到 `2^62`，不可以把 `[1, u)` 掃過一遍（也不可以讓 `k·n` 溢位）。

## Stage 3 — Universal family 的極限

`h_ab(k) = ((a·k + b) mod p) mod n`，`p` 為質數（`2 ≤ n ≤ p < 2^31`），`a ∈ {1, …, p-1}`、`b ∈ {0, …, p-1}`。

這題你保證不了碰撞，只能算機率。`universal_collision_probability(p, n)` 回傳兩個相異 key（`< p`）相撞的**精確機率**，以最簡分數 `Fraction{num, den}` 表示（`0` 的最簡分數是 `{0, 1}`）。

提示：對固定的兩個相異 key，當 `(a, b)` 跑遍 family 時，`(a·k1 + b mod p, a·k2 + b mod p)` 會跑過哪些配對？這個機率會不會隨 key 而變？用 `p = q·n + t` 寫成封閉解，不要枚舉。

## Stage 4 — 實務延伸：Flood key 產生器

- `flood_linear(n, u, count)`：回傳 `count` 個 `[1, u)` 中相異的 key，對 linear family 的**每個**函數都落在同一個 bucket。
- `flood_bucket(n, u, count)`：同上，對 bucket family。

不存在這樣 `count` 個 key 時回傳空 vector（`count == 0` 也回傳空）。這就是實際的 flood 攻擊資料；對照組在測試裡：同一批 key 進 universal family 就散開了。

## 面試追問

1. Stage 3 的機率為什麼是所有 key 對都一樣？對應到「universal」定義的哪一步？
2. 真實系統的 hash 是 SipHash + 隨機 seed，而不是 `(ak+b) mod p`，差別是什麼？
3. 如果攻擊者可以**觀察**自己的 key 被分到哪裡（例如從回應延遲推測），universal hash 的保證還成立嗎？
