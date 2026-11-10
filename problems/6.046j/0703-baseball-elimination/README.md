---
title: Baseball Elimination（聯賽誰還有希望）
unit: 07 · Network Flow（PS7）
source: 6.046J Lecture 14 (Baseball Elimination)
pdf: materials/6.046j/lectures/lec14b.pdf
read: materials/6.046j/lectures/lec14b.pdf
kind: code
minutes: 80
---

# 0703 · Baseball Elimination

> **實務上為什麼會遇到**：排行榜類產品的「魔術數字」、賽季排程（剩餘賽程能否讓某隊奪冠）、任何「給定未完成的比賽，某個結果是否還可能發生」的問題，都能化成 flow 判定。

`Standings` 描述目前戰績：`wins[i]` 是 i 隊已贏場數，`games_left[i][j]` 是 i、j 之間剩餘比賽數（對稱、對角線 0）。只有贏或輸，沒有平手。

某隊被 **eliminated**：不管剩下的比賽結果如何，都不可能讓它贏最多場（第一名並列也算有希望，不算 eliminated）。

一次只看一關。

## Stage 1 — 明顯出局

`bool trivially_eliminated(const Standings& s, int team)`：只比較「`team` 的 wins + 自己剩下比賽全贏」和「其他隊已贏場數」。

## Stage 2 — Flow 判定

`bool is_eliminated(const Standings& s, int team)`：建立 flow network 精確判定。若某隊 `wins[i]` 已經超過 `team` 能達到的上限，直接判定 eliminated（capacity 不能是負的）。

## Stage 3 — 淘汰憑證

`std::vector<int> elimination_certificate(const Standings& s, int team)`：未被 eliminated 回傳空 vector；否則回傳隊伍子集 `R`（不含 `team`，升冪），滿足

```
(wins(R) + games_within(R)) / |R| > wins[team] + remaining(team)
```

其中 `wins(R)` 是 R 中各隊已贏場數總和、`games_within(R)` 是 R 內部剩餘比賽總數、`remaining(team)` 是 `team` 自己剩餘比賽數。這個集合能讓人類一眼看懂：R 內的比賽再怎麼分配，總有一隊贏得比 `team` 多。

## Stage 4 — 實務延伸：賽季追蹤器

```cpp
class SeasonTracker {
public:
    explicit SeasonTracker(Standings initial);
    void record_game(int winner, int loser);        // 一場 winner 勝 loser 的比賽打完
    std::vector<int> contenders() const;            // 目前沒被 eliminated 的隊伍，升冪
    bool is_contender(int team) const;
};
```

`record_game` 必須是剩下賽程中存在的對戰；不必維護增量狀態，O(n) 次 flow 重算即可。

## 面試追問

1. 為什麼 max flow 飽和（saturated）和「所有比賽都能分配給 team 上限以下的贏家」等價？Flow integrality 在這裡扮演什麼角色？
2. Min cut 的另一側為什麼恰好就是淘汰憑證 R？
3. 如果賽制改成贏得分、平手各得 1 分（足球），這個模型要怎麼改？為什麼難度完全不同？
