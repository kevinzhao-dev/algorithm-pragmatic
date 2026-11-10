---
title: 四台機器的 makespan 平衡（多維 Subset Sum）
unit: 08 · Dynamic Programming II（PS8）
source: 6.006 PS8 Problem 8-3
pdf: materials/6.006/psets/ps8/questions.pdf
answer: materials/6.006/psets/ps8/solutions.pdf
read: materials/6.006/recitations/rec18.pdf, materials/6.006/lectures/lec18.pdf
kind: code
minutes: 90
---

# 0813 · 四台機器的 makespan 平衡

> **實務上為什麼會遇到**：把一批訓練 job 分到幾台 GPU、把 map task 分到 worker，要讓最晚結束的那台盡量早（makespan）。任意多台是 NP-hard，機器數固定時有 pseudopolynomial 的精確解；真實 scheduler 通常先試精確解，太貴就退回 LPT 這類 greedy，並且知道自己退回了。

`jobs` 是 `n` 個工作的處理時間（正整數，可以重複）。總和 `m = Σ jobs`。把每個工作分到一台機器，一台機器的負載是它所有工作的總和；`makespan` 是負載最大的那台。目標是 makespan 最小。

```cpp
struct Assignment { std::int64_t makespan; std::vector<int> machine; };
```

`machine[i]` 是 `jobs[i]` 分到的機器編號（從 0 開始）。沒有工作時 `makespan` 是 0。一次只看一關，做完再往下讀。

## Stage 1 — 兩台機器

`Assignment balance_two(std::span<const int> jobs)`：O(n·m) 時間。

## Stage 2 — 四台機器的最佳 makespan

`std::int64_t min_makespan_four(std::span<const int> jobs)`：O(n·m³) 時間（`m` 不大，測試最多幾百）。

三維的 subset-sum：狀態是前三台機器各自的負載，第四台由總和推出。滾動 `n` 那一維可以省記憶體。

## Stage 3 — 四台機器的分配

`Assignment balance_four(std::span<const int> jobs)`：回傳實際分配，makespan 要最小。同樣 O(n·m³) 時間。

和 0802 的 witness 一樣：不要存 `n` 層表。想想「這個狀態第一次變成可達，是靠哪個工作」。

## Stage 4 — 實務延伸：scheduler

```cpp
struct Plan { std::int64_t makespan; std::vector<int> machine; bool exact; };
Plan plan_jobs(std::span<const int> jobs, std::size_t machines, std::uint64_t work_limit);
```

- `machines == 1`：全部放機器 0，`exact` 為 `true`。
- `machines == 2` 或 `4`：如果精確解的工作量（`machines == 2` 時為 `n·(m+1)`，`machines == 4` 時為 `n·(m+1)³`）不超過 `work_limit`，用精確解，`exact` 為 `true`；否則用 LPT，`exact` 為 `false`。
- 其他 `machines`：一律 LPT，`exact` 為 `false`。
- `machines == 0` 時丟 `std::invalid_argument`。

LPT：工作由大到小排序，每次放到目前負載最小的機器。LPT 的 makespan 不超過最佳值的 `4/3 − 1/(3k)` 倍，測試會驗證。

## 面試追問

1. 為什麼這題是 pseudopolynomial？`m` 的位元數加 1，時間大約變成幾倍？
2. Stage 3 的 `first` 為什麼足以重建分配？第四台機器什麼時候會被分到工作？
3. 先用 LPT 算出上界 `U`，再把 DP 的每個維度裁到 `U` 以內，複雜度實際會降多少？這個技巧有什麼風險？
