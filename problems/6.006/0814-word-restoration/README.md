---
title: Restoring the Spaces in Logs (segmentation DP and tries)
unit: 08 · Dynamic Programming II (PS8)
source: 6.006 PS8 Problem 8-4
pdf: materials/6.006/psets/ps8/questions.pdf
answer: materials/6.006/psets/ps8/solutions.pdf
read: materials/6.006/recitations/rec15.pdf, materials/6.006/recitations/rec18.pdf
kind: code
minutes: 100
---

# 0814 · Restoring the Spaces in Logs

**Original problem:** [Problem Set 8 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/740643e3711574870a83ebfceee9a8ae_MIT6_006S20_ps8_questions.pdf) · **Read first:** [Recitation 15 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6022dc36557c5e5df2ad0a77e9463855_MIT6_006S20_r15.pdf), [Recitation 18 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/e88f4499b351ab82ebf07c4de2859272_MIT6_006S20_r18.pdf)

> **Why this comes up in practice:** Chinese word segmentation (the basic approach in jieba) and splitting words that run together in OCR output or URLs all "cut a string into words using scores from corpus statistics". O(L²) hash lookups only hold up on small inputs; a production version needs a trie or a DAG to be fast on long strings.

## Original problem

6.006 PS8 Problem 8-4 is set aboard a starship. Every day the captain uploads a captain's log: a string of at most m characters, each a lowercase English letter or a space, in which a word is any maximal run of non-space characters. After the captain is abducted, an officer goes through the logs for evidence, but a fault in the upload system has stripped every space out of the last n logs. The officer wants to put the spaces back using the captain's word habits from earlier logs. Given the list L_c of the n corrupted logs and a list L_u of O(m²n) intact logs from before the fault, the officer wants to:

- count, for every word w found in L_u, its number of occurrences f(w) there (f(w) is 0 for a word that never occurs in L_u); and
- restore each corrupted log ℓ_i ∈ L_c as a sequence of words R_i, called a restoration, whose in-order concatenation is ℓ_i and which maximizes Σ_{w∈R_i} f(w) among all restorations of ℓ_i.

The problem asks for an O(m³n)-time algorithm that carries out this whole procedure. Like every PS8 problem, it must be solved with a dynamic program presented through the SRT BOT steps, and you must also say whether the requested running time is polynomial, pseudopolynomial, or exponential in the size of the input. PS8 has no coding part.

## Your task

The text contains only lowercase English letters and spaces. A **word** is a maximal substring without spaces. One batch of undamaged logs lets you count how many times each word occurs, `f(w)`, with `f(w) = 0` for a word that never occurs. Another batch of logs has lost all of its spaces; restore each of them as a sequence of words (which must concatenate back to the original string) that maximizes `Σ f(w)`.

This is the original problem, one corrupted log at a time. Beyond the best score and a restoration that reaches it, Stage 4 builds a trie so that long logs run in O(L · W + L) time, and breaks ties in favor of fewer words.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Counting

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

`count` looks words up by `string_view` and shouldn't copy the string just to do a lookup (use a transparent hash, C++20). Handle runs of spaces as well as leading and trailing spaces (leave `for_each` until Stage 4; it must call `callback` exactly once for each distinct word, with that word's count).

## Stage 2 — The best score

```cpp
std::int64_t best_restoration_score(const WordFrequencies& frequencies,
                                    std::string_view corrupted);
```

`std::int64_t best_restoration_score(const WordFrequencies& frequencies, std::string_view corrupted)`: O(L²) lookups. You may cut out words that were never seen (they score 0), and the empty string scores 0.

## Stage 3 — Restoring

```cpp
struct Restoration { std::int64_t score; std::vector<std::string_view> words; };
Restoration restore(const WordFrequencies& frequencies, std::string_view corrupted);
```

Return a segmentation with the highest score. `words` points into the contents of `corrupted` (mind the lifetimes).

## Stage 4 — A trie for long text, and the fewest words on ties

```cpp
class FastRestorer {
public:
    explicit FastRestorer(const WordFrequencies& frequencies);
    Restoration restore(std::string_view corrupted) const;
};
```

- The constructor puts every word into a trie (26 children per node, stored in a `std::vector` rather than as a pile of `new` allocations).
- `restore` finishes in O(L · W + L) time, where `W` is the length of the longest known word; it must not scan every suffix at every position.
- When scores tie, choose the segmentation with **the fewest words** (for example, when nothing in `xyz` has been seen, keep it as one word rather than three).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does each position only need to consider lengths up to "the longest known word"? Unseen words can still stand as words of their own, though, so how do you handle them without falling back to O(L²)?
2. Is this problem polynomial, pseudopolynomial, or exponential? Why isn't `m` a problem here the way it is in 8-1 (exercise 0811)?
3. `words` holds `std::string_view`s. What happens if the caller passes a temporary `std::string`? How would you make the API harder to misuse?

---

**Official solution:** [Problem Set 8 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/39b3ead18e480e4322a8a7308daf8fa1_MIT6_006S20_ps8_solutions.pdf). Read it after you finish.
