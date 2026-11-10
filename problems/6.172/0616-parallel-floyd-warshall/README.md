---
title: 平行 Floyd-Warshall 與遞移閉包
unit: 06 · Work/Span 理論（HW5）
source: 6.172 HW5 Write-up 6（CLRS 27.2-6）
pdf: materials/6.172/homework/hw05/handout.pdf
read: materials/6.172/lectures/lec08.pdf
kind: code
sanitizer: tsan
minutes: 110
---

# 0616 · 平行 Floyd-Warshall 與遞移閉包

> **實務上為什麼會遇到**：路徑規劃的全對全距離、模組相依的傳遞閉包（「改這個 header 要重編哪些檔案」）都是這個三層迴圈。難處不在分析，在於看出「哪些東西在這一輪不會被改」，那正是能平行的理由。

沿用 `template <typename Exec>`（`_shared/01_exec.hpp`）：`exec::Meter` 算 work／span，`exec::ForkJoin` 由 TSan 把關。`dist` 是 `n×n` 的 row-major 陣列，沒有邊的格子是 `kNoEdge`，對角線是 0。

## Stage 1 — 平行化三層迴圈

`floyd_warshall(exec, dist, n)`：就地算全對全最短距離。可能有負邊，但沒有負環；`kNoEdge` 參與的加法要跳過（別讓它溢位或變成「看起來可達」）。

最外層 `k` 只能循序（第 `k` 輪要用第 `k-1` 輪的結果）。內兩層要用 `parallel_for`。關鍵觀察：第 `k` 輪時第 `k` 列和第 `k` 行**不會改變**（`d[k][k] = 0`），所以寫入 `d[i][j]` 時讀 `d[i][k]`、`d[k][j]` 是安全的——前提是你真的**不要去寫** `i == k` 或 `j == k` 的格子，否則 TSan 會當場抓到（就算寫回的值一樣，那依然是資料競爭）。

計帳：每考慮一個 `i != k` 且 `j != k` 的 `(i, j)` 就 `exec.charge(1)`（不管是否更新），所以 work 恰好是 `n(n-1)²` 加上 fork 次數。Span 要求 `O(n lg n)`：不超過 `n·(2⌈lg n⌉ + 6)`。

## Stage 2 — 路徑

`floyd_warshall_paths(exec, dist, next, n)`：`next` 同尺寸；呼叫時由 `dist` 自行初始化（有邊 `next[i][j] = j`，否則 `-1`；對角線 `-1`）。之後更新 `d[i][j]` 時順便把 `next[i][j]` 設為 `next[i][k]`，同樣不要寫 `i == k`、`j == k` 的格子。

- `reconstruct_path(next, n, s, t)`：回傳 `s` 到 `t` 的頂點序列（含兩端），不可達回傳空，`s == t` 回傳 `{s}`。
- `has_negative_cycle(dist, n)`：跑完 `floyd_warshall` 後呼叫，任一對角線為負就是有負環。

## Stage 3 — 實務延伸：位元集合遞移閉包

`transitive_closure(exec, rows, n)`：`rows` 是 `n` 列的位元矩陣，每列 `words = (n + 63) / 64` 個 `uint64_t`，`rows[i * words + (j / 64)]` 的第 `j % 64` 位表示 `i → j`。把它改成遞移閉包（`i` 能經過至少一條邊走到 `j`）。第 `k` 輪只要把第 `k` 列 OR 到「第 `k` 位為 1 的其他列」，內層可平行（一樣不能動第 `k` 列）。

用位元集合比逐格布林陣列快數十倍：測試要求比同一支執行檔裡的逐格 baseline 快至少 **10 倍**。

## 面試追問

1. 寫出 Stage 1 的 work 與 span，並解釋為什麼 `k` 不能平行（HW5 Write-up 6）。若 `(i, j)` 內兩層改回循序，span 變成多少？
2. 為什麼 Stage 1 提示的 `d[k][k] = 0` 那個性質是必要前提？有負環時會怎麼樣？
3. 位元集合版的複雜度 `O(n³/w)` 裡的 `w` 代表什麼？什麼情況下輸入稀疏圖反而該改用 `n` 次 BFS？
