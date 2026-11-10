---
title: Doubly Linked List（O(1) 區段搬移）
unit: 01 · Sequences（PS1）
source: 6.006 PS1 Problem 1-4
pdf: materials/6.006/psets/ps1/questions.pdf
answer: materials/6.006/psets/ps1/solutions.pdf
read: materials/6.006/lectures/lec02.pdf, materials/6.006/recitations/rec02.pdf
kind: code
minutes: 75
---

# 0114 · Doubly Linked List

> **實務上為什麼會遇到**：LRU cache、排程器的 ready queue、編輯器的行緩衝，都需要「手上已經握著節點時，O(1) 把它（或一整段）搬走」。這正是 `std::vector` 做不到、而 linked list 唯一值得存在的理由。

在 `solution.hpp` 實作一個擁有自身節點的雙向鏈結串列。**不可以維護長度**：一旦 O(1) 搬走一整段，就沒有辦法同步更新長度。

一次只看一關，做完再往下讀。

## Stage 1 — 兩端操作與所有權

| 操作 | 行為 | 複雜度 |
| --- | --- | --- |
| `insert_first(v)` / `insert_last(v)` | 插入並回傳新節點 | O(1) |
| `delete_first()` / `delete_last()` | 移除並回傳值；串列為空時丟 `std::out_of_range` | O(1) |
| `head()` / `tail()` / `empty()` | 空串列時 `head()`、`tail()` 為 `nullptr` | O(1) |
| 解構子 | 釋放所有節點，不可使用遞迴（百萬節點會把 stack 撐爆） | O(n) |

要處理好的邊界：只剩一個節點時刪除，`head` 和 `tail` 都要回到 `nullptr`。

## Stage 2 — 切出一段

`remove(first, last)`：`first` 和 `last` 是本串列的節點，`first` 在 `last` 之前（或相同）。把這段（含兩端）從串列切走，**O(1)** 回傳成一條新的串列。節點本身要被搬走，不可以複製。

邊界：切掉開頭、切掉結尾、切掉整條、只切一個節點。

## Stage 3 — 接回去

`splice(after, other)`：把 `other` 的全部節點接到本串列節點 `after` 的後面，**O(1)**。完成後 `other` 變成空串列。

邊界：`other` 是空的、`after` 是目前的 `tail`。

同時補上 move constructor 與 move assignment（Stage 2 回傳新串列時就會用到）。

## Stage 4 — 實務延伸：LRU Cache

用你的串列加上 `std::unordered_map` 做出 `LruCache<K, V>`：

- `explicit LruCache(std::size_t capacity)`，`capacity` 至少為 1。
- `std::optional<V> get(const K&)`：存在時回傳值，並把它標成最近使用。
- `std::optional<K> put(K, V)`：寫入或覆蓋，也算最近使用。若因此淘汰了某個 key，回傳被淘汰的 key，否則回傳 `std::nullopt`。
- `std::size_t size() const`。

`get` 和 `put` 都要平均 O(1)。

## 面試追問

做完之後，不看程式回答：

1. 為什麼這個串列不能有 O(1) 的 `size()`？如果需求堅持要有，你會犧牲哪個操作？
2. `std::list::splice` 在 C++11 之後的複雜度規定是什麼？跟你的設計取捨有什麼關係？
3. 把 `LruCache` 改成多執行緒安全：一把大鎖的瓶頸在哪？`get` 為什麼也需要寫入鎖？
