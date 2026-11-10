---
title: 記憶體配置、物件生命週期與 UB
unit: 01 · C++ 基本功與工具（HW1）
source: 6.172 HW1 §4 C Primer（Write-up 2–4）
pdf: materials/6.172/homework/hw01/handout.pdf
read: materials/6.172/homework/hw01/handout.pdf
kind: code
minutes: 120
---

# 0111 · 記憶體配置、物件生命週期與 UB

> **實務上為什麼會遇到**：一個存 token 的 struct 少排一次欄位順序，整個語料的記憶體就多一成；一個 `(a + b) / 2` 可以變成線上 crash 的根因。這題把 C primer 的 `sizeof`、指標、`swap` 改寫成會被 UBSan 當場抓包的 C++。

這題的測試都在 ASan/UBSan 下跑。**任何 undefined behaviour 都會讓程式直接中止**，所以「看起來會動」不算過關。

## Stage 1 — Struct layout 計算機

給一串欄位（`FieldSpec{size, align}`，`align` 是 2 的次方，`size` 是 `align` 的倍數），依 C++ 的規則算出位移：每個欄位放在目前游標往上對齊到 `align` 的位置；整個 struct 的對齊是最大的欄位對齊；總大小要往上補到整個 struct 的對齊。

| 介面 | 行為 |
| --- | --- |
| `compute_layout(fields)` | 回傳 `StructLayout{offsets, size, align}`；沒有欄位時 `size == 1`、`align == 1`（空 struct 也要有大小）；`size` 不是 `align` 倍數時丟 `std::invalid_argument` |
| `padding_bytes(fields)` | 總大小減掉所有欄位 `size` 的總和 |
| `packed_order(fields)` | 回傳 `0..n-1` 的一個排列，照這個順序排欄位時 `compute_layout` 的總大小最小 |

## Stage 2 — 指標算術：Bump Arena

```cpp
class BumpArena {
public:
    explicit BumpArena(std::span<std::byte> buffer);
    void* allocate(std::size_t bytes, std::size_t alignment);
    void reset();
    std::size_t used() const;       // 已消耗的 byte 數，含 padding
    std::size_t capacity() const;
};
```

- `alignment` 不是 2 的次方（含 0）丟 `std::invalid_argument`。
- 回傳的指標要滿足 `alignment`，而且 `[p, p + bytes)` 完全落在 `buffer` 內。放不下回傳 `nullptr`，且**內部狀態不變**（之後比較小的配置要還能成功）。
- `bytes == 0` 也要回傳對齊的合法指標。
- 把 `bytes` 接近 `SIZE_MAX` 傳進來時，不可以因為指標加法溢位而算出一個「看起來放得下」的位置（這本身就是 UB，會被 UBSan 抓到）。

## Stage 3 — 物件生命週期：StaticVector

做一個容量固定 `N`（`N >= 1`）的 `StaticVector<T, N>`，元素放在物件內部的 raw storage，**不可以**宣告 `T data_[N]`（那會直接預設建構 N 個元素，也無法存放沒有預設建構子的型別）。

介面看 `solution.hpp`。行為：

- `push_back` / `emplace_back` 滿了丟 `std::length_error`，回傳新元素的參考。
- `pop_back` / `operator[]` 用錯（空的時候 `pop_back`）丟 `std::out_of_range`。`data()`／迭代器要是連續的 `T*`。
- 建構子丟例外時：沒有洩漏，`size` 不變（`emplace_back` 失敗就像沒發生過）。
- 複製建構子複製到一半丟例外時，已複製的要銷毀（RAII）。
- 移動建構／移動指派之後來源變成空（`size() == 0`）。
- 析構要銷毀所有存活元素；型別過度對齊（`alignas(64)`）時 `data()` 也要對齊。

提示：`std::construct_at`、`std::destroy_at`、`std::launder`。

## Stage 4 — 不靠 UB 的整數運算

每一個最直覺的寫法都是 undefined behaviour：

| 函式 | 規格 |
| --- | --- |
| `checked_add(a, b)` / `checked_mul(a, b)` | 溢位回傳 `std::nullopt` |
| `midpoint_floor(a, b)` | 對所有 `int` 回傳 `⌊(a + b) / 2⌋` 的真實值，不可溢位 |
| `rotl32(x, k)` | 任意 `int` 的 `k`：向左旋轉 `k mod 32` 位（負值表示向右），`k == 0` 與 32 的倍數都要回傳 `x` |
| `sign_extend(value, bits)` | `value` 的低 `bits` 位（`1 <= bits <= 32`）視為二補數，符號延伸成 `int32_t` |
| `load_le32(p)` / `store_le32(p, v)` | `p` 可能沒對齊；固定 little-endian |
| `no_temp_swap(a, b)` | XOR swap，**`&a == &b` 時也要正確** |

## 面試追問

1. `char* const`、`const char*`、`char const*` 差在哪？`char**` 為什麼不能隱式轉成 `const char**`？
2. `int x[5]` 傳進 `void f(int a[])` 之後，`sizeof(a)` 是多少？這就是 array decay，C++ 用 `std::span` 怎麼避開？
3. 為什麼有號整數溢位是 UB，但無號不是？編譯器會怎麼利用這點（舉一個它因此把你的檢查優化掉的例子）？
