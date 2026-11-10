---
title: 倉儲機器人的取貨任務（門禁、識別證、來回最短）
unit: 05 · Graph Search（PS5）
source: 6.006 PS5 Problem 5-5
pdf: materials/6.006/psets/ps5/questions.pdf
answer: materials/6.006/psets/ps5/solutions.pdf
read: materials/6.006/lectures/lec09.pdf, materials/6.006/recitations/rec09.pdf
kind: code
minutes: 110
---

# 0515 · 倉儲機器人的取貨任務

> **實務上為什麼會遇到**：機器人路徑規劃、遊戲裡的鑰匙與機關、服務之間的權限鏈（拿到某個 token 才能呼叫下一個 API），都是「走到哪裡」之外還要記得「手上有什麼」的搜尋問題。

倉儲裡有 `locations` 個地點（`0 … locations-1`），由門相連。機器人從 `dock` 出發，要走到**至少一個**放了貨物的地點，再回到 `dock`，並讓穿過門的次數最少。

```cpp
inline constexpr int kNoBadge = -1;

struct Door {
    int a;
    int b;
    bool two_way;  // false means only a -> b
    int badge;     // kNoBadge, or an index into Site::badge_locations
};

struct Site {
    int locations;
    int dock;
    std::vector<Door> doors;
    std::vector<int> item_locations;   // may be empty, may repeat
    std::vector<int> badge_locations;  // badge i lies at badge_locations[i]; at most 8 badges
};
```

規則：

- 機器人**一抵達**某個地點（包含一開始在 `dock`），就自動拿起那裡所有的識別證和貨物，之後一直帶著。
- 要穿過 `badge != kNoBadge` 的門，身上必須已經有那張識別證（雙向門兩個方向都一樣）。
- 同一個地點、同一扇門都可以重複經過。

`L = locations`、`D = doors.size()`、`k = badge_locations.size()`。一次只看一關，做完再往下讀。

## Stage 1 — 沒有門禁

`fewest_doors(site)`：回傳最少穿過幾次門，做不到回傳 `std::nullopt`。這一關所有門的 `badge` 都是 `kNoBadge`，`badge_locations` 是空的。

`dock` 本身就放了貨物時答案是 0。

把 `k` 視為常數，要做到 O(L + D)。

## Stage 2 — 門禁

這一關開始，門可以要求識別證。`fewest_doors` 要照規則算出最少次數，一樣要 O(L + D)（`k` 視為常數）。

## Stage 3 — 交出路線

`errand_route(site)`：回傳實際走過的地點序列（第一個和最後一個都是 `dock`，`dock` 有貨物時就是只有 `dock` 一個元素的序列），做不到回傳 `std::nullopt`。最短路線不只一條時，回傳任一條都算對。

## 面試追問

1. 你的搜尋實際上走的是一張什麼圖？它有幾個頂點、幾條邊？`k` 變成 30 時會發生什麼事，怎麼辦？
2. Stage 3 的路線什麼時候必須重複經過同一個地點？能不能舉一個最短路線一定要繞回頭的例子？
3. 如果每扇門的通過時間不同，你要換掉 BFS 的哪一部分？整個建模要改嗎？
