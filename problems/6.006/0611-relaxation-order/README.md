---
title: 手算 Dijkstra 轉成程式（relaxation 順序與合法性檢查）
unit: 06 · Weighted Shortest Paths（PS6）
source: 6.006 PS6 Problem 6-1
pdf: materials/6.006/psets/ps6/questions.pdf
answer: materials/6.006/psets/ps6/solutions.pdf
read: materials/6.006/lectures/lec11.pdf, materials/6.006/lectures/lec13.pdf
kind: code
minutes: 60
---

# 0611 · 手算 Dijkstra 轉成程式

> **實務上為什麼會遇到**：要判斷一個實作對不對，光看最後的距離不夠，還要看「過程」是否符合演算法。寫一個 trace checker 是把演算法的不變量真正想清楚的好方法，property-based testing 也常常這樣用。

原題要你手算兩個演算法的 relaxation 順序。這裡把過程寫成程式，再反過來寫一個檢查器。圖的型別：

```cpp
struct Arc { int to; std::int64_t weight; };
using WeightedGraph = std::vector<std::vector<Arc>>;
using RelaxedEdge = std::pair<int, int>;
struct RelaxationTrace {
    std::vector<RelaxedEdge> order;
    std::vector<std::int64_t> dist;
};
```

`dist` 是最短距離（到不了為 `kUnreachable`）。`order` 列出**所有從 `s` 可達頂點出發的邊**，每條恰好一次，依「嘗試 relax」的先後排列。起點不存在時丟 `std::out_of_range`。一次只看一關，做完再往下讀。

## Stage 1 — DAG Relaxation

`dag_relaxation_trace(g, s)`，保證 `g` 是 DAG，權重可為負。只考慮從 `s` 可達的頂點，依拓撲順序處理；當有多個頂點可以處理時，選編號最小的；同一個頂點的出邊依 `to` 由小到大嘗試。

## Stage 2 — Dijkstra

`dijkstra_trace(g, s)`，保證權重非負（可以有環）。下一個處理的頂點是 `(距離估計, 編號)` 最小的未處理可達頂點；同一個頂點的出邊依 `to` 由小到大嘗試。

## Stage 3 — 合法順序檢查器

`is_legal_dijkstra_order(g, s, order)`：判斷 `order` 是否可能是**某一次** Dijkstra 的 relaxation 順序，不限 tie-breaking 方式：

- 每次取出頂點時，只要它的估計距離在當下是最小的都算合法（平手的順序不限）；同一個頂點的出邊可以任意順序，但必須連續出現、恰好各一次。
- 從 `s` 可達、又有出邊的頂點都必須出現；沒有出邊的頂點不會出現在 `order` 裡，不影響判斷。
- `order` 裡出現不存在的頂點或邊時回傳 `false`。

要 O((V + E) log V)。

## 面試追問

1. 為什麼 DAG Relaxation 的順序不必依距離，Dijkstra 卻一定要？
2. 兩個頂點最終距離相同時，先處理哪個重要嗎？如果有權重為 0 的邊，你的檢查器要特別注意什麼？
3. 這種 trace checker 可以怎麼用在壓力測試上？它能抓到哪些光比對最終距離抓不到的 bug？
