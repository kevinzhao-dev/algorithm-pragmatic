---
title: Merkle Tree（驗證儲存與新鮮度）
unit: 10 · Distributed Algorithms（PS10）
source: 6.046J Recitation 11, Lecture 21
pdf: materials/6.046j/recitations/rec11.pdf
read: materials/6.046j/recitations/rec11.pdf, materials/6.046j/lectures/lec21.pdf
kind: code
minutes: 110
---

# 1022 · Merkle Tree

> **實務上為什麼會遇到**：把資料放在不受信任的儲存（S3、遠端磁碟、區塊鏈節點）時，你要驗證內容沒被竄改、而且**不是舊版本**。Certificate Transparency、Bitcoin 的 SPV、dm-verity 都是同一個結構：本地只存一個 root hash。

`support/sha256.hpp`（已提供）有 `sha256::hash(std::string_view)` 與 `sha256::Digest`（`std::array<unsigned char, 32>`）。`sha256::calls()` 計算呼叫次數，測試用它確認 update 真的是 O(log n)。

一次只看一關。

## Stage 1 — 建樹

規格（必須完全照做，測試會比對 root）：

- `Digest hash_leaf(std::string_view block)`：`SHA256(0x00 || block)`。
- `Digest hash_node(const Digest& l, const Digest& r)`：`SHA256(0x01 || l || r)`。
- `Digest hash_empty()`：`SHA256(0x02)`。

`MerkleTree(std::vector<std::string> blocks)`：`n = blocks.size() ≥ 1`。把葉子補到 `cap`（`≥ n` 的最小 2 的冪），多出來的葉子用 `hash_empty()`。`Digest root() const`、`std::size_t size() const`（原始 `n`）。

## Stage 2 — 成員證明

`std::vector<Digest> proof(std::size_t index) const`：從葉到根的兄弟節點（長度 `⌈lg cap⌉`，`cap = 1` 時長度 0）。

`bool verify_proof(const Digest& root, std::size_t n, std::size_t index, std::string_view block, std::span<const Digest> proof)`：純函式，不需要整棵樹。`index >= n` 或 proof 長度不對要回傳 `false`。

## Stage 3 — 更新

`void update(std::size_t index, std::string block)`：只重算路徑上的節點。每次呼叫 `sha256::hash` 的次數不超過 `lg cap + 2`。

## Stage 4 — 實務延伸：在不受信任的伺服器上讀寫

```cpp
class UntrustedServer {   // 測試提供實作，可能說謊
public:
    virtual ~UntrustedServer() = default;
    virtual std::string get_block(std::size_t i) const = 0;
    virtual std::vector<Digest> get_proof(std::size_t i) const = 0;
    virtual void put_block(std::size_t i, std::string block) = 0;
};

class VerifiedClient {
public:
    VerifiedClient(std::size_t n, Digest root);
    std::string read(UntrustedServer& server, std::size_t i);
    void write(UntrustedServer& server, std::size_t i, std::string block);
    const Digest& root() const;
};
```

Client 只信任自己手上的 root（O(1) 的 trusted storage）。`read` 要驗證 server 回傳的 block 和 proof，否則丟 `TamperDetected`；`write` 也要先驗證舊 block 的 proof，用同一條 proof 算出新 root，再把新 block 交給 server；驗證失敗時 root 不可改變。

（這解決了 recitation 提到的 freshness：攻擊者重放舊版本會對不上新的 root。）

## 面試追問

1. 為什麼 leaf 和 node 要用不同的前綴（domain separation）？少了前綴會有什麼攻擊？
2. 為什麼要把 proof 長度跟 `n` 綁定？如果 verifier 不知道 `n` 會怎樣？
3. Merkle tree 和直接存 n 個 hash、或存一個大 hash，三種做法的 trusted storage／驗證時間／更新時間各是多少？
