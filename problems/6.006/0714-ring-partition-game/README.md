---
title: 環狀切分賽局（環形區間 DP 與 minimax）
unit: 07 · Dynamic Programming I（PS7）
source: 6.006 PS7 Problem 7-4
pdf: materials/6.006/psets/ps7/questions.pdf
answer: materials/6.006/psets/ps7/solutions.pdf
read: materials/6.006/lectures/lec16.pdf
kind: code
minutes: 90
---

# 0714 · 環狀切分賽局

> **實務上為什麼會遇到**：環狀資料（consistent hashing ring、circular buffer 的區間規劃）的標準手法是把環拆成線；而兩個會互相搶資源的 agent（或是把最壞情況當成對手）要用 minimax 而不是只看自己的最佳解。這題把兩件事放在一起。

一次只看一關，做完再往下讀。

## Stage 1 — 線形賽局

一條線上有 `n` 片，第 `i` 片的價值是 `values[i]`（可為負），兩人輪流。輪到的人：

- 只剩一片：吃掉它，遊戲結束。
- 否則：在任一相鄰兩片之間切一刀（兩段都非空），自己吃掉其中一段，另一段留給下一個人。

雙方都想讓自己吃到的總和最大。

`std::int64_t line_game_value(std::span<const int> values)`：回傳雙方最佳時先手的總和。空輸入回傳 0。O(n³) 時間（區間和要 O(1) 取得）。

## Stage 2 — 環形賽局

`2n` 片排成一個環，`slices` 依環上順序給定（尺寸須為偶數且 ≥ 2，否則丟 `std::invalid_argument`）。第一手：選任一個起點，吃掉從起點開始**連續的 `n` 片**（直徑切一刀，選一半）。剩下的 `n` 片是連續的一段，接著照 Stage 1 的規則由另一個人先手進行。

`std::int64_t ring_game_value(std::span<const int> slices)`：先手的最佳總和，O(n³) 時間。

## Stage 3 — 上場對打

```cpp
struct Arc { std::size_t start; std::size_t length; };   // start is a position on the ring

class RingGame {
public:
    explicit RingGame(std::span<const int> slices);
    std::int64_t value() const;      // same as ring_game_value
    Arc best_claim(Arc remaining) const;
};
```

`best_claim` 回傳輪到的人應該吃掉的那段（`Arc`，起點落在環上 `[0, 2n)`）：

- `remaining.length == 2n`：完整的環，`remaining.start` 可為 `0`，回傳必須剛好 `n` 片。
- `remaining.length == 1`：回傳 `remaining` 本身。
- 其他：回傳 `remaining` 的前綴或後綴，長度在 `[1, remaining.length − 1]`，這是 Stage 1 的一步。

測試會讓你的 bot 對上所有對手走法：無論對手怎麼走，都要拿到至少 `value()`。

## 面試追問

1. 這題為什麼不需要記「輪到誰」？如果兩人的評分函數不同（不是零和），表要長什麼樣？
2. 環為什麼只要把陣列接在自己後面一次就夠？這個手法在 circular buffer 上還能用在哪裡？
3. 第一手之後只有 `O(n²)` 種局面，為什麼環形的輸入只帶來常數倍的額外成本？如果允許第一手吃掉任意長度，狀態數會變成多少？
