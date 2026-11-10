---
title: 非同步網路上的 Layered BFS（不做 correction）
unit: 10 · Distributed Algorithms（PS10）
source: 6.046J PS10 Problem 10-2
pdf: materials/6.046j/psets/ps10/questions.pdf
answer: materials/6.046j/psets/ps10/solutions.pdf
read: materials/6.046j/lectures/lec20.pdf
kind: code
minutes: 150
---

# 1012 · Layered BFS

> **實務上為什麼會遇到**：路由協定的 BFS 樹、資料中心的 fabric 建樹，一旦節點「輸出」了 parent，就很難收回。1002 的 relaxation 版本要不斷校正；這題用 layered wave 換掉 correction，訊息與時間的上界都比較乾淨。

用 `netsim::AsyncNetwork`，訊息延遲任意（但 FIFO），延遲上限為 `d`。圖是連通無向圖，至少兩個節點，root 是 `v0`（透過建構子得知），不知道 n 或 diam。

```cpp
class LayeredBfs {
public:
    struct Message { ... };   // 自行定義，四種訊息（search、parent、ready、done）就夠
    explicit LayeredBfs(bool is_root);
    void on_start(netsim::Context<Message>& ctx);
    void on_message(int port, const Message& m, netsim::Context<Message>& ctx);

    bool root_done() const;     // root 專用：整個 BFS 已結束
    int parent_port() const;    // 非 root 的 parent port；root 為 -1
    int level() const;          // BFS 深度，root 為 0
};
```

## 規格

每個 phase，root 用 `ready` broadcast 沿著目前的 BFS 樹把訊號送到目前的 frontier，frontier 節點向所有鄰居送 `search`，收齊回覆（`parent(true)`/`parent(false)`）後 convergecast `done(bit)` 回 root，`bit` 表示這一輪有沒有產生新節點。root 收齊所有 `done` 後：有新節點就開始下一個 phase，否則結束。

- 一旦節點決定了 parent，就用 `ctx.output(parent_port)` 輸出，**一生只輸出一次**、之後不得改變（root 不用輸出）。這就是題目要的「不做 correction」。
- `level()` 必須等於到 root 的 hop 距離。

## 複雜度要求

令 `E` 為邊數、`diam` 為圖的直徑。測試會確認：

- 訊息總數 ≤ `4E + 2·n·(diam + 2)`。
- 延遲上限 `d` 時，完成時間 ≤ `3·d·(diam + 3)²`。
- 在各種 delay policy（unit、隨機、FIFO 下的 adversarial skew）下，`root_done()` 最終成立、每個節點恰好輸出一次 parent，且 `level` 正確。

## 面試追問

1. Layered BFS 為什麼每個 phase 要花 O(diam·d)、總共 O(diam²·d)？訊息量又為什麼是 O(E + n·diam)？
2. 和 1002 的 relaxation BFS 相比，時間與訊息各賺賠什麼？什麼情況下 layered 更慢？
3. 為什麼每條 tree edge 每個 phase 最多走兩次 ready/done？如果把 phase 改成一次多層，能快多少？（synchronizer 的想法）
