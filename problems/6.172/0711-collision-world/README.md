---
title: 2D Collision World（quadtree + fork-join）
unit: 07 · Collision Detection（Project 2）
source: 6.172 Project 2
pdf: materials/6.172/projects/p2/handout.pdf
read: materials/6.172/lectures/lec06.pdf, materials/6.172/lectures/lec07.pdf, materials/6.172/lectures/lec08.pdf, materials/6.172/homework/hw04/handout.pdf
kind: project
minutes: 540
sanitizer: tsan
---

# 0711 · 2D Collision World

> **實務上為什麼會遇到**：物理引擎的 broad phase、AGV 與機器手臂的路徑衝突檢查、地圖服務的 spatial join，做的都是同一件事：在大量移動物體裡只挑「有可能撞到」的配對做昂貴的精確判定。在同一段時間內精確判定，叫 continuous collision detection。

世界裡有一堆剛體細桿（`Segment`），以固定 tick 前進，碰到就交換速度，碰到牆就反彈。你要把偵測由暴力 O(N²) 一路做到 quadtree 加 fork-join，每一關結果都要和暴力解**一個 pair 都不差**。

`support/` 已備好不用改：`world_types.hpp`（`Vec`、`Segment`、`Box`、`Hit`）、`scene.hpp`（seeded 場景生成）、`scale.hpp`（`scale::fork2` 與確定性 work/span 量測）。程式分成 `collide.hpp`、`world.hpp`、`quadtree.hpp`。全程用整數座標，**不准出現浮點數**。保證 `|座標| ≤ 2²⁶`、`|v| ≤ 2²⁰`：請自己算出 `int64_t` 夠用的理由。

## Stage 1 — Narrow phase、暴力偵測、世界一步

**`collides(a, b)`**：桿 `s` 在一個 tick 的時間參數 `τ ∈ [0, 1]` 佔據 `s.p + λ(s.q − s.p) + τ·s.v`（`λ ∈ [0, 1]`）。兩桿存在某個共同的 `τ` 使位置重合就算相撞；**是閉集**：端點剛好碰到、共線重疊、一開始就重疊而後分離都算。`p == q` 的零長度桿也要正確。

**`detect_brute(segments, pred)`** 回傳全部相撞的 `Hit`，id 正規化（`a < b`）、依 `(a, b)` 遞增排序。`Hit` 裡是 `Segment::id`，不是陣列 index。對每個無序 pair 恰好呼叫一次 `pred`。這是之後所有關卡的基準線。

**`World::step(detect)`** 一個 tick（`id == index`），順序是規格的一部分：

1. `hits = detect(segments)`；對每個 hit **依排序後順序**交換兩桿的 `v`，`segment_hits` 累加 hit 數。
2. 依 id 順序處理每一桿，x、y 兩軸各自獨立：若兩端點位移後有一點落在 `[0, width]`（或 `[0, height]`）之外，就把該軸 `v` 取負、**這個 tick 該軸不位移**、`wall_hits` 加一；否則位移。

## Stage 2 — Quadtree

`detect_quadtree(segments, pred, options)`：輸出必須和 `detect_brute` 完全相同，但只對可能相撞的 pair 呼叫 `pred`，且**每個 pair 最多一次**。

你要自己想清楚的事：

- 物體會動，索引要涵蓋整個 tick 掃過的範圍；速度快的細桿可能跨很多格。
- 長桿壓在切割線上放不進任何子格。
- 兩桿可能剛好在切割線上接觸；格子邊界要怎麼定義才不會把它們分開？
- 完全重合的桿再怎麼切都分不開：`max_depth` 要真的生效，`leaf_capacity` 只是起點。

`QuadtreeOptions` 的任何合法設定都要給正確答案（包括 `max_depth == 0`）。

## Stage 3 — Fork-join，零 data race

`detect_parallel(segments, pred, options, grain)`：用 `scale::fork2` 分工（一般情況下等同 `practice::fork2`），子問題不超過 `grain` 時就走序列路徑。要求：

- 任何 worker 數下輸出都和序列版相同且確定（排序好）；`pred` 會被多個執行緒同時呼叫，呼叫總數要與序列版相同。
- 由 ThreadSanitizer 驗證。這題的功能測試都跑在 TSan 下，發現 race 就會在當下的測試中斷。
- 收集結果是最容易出事的地方。想想你手上能用的：`practice::worker_index()` 分 buffer，或是遞迴回傳再合併。
- 確定性的平行度檢查：測試用 `scale::measure` 量 work/span（work 是 `pred` 呼叫次數），spread-out 場景上 `work / span` 要超過門檻。`grain` 太大會直接在這裡被擋下。

## Stage 4 — 量測

沒有新介面，要過的是同一支執行檔裡對基準線的倍數（只跑 Release）：

| 場景 | 要求 |
| --- | --- |
| 大量均勻分布 | quadtree 至少比暴力快 20 倍 |
| 群聚 | quadtree 至少比暴力快 5 倍 |
| 全部橫跨中心（樹幫不上忙） | quadtree 最多比暴力慢 3 倍 |
| 8 workers vs 1 worker | 至少 1.5 倍（少於 8 個硬體執行緒就略過） |
| 跑完整個 `World` 模擬 | 至少比暴力世界快 8 倍 |

沒過就先量再改：`leaf_capacity` 取多少？每個節點配置一次記憶體嗎？`grain` 太小會變成什麼樣子？可以用 `sample` 或 Instruments 看熱點。倍數測試內建 best-of-N，但機器太忙時可能失準，先重跑一次再下結論。

## 面試追問

1. 為什麼 fork-join 版的輸出必須排序？去掉排序後，一樣的 hit 集合為什麼還會讓世界的演化不同？
2. 每次 tick 重建 quadtree 而不是增量更新，在什麼條件下才划得來？你的量測顯示重建佔了多少比例？
3. 長桿很多時根節點的清單會爆掉，你還能把 parallel span 壓低嗎？用什麼代價換？

想同時抓記憶體錯誤（TSan 抓不到越界）：`cmake -S . -B build/asan -G Ninja -DPRACTICE_MODE=asan && cmake --build build/asan --target t_6_172_0711_collision_world && build/asan/bin/6.172/0711-collision-world/run --skip-perf`。
