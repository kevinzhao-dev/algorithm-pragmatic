---
title: Alternating Coin Game（兩人零和區間 DP）
unit: 07 · Dynamic Programming I（PS7）
source: 6.006 Lecture 16
pdf: materials/6.006/lectures/lec16.pdf
read: materials/6.006/lectures/lec16.pdf
kind: code
minutes: 60
---

# 0705 · Alternating Coin Game

> **實務上為什麼會遇到**：minimax 是 AlphaZero 這類 self-play 系統的出發點，能共用的局面就是 transposition table。這題的局面只用「剩下的區間」就能描述，正好拿來練兩人博弈的 DP：對手也在做最好的決定，你的答案要對每種回應都站得住腳。

一排 `n` 枚硬幣，值 `v[0..n-1]`（可為負）。兩人輪流從**目前這一排的最前面或最後面**拿走一枚，直到拿完（輪到你就一定要拿）。雙方都想讓自己的總和最大。

一次只看一關，做完再往下讀。

## Stage 1 — 先手能拿多少

`std::int64_t coin_game_value(std::span<const int> coins)`：雙方都最佳時，先手的總和。O(n²) 時間。空輸入回傳 0。

貪心（每次拿兩端較大的）是錯的。

## Stage 2 — 可以上場對打的策略

```cpp
enum class Side { Front, Back };

class CoinGame {
public:
    explicit CoinGame(std::span<const int> coins);       // O(n^2)
    std::int64_t value() const;                          // same as Stage 1
    Side best_move(std::size_t first, std::size_t last) const;  // O(1)
};
```

`best_move(first, last)`：目前剩下 `coins[first..last]`（包含兩端，`first <= last`）時，輪到的人應該拿哪一端才能達到最佳結果。測試會讓你的 bot 對上窮舉的所有對手走法：無論對手怎麼走，你的 bot 都要拿到至少 `value()`。

## Stage 3 — `coin_game_value` 降到 O(n) 記憶體

依區間長度遞增計算時，只需要長度少一的那一層。Heap 高水位要在 O(n) 以內。把「雙方差值」當成子問題可以省掉 prefix sum，也更簡潔。

## 面試追問

1. 這題是零和，所以一個表就夠。什麼樣的計分規則會讓你必須多記「輪到誰」，甚至兩個人的分數？
2. 長度 `n` 為偶數時，先手其實有個簡單的不敗策略（只算總分勝負）。為什麼它算不出「最多能拿多少」？
3. 把 relation 改寫成差值後，答案怎麼從差值換回總和？整數除法會出問題嗎？
