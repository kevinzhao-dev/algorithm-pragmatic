---
title: 匿名環上的 Leader Election（對稱性、隨機與不可能性）
unit: 10 · Distributed Algorithms（PS10）
source: 6.046J PS10 Problem 10-1
pdf: materials/6.046j/psets/ps10/questions.pdf
answer: materials/6.046j/psets/ps10/solutions.pdf
read: materials/6.046j/lectures/lec19.pdf
kind: code
minutes: 130
---

# 1011 · 匿名環上的 Leader Election

> **實務上為什麼會遇到**：沒有 UID 的 IoT 群組、需要打破對稱的共識協定，都會碰到一個基本事實：完全對稱的 deterministic 節點永遠選不出 leader；有了隨機性但不知道 n，問題仍然不可解。這題把三個論證全部變成可以跑的程式。

環由 `Topology::ring(n)` 建立（同步模型，`SyncNetwork`），所有 process **完全相同**，只有 port 方向感。一次只看一關。

## Stage 1 — Deterministic 為什麼不可能（Part a）

```cpp
template <typename P>
bool symmetry_preserved(std::function<P()> make, int n, int rounds);
```

用 `make()` 建立 n 個 process 放上環，依序跑 `rounds` 回合（`net.step()`），每一回合之後比較所有 process 的 `state_digest()`（`P` 提供，回傳 `std::string`）。全部相同回傳 `true`，一旦有差異回傳 `false`。測試會餵 deterministic 的 process 確認對稱不可能被打破，也會餵一個偷偷帶有不同內部編號的 process，確認你的檢查真的有比。

（這就是 lecture 的數學歸納法證明：相同的狀態、相同的 port 輸入，下一回合還是相同的狀態。）

## Stage 2 — 隨機化，知道 n（Part b）

`RandomizedElection(int n, std::uint64_t seed)`：同步 process，同時滿足：

1. **Safety**：任何時候至多一個 process 輸出 leader（`is_leader()`）。
2. **Termination**（機率 1）：最終全部 `halted()`，恰好一個 `is_leader()`。
3. `phases()` 回傳用了幾個 phase；每個 phase 至多 `n` 回合、至多 `n²` 則訊息（`stats.rounds ≤ n·phases`、`stats.messages ≤ n²·phases`）。
4. 每個 phase 成功機率至少 1/2：phases 超過 `h` 的比例在統計上必須 ≤ `2^-h`（測試用多個 seed 量測 tail）。

提示：每個 process 每個 phase 從 `1..n²` 隨機取 id，往順時針傳 n 回合，看最大值是否唯一。

## Stage 3 — 不知道 n 為什麼不可能（Part c）

```cpp
struct MirrorResult { int leaders_small = 0; int leaders_large = 0; };
template <typename P, typename Make>
MirrorResult mirror_attack(Make make, int n, std::uint64_t seed, std::int64_t max_rounds);
```

`make(s)` 回傳一個用隨機種子 `s` 的 process（`P`，不知道 n，有 `is_leader()`）。在環 `n` 上執行（第 i 個節點種子 `seed+i`），再在環 `2n` 上執行：位置 `i` 和 `i+n` 用**相同**的種子 `seed+i`（複製 random tape）。回傳兩次的 leader 數。

這個 harness 把論證具體化：只要環 `n` 上選得出 leader，環 `2n` 的鏡像執行就有兩個 leader——沒有演算法同時正確處理這兩種大小。

## 面試追問

1. Part (a) 的證明為什麼需要「確定性」和「相同的 port labeling」？如果 port 標籤不一致呢？
2. 為什麼 Stage 2 只需要 `1..n²` 的 id 空間？為什麼用 birthday bound 算成功機率？成功機率跟時間怎麼換算？
3. 分散式系統在實務上怎麼解決 Stage 3 的 impossibility？（用 n 的上界、用 UID、或放棄確定性保證）
