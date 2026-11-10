---
title: Memo 與 Bowling（SRT BOT 線性 DP）
unit: 07 · Dynamic Programming I（PS7）
source: 6.006 Lecture 15
pdf: materials/6.006/lectures/lec15.pdf
read: materials/6.006/lectures/lec15.pdf, materials/6.006/recitations/rec15.pdf
kind: code
minutes: 75
---

# 0701 · Memo 與 Bowling

> **實務上為什麼會遇到**：把昂貴函式的結果快取起來，就是 DP 的 top-down 形式；「一維掃過去、每格只問一個局部問題」則是幾乎所有 DP 的原型。先用一個小題把 SRT BOT（Subproblem、Relate、Topological order、Base、Original、Time）完整走一遍，這個單元的每一題都是同一套。

一次只看一關，做完再往下讀。

## Stage 1 — 通用的 `Memo`

`Memo<Key, Value, Hash>` 把遞迴關係式和快取拆開，呼叫端只寫 relation：

```cpp
Memo<int, std::int64_t> memo;
auto fib = [](int n, auto& recurse) -> std::int64_t {
    return n < 2 ? n : recurse(n - 1) + recurse(n - 2);
};
memo.get(90, fib);
```

| 操作 | 行為 |
| --- | --- |
| `get(key, relation)` | 算過就回傳 `const Value&`；否則呼叫 `relation(key, recurse)`，存起來再回傳。`recurse(k)` 回傳另一個子問題的 `const Value&` |
| `size()` | 已完成的子問題數量 |

每個 key 最多計算一次，回傳的 reference 在 `Memo` 存活期間一直有效。邊界要做對：

- **循環依賴**：`relation` 繞回正在計算的 key 時丟 `std::logic_error`。這正是 SRT BOT 的 T 不成立。
- **例外安全**：`relation` 丟例外時往外傳。還在計算中的 key 不留痕跡，之後可以重試；已經完成的保留。
- `recurse` 可能讓表 rehash，別握著會失效的 iterator。

## Stage 2 — Bowling 的最佳分數

有一排 `n` 個 pin，第 `i` 個的值是 `v[i]`（可為負）。每個 pin 最多處理一次，可以跳過、單獨打（得 `v[i]`），或和下一個一起打（得 `v[i]·v[i+1]`）。

`std::int64_t bowling_max_score(std::span<const int> pins)`：回傳最高總分，O(n) 時間。值的範圍 `|v| ≤ 10^6`，`n ≤ 2·10^6`。

自己先把 SRT BOT 六步寫下來，再動手。

## Stage 3 — 還原做法

`std::vector<Throw> bowling_plan(std::span<const int> pins)`：回傳一組達到最高分的打法，O(n) 時間。`Throw{first, last}` 的 `last == first`（單打）或 `last == first + 1`（一起打）。順序不拘，同分時任一組都可以。

## Stage 4 — 串流版，O(1) 記憶體

Pin 一個一個到達，不能先看完全部：

```cpp
class BowlingStream {
    void push(int pin);         // O(1)，不配置 heap
    std::int64_t best() const;  // 目前已到達的這些 pin 的最高分
};
```

Stage 2 的 suffix 形式要先知道結尾，串流需要改成 prefix 形式。想清楚 relation 真正依賴幾個狀態。

## 面試追問

1. Top-down 與 bottom-up 各在什麼情況比較好？`n` 到百萬時 top-down 會先死在哪裡？
2. 為什麼 Stage 4 能丟掉表，Stage 3 卻不行？如果要在串流上隨時回報「目前的打法」，最少要留多少資訊？
3. 回傳 `const Value&` 而不是 `Value`，如果底層從 `unordered_map` 換成 `vector`，哪裡會壞？
