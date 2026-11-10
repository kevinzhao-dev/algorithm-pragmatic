---
title: 電動車路線規劃（電量也是座標的一部分）
unit: 06 · Weighted Shortest Paths（PS6）
source: 6.006 PS6 Problem 6-5
pdf: materials/6.006/psets/ps6/questions.pdf
answer: materials/6.006/psets/ps6/solutions.pdf
read: materials/6.006/lectures/lec13.pdf, materials/6.006/recitations/rec13.pdf
kind: code
minutes: 110
---

# 0615 · 電動車路線規劃

> **實務上為什麼會遇到**：電動車導航、無人機航線、配額受限的 API 呼叫鏈，全都是「最短路徑，但手上的資源會影響接下來能走哪裡」。真正要練的是：什麼東西應該算進搜尋的狀態裡。

一輛電動車要從 `source` 開到 `target`，時間越短越好。

```cpp
struct Road {
    int a;
    int b;
    std::int64_t minutes;  // >= 1, drivable in both directions
};

struct RoadMap {
    std::vector<int> elevation;     // one per intersection, positive
    std::vector<char> has_charger;  // 0 or 1, one per intersection
    std::vector<Road> roads;
    int capacity;                   // battery capacity, >= 1 unit
    std::int64_t minutes_per_unit;  // >= 1
};
```

規則：

- 從 `source` 出發時電量是滿的（`capacity` 單位）。
- 從路口 `x` 開上一條路到 `y`：時間 `minutes`，若 `y` 比 `x` 高則消耗 `elevation[y] - elevation[x]` 單位電量，下坡和平路都不耗電。
- 電量**在任何時刻都必須大於 0**（到達路口時也一樣）。
- 在有充電站的路口（包含 `source`、`target`）可以充任意整數單位的電，每單位花 `minutes_per_unit` 分鐘，總電量不能超過 `capacity`。

`source` 或 `target` 不存在時丟 `std::out_of_range`。令 `g = capacity`、`n` 為路口數、`r` 為道路數。一次只看一關，做完再往下讀。

## Stage 1 — 沒有充電站

`fastest_time(map, source, target)`：這一關所有 `has_charger` 都是 0。回傳最短時間，到不了回傳 `std::nullopt`；`source == target` 時是 0。

要 O((n + r) · g · log(n · g))。注意：最快的路線不一定留得住足夠的電量。

## Stage 2 — 充電

開放充電站。`fastest_time` 必須把充電也考慮進去，一樣要 O((n + r) · g · log(n · g))，`g` 可以遠大於 `n`，所以充電的處理不能讓複雜度多出一個 `g` 倍。

## Stage 3 — 交出行程

`fastest_plan(map, source, target)` 回傳 `RoutePlan`：

```cpp
struct Stop {
    int location;
    int charge_units;  // charged here before leaving, >= 0, only at chargers
};
struct RoutePlan {
    std::int64_t total_minutes;
    std::vector<Stop> stops;  // first is source, last is target, consecutive stops share a road
};
```

到不了回傳 `std::nullopt`。行程要能真的跑得完；最快的行程不只一種時，回傳任一種都算對。

## 面試追問

1. 這張圖有幾個頂點、幾條邊？跟原本的路網差多少？
2. 為什麼每次只充一單位就夠了？改成「每個路口直接決定充多少」會多花多少時間？
3. 如果電池有回充（下坡加電），你的做法要改哪裡？會不會影響 Dijkstra 的前提？
