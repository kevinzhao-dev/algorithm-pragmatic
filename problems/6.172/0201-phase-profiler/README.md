---
title: Phase Profiler（自己做 perf report）
unit: 02 · Profiling（HW2）
source: 6.172 HW2 §2 Recitation（perf／Cachegrind 概念）
pdf: materials/6.172/homework/hw02/handout.pdf
read: materials/6.172/homework/hw02/handout.pdf, materials/6.172/lectures/lec10.pdf
kind: code
minutes: 90
---

# 0201 · Phase Profiler（自己做 perf report）

> **實務上為什麼會遇到**：推論服務慢了，要回答的是「時間花在哪一段」。`perf report` 的 inclusive／self 欄位不是魔法，就是這題要你寫的東西。macOS 沒有 perf，但有 `sample` 和 Instruments；它們輸出的就是下面會處理的 call stack。

做一個依 phase（程式片段名稱）計時的 profiler。時間來自建構時傳入的 `clock`（回傳單調遞增的 tick，測試用假時鐘，所以一切都是確定的）。

## Stage 1 — enter / exit 與 self、inclusive

| 介面 | 行為 |
| --- | --- |
| `Profiler(clock)` | `clock` 是 `std::function<std::uint64_t()>` |
| `enter(name)` | 開始一個 phase。`name` 要自己複製，傳進來的字串可能馬上被銷毀 |
| `exit()` | 結束最內層 phase；沒有未結束的 phase 時丟 `std::logic_error`（狀態不變） |
| `depth()` | 目前巢狀深度 |
| `report()` | 回傳每個 phase 的 `Entry{name, calls, inclusive, self}`，依 `self` 由大到小，同值依 `name` 字典序 |

- `self`：這個 phase 自己花的時間，扣掉直接巢狀在裡面的 phase。
- `inclusive`：這個 phase 從 enter 到 exit 的總時間，包含內部。
- **遞迴**：同一個名稱在呼叫鏈上出現兩次時，`inclusive` 只算最外層那次（否則會重複計算）。`self` 和 `calls` 照常累加。

例：`a` 進入後做 1 tick，呼叫 `b`（做 2 tick），回來再做 1 tick：`a` 的 `self = 2`、`inclusive = 4`，`b` 的 `self = inclusive = 2`。

`report()` 只計入已經結束的呼叫。

## Stage 2 — RAII

`ScopedPhase(profiler, name)`：建構時 `enter`，解構時 `exit`。解構不可丟例外。例外穿過 scope 時巢狀結構要保持正確。

## Stage 3 — Folded stacks（flamegraph 輸入格式）

`std::vector<std::string> folded() const`：每一條不同的呼叫鏈一行，格式 `"a;b;c 120"`（`self` tick 數），由外到內以分號連接。`self` 為 0 的路徑不輸出。結果依整行字典序排序。

## Stage 4 — 實務延伸：從 stack 取樣產生同樣的報告

`sample`／`perf` 是定時取樣，每個樣本是一條完整 call stack（最外層在前）。實作：

```cpp
std::vector<Profiler::Entry> report_from_samples(std::span<const std::vector<std::string>> stacks);
```

每個樣本當作 1 tick：`self` 計到最內層的 frame，`inclusive` 計到這個樣本中出現過的每個名稱（同一個樣本同名只算一次）。`calls` 留 0。排序規則同 `report()`。

## 面試追問

1. 取樣式 profiler 和儀器化 profiler 各自的誤差來源是什麼？什麼情況下一個有 bug 的程式在 profiler 底下不會重現？
2. 用 `sample $PID 2` 看一個 Release 的執行檔，為什麼有些函式完全消失了？（inlining）
3. 把 cachegrind 的 instruction count 當作時間的代理指標，優點和缺點各是什麼？（HW2 Write-up 1）
