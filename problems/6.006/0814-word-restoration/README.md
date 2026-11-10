---
title: 還原 log 的空白（切割 DP 與 Trie）
unit: 08 · Dynamic Programming II（PS8）
source: 6.006 PS8 Problem 8-4
pdf: materials/6.006/psets/ps8/questions.pdf
answer: materials/6.006/psets/ps8/solutions.pdf
read: materials/6.006/recitations/rec15.pdf, materials/6.006/recitations/rec18.pdf
kind: code
minutes: 100
---

# 0814 · 還原 log 的空白

> **實務上為什麼會遇到**：中文斷詞（jieba 的基本作法）、把 OCR 或 URL 裡黏在一起的字切開，都是「用語料統計的分數，把字串切成字」。O(L²) 次 hash lookup 只有小輸入撐得住，正式的版本要靠 trie 或 DAG 才能在長字串上快起來。

文字只有小寫英文字母和空白。**字**是不含空白的極大子字串。有一批未損壞的 log 可以統計每個字的出現次數 `f(w)`，沒出現過的字 `f(w) = 0`。另一批 log 的空白全被弄丟了，要把它還原成字的序列（串起來要等於原字串），使 `Σ f(w)` 最大。

一次只看一關，做完再往下讀。

## Stage 1 — 統計

```cpp
class WordFrequencies {
public:
    void add_text(std::string_view text);                // may be called many times
    std::int64_t count(std::string_view word) const;     // average O(|word|)
    std::size_t distinct_words() const;
    template <typename Callback>
    void for_each(Callback&& callback) const;            // callback(std::string_view word, std::int64_t count), Stage 4
};
```

`count` 拿 `string_view` 來查，不應該為了 lookup 複製字串（用 transparent hash，C++20）。連續空白、前後空白都要處理（`for_each` 留到 Stage 4 再做）。

## Stage 2 — 最佳分數

`std::int64_t best_restoration_score(const WordFrequencies& frequencies, std::string_view corrupted)`：O(L²) 次 lookup。可以切出沒見過的字（分數 0），空字串的分數是 0。

## Stage 3 — 還原

```cpp
struct Restoration { std::int64_t score; std::vector<std::string_view> words; };
Restoration restore(const WordFrequencies& frequencies, std::string_view corrupted);
```

回傳一組使分數最大的切法。`words` 指向 `corrupted` 的內容（注意生命週期）。

## Stage 4 — 長文字用 trie，同分取字數最少

```cpp
class FastRestorer {
public:
    explicit FastRestorer(const WordFrequencies& frequencies);
    Restoration restore(std::string_view corrupted) const;
};
```

- 建構時把所有字放進 trie（一個節點 26 個 child，存在 `std::vector` 裡而不是一堆 `new`）。
- `restore` 在 O(L · W + L) 時間完成，其中 `W` 是最長已知字的長度，不可以在每個位置掃所有 suffix。
- 分數相同時，選**字數最少**的切法（比如 `xyz` 全部沒見過時，切成一個字而不是三個）。

## 面試追問

1. 為什麼每個位置要考慮的長度只到「最長已知字」？但是沒見過的字也能單獨成字，這部分怎麼處理才不會退回 O(L²)？
2. 這題是 polynomial、pseudopolynomial 還是 exponential？為什麼 `m` 在這裡不像 8-1 那樣是問題？
3. `words` 回傳 `std::string_view`，如果呼叫端傳的是暫時的 `std::string` 會發生什麼？你會怎麼讓 API 比較不容易誤用？
