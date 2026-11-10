---
title: Furrow Search Engine（bitboard → alpha-beta → fork-join）
unit: 14 · Game-Playing Engine（Project 4）
source: 6.172 Project 4
pdf: materials/6.172/projects/p4/handout.pdf
read: materials/6.172/lectures/lec19.pdf, materials/6.172/lectures/lec20.pdf
kind: project
minutes: 900
sanitizer: tsan
timeout: 180
---

# 1411 · Furrow Search Engine

> **實務上為什麼會遇到**：alpha-beta、transposition table、move ordering 和 speculative parallelism 不只出現在棋類，也是資料庫 join order、規劃器和形式驗證的共同骨架。這題的重點和原課程專案一樣：**看著效能數字把搜尋引擎改快**，不是背演算法。

棋盤遊戲是為這題另外設計的（沒有採用任何課程內的遊戲規則）。規則全部寫在下面，`support/slow_board.hpp` 有一份「照字面翻譯」的陣列實作，是對錯的唯一標準，這題所有速度都要贏過它。

## Furrow（犁溝）規則

7×7 棋盤，格子編號 `a1 = 0, b1 = 1, …, g7 = 48`（`board.hpp`）。雙方各有 **3 架犁**（plow）：South 在 `b1, d1, f1`，North 在 `b7, d7, f7`，South 先走。

- 一手棋：選一架自己的犁，沿八個方向之一直線滑行至少一格，停在空格。滑行路徑**可以穿過自己的 furrow**（但不能停在上面），不能穿過敵方 furrow，也不能穿過任何一架犁。
- 走完：起點變成**自己的 furrow**；落點**上下左右**四格內的敵方 furrow 全部翻成自己的。
- 輪到的一方**沒有合法手就結束**，最終以 furrow 數量決勝負。

**評估函數**（對輪到的一方）：`diff = 己方 furrow 數 − 敵方 furrow 數`，`m = 己方可走手數`，`m′ = 敵方假設輪到時的可走手數`：
`m == 0` 時是終局：`diff > 0` 回傳 `kWinScore + diff`，`diff < 0` 回傳 `−kWinScore + diff`，`diff == 0` 回傳 0；否則回傳 `kFurrowWeight × diff + m − m′`（常數在 `board.hpp`）。

`Position` 是四個 bitboard 加 `side` 和 `key`（`key` 從 Stage 3 開始才有意義，`same_board` 比較時不看它）。

## Stage 1 — Bitboard 走子產生

`generate_moves`（順序自訂）、`apply_move`（複製出新的 `Position`，不做 undo）、`perft`、`evaluate`。全部照 bitboard 寫：預先算好 ray / 鄰居表，用 `countr_zero` / `countl_zero` 找最近的阻擋。邊界是經典陷阱：`g 檔往東` 不能繞回 `a` 檔。`perft`（從起始局面）在深度 1–5 是 39、1424、56548、2122948、82403728，可拿來除錯。

`generate_moves` 要比陣列實作快至少 4 倍（同一支執行檔比）。`perft` 的葉節點不必真的展開：能用 popcount 數的就別列舉。

## Stage 2 — Minimax 與 Alpha-Beta

`minimax(pos, depth, nodes)`：不剪枝的 negamax，當對照組。深度 0 或終局回傳 `evaluate`。

`Engine::search(pos, depth, alpha, beta)`：fail-soft alpha-beta（negamax 形式）。全窗口時分數**必須等於 minimax**；窄窗口時回傳值要遵守 fail-soft 合約（真值 ≤ α 就回傳 ≤ α，真值 ≥ β 就回傳 ≥ β）。`SearchResult.best` 必須是合法且真的達到該分數的走法（深度 0 或終局回傳 `kNoMove`）。`stats.nodes` 算進入過的每個節點（含 root 與葉節點）。

## Stage 3 — Zobrist 與 Transposition Table

- `zobrist_key(pos)` 計算完整 hash，要取決於犁／furrow 的種類、格子、擁有者和輪到誰。`apply_move` 要用增量維護 `key`。root 的 `key` 可能是 0，由 `Engine` 自己算第一次。
- `TranspositionTable` 用 `2^bits` 個 slot（替換策略自訂），`probe` **只能**回傳 key 完全相符的 entry（表再小也不能回別的局面的資料）。
- `Engine` 打開 `use_tt` 時：拿 TT 的 bound 來截斷，**只有 entry 深度等於目前深度時才能用**。真實引擎會接受更深的 entry，但這題的驗收是「分數必須等於同深度的 minimax」，你得想清楚差別（面試追問）。TT 裡沒有完整截斷資料時，它存的 best move 還是可以拿來排序（Stage 4）。
- `iterative(pos, d)`：跑深度 1..d，回傳最後一層的結果與累積的 stats。

## Stage 4 — Move Ordering（用節點數驗收）

`use_ordering` 打開時，alpha-beta 的搜尋順序：TT best move、翻面數（類比吃子）多的、killer、history。要過的目標，前兩項只看節點數與統計（完全確定），最後一項是同一支執行檔裡的倍數：

- 比 plain alpha-beta 少至少 70% 節點，加上 TT 再少至少 5 倍（相對 plain）。
- 大部分 cutoff 都發生在第一個試的走法（first-move cutoff rate ≥ 85%）。
- 打開全部的 tuned engine 比 plain alpha-beta 快至少 5 倍。

跑 `./practice test 1411` 建置之後，執行 `build/release/bin/6.172/1411-furrow-engine/run --perf-only --stage 4 --verbose` 會印出 `engine report`：nodes、EBF、first-move cutoff rate、TT hit rate。**看著數字改**，不要憑感覺調。

## Stage 5 — Fork-Join 搜尋

`Engine::parallel(pos, depth)` 用 `practice::fork2` 平行搜尋，分數必須與 `search` 相同（`practice::set_workers` 的任何 worker 數），由 ThreadSanitizer 驗證零 data race。先搜第一個子節點建立 alpha（Young Brothers Wait），其餘 sibling 才平行；再加上 alpha 共享與 abort 機制，cutoff 發生時要把不再需要的子樹中止。

TSan 不接受「良性 race」：TT 要用 atomic（配 XOR 驗證把 torn entry 丟掉）或別的方式，共享的計數器和 history 也一樣。要過：8 workers 至少快 1.5 倍，總節點數 4 workers 不超過序列的 2.2 倍、8 workers 不超過 2.8 倍（量的是投機浪費）。

## 面試追問

1. 真實引擎接受「更深的 TT entry」來截斷，好處和代價各是什麼？這和本題「分數等於固定深度 minimax」的驗收為什麼衝突？
2. Young Brothers Wait 為什麼先序列搜第一個子節點？改成全部平行時，節點數在你的量測中膨脹多少、為什麼？
3. 真實引擎的 TT 經常靠 data race 運作，C++ 標準怎麼看這件事？你用了什麼手段讓它在 TSan 下乾淨，又花了什麼代價？
