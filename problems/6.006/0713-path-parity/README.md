---
title: DAG 路徑計數與同餘（狀態擴充）
unit: 07 · Dynamic Programming I（PS7）
source: 6.006 PS7 Problem 7-3
pdf: materials/6.006/psets/ps7/questions.pdf
answer: materials/6.006/psets/ps7/solutions.pdf
read: materials/6.006/lectures/lec15.pdf
kind: code
minutes: 70
---

# 0713 · DAG 路徑計數與同餘

> **實務上為什麼會遇到**：數位電路的靜態時序分析要知道一條 timing path 經過奇數或偶數個 inverter（訊號有沒有反相）；build graph、workflow engine 也常要統計滿足某種條件的路徑。路徑可能有指數多條，絕對不能列舉，但「模 k 的餘數」只有 k 種，把它塞進狀態就好。

圖是 weighted directed graph，邊用 adjacency list 表示：

```cpp
struct Edge { int to; int weight; };          // weight may be negative
using Graph = std::vector<std::vector<Edge>>; // graph[v] lists edges out of v
```

路徑的 weight 是所有邊的總和，平行邊算不同路徑，`source == target` 時長度 0 的空路徑算一條（weight 0）。路徑數超過 2⁶⁴ 時以 `std::uint64_t` 的 wrap-around 語意計算（照常寫就好）。

所有函式都要符合：

- 圖只要有 cycle（包含從 `source` 到不了的部分）就丟 `std::invalid_argument`。
- `source`、`target` 不在範圍內時丟 `std::out_of_range`。
- 頂點編號不保證是 topological order，整體時間要線性（`|V| + |E|`，以下乘上餘數個數）。
- 不可遞迴：測試有數十萬節點的長鏈。

一次只看一關，做完再往下讀。

## Stage 1 — 路徑數

`std::uint64_t count_paths(const Graph& graph, int source, int target)`。

## Stage 2 — 奇偶

```cpp
struct ParityCounts { std::uint64_t even; std::uint64_t odd; };
ParityCounts count_paths_by_parity(const Graph& graph, int source, int target);
```

Weight 可以是負數，判斷奇偶時要小心。

## Stage 3 — 任意模數

`std::vector<std::uint64_t> count_paths_by_residue(const Graph& graph, int source, int target, int modulus)`：回傳大小為 `modulus` 的 vector，第 `r` 格是 weight 同餘 `r`（模 `modulus`，正規化到 `[0, modulus)`）的路徑數。`modulus < 1` 時丟 `std::invalid_argument`。O(modulus · (|V| + |E|))。

## Stage 4 — 實務延伸：找一條符合條件的路徑

`std::optional<std::vector<int>> find_path_with_residue(graph, source, target, modulus, residue)`：回傳任一條 weight 同餘 `residue` 的路徑（頂點序列，含兩端），沒有時回傳 `std::nullopt`。錯誤處理與前面相同。

## 面試追問

1. 把 weight 換成 parity 之後狀態數只多一倍。這招什麼時候會讓狀態爆炸？（例如換成「總和剛好是 W」）
2. 為什麼 C++ 的 `%` 在負數上會把 parity 判斷搞錯？`x & 1` 在什麼前提下才正確？
3. 真的需要路徑數超過 64 bit 的精確值時，時間複雜度要怎麼重算（word RAM 的前提變了）？
