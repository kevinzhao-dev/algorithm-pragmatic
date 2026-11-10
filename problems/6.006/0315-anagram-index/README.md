---
title: Anagram Substring Index (sliding-window fingerprints)
unit: 03 · Hashing & Linear Sorting (PS3)
source: 6.006 PS3 Problem 3-5
pdf: materials/6.006/psets/ps3/questions.pdf
answer: materials/6.006/psets/ps3/solutions.pdf
read: materials/6.006/lectures/lec04.pdf
kind: code
minutes: 60
---

# 0315 · Anagram Substring Index

**Original problem:** [Problem Set 3 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/03a34fcf3012a242ffcae5534e9fa511_MIT6_006S20_ps3-questions.pdf) · **Read first:** [Lecture 4: Hashing notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/ce9e94705b914598ce78a00a70a1f734_MIT6_006S20_lec4.pdf)

> **Why this comes up in practice:** the k-mer composition of a gene sequence and the bag-of-words fingerprints used in plagiarism detection both maintain character statistics for every length-k window of a long string. There is only one key move: when the window slides by one, update in O(1) instead of recomputing.

## Original problem

6.006 PS3 Problem 3-5 works with strings made only of the lowercase English letters a to z. A string A is an anagram of a string B when A's letters are a rearrangement of B's; for example, indicatory and dictionary are anagrams, and so are brush and shrub. The anagram substring count of B in A counts how many contiguous substrings of A are anagrams of B. For example, with A = esleastealaslatet and B = tesla, 3 of the 13 substrings of A with length 5 (least, steal, and slate) are anagrams of B, so the count is 3.

- (a) For a string A and a positive integer k, design a structure that takes O(|A|) time to build and then answers one kind of query in O(k) time: for a string B of length k, report the anagram substring count of B in A.
- (b) For a string T and n strings S_0, …, S_{n−1} of length k, where 0 < k < |T|, compute the anagram substring count of every S_i in T in O(|T| + nk) total time.
- (c) Implement (b) in the Python template as `count_anagram_substrings(T, S)`, returning the counts as a tuple. Python's `ord(c)` gives a character's ASCII code in O(1) time.

## Your task

All strings contain only the lowercase English letters `a`–`z`. A string `A` is an anagram of `B` when the two have the same multiset of letters. The anagram substring count of `B` in `A` is how many of the **contiguous** substrings of `A` with length `|B|` are anagrams of `B`.

Stage 1 builds the data structure of part (a) as a class, Stage 2 is part (b) in C++ rather than Python, and Stage 3 adds streaming appends, which the original doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — The index

```cpp
class AnagramIndex {
public:
    AnagramIndex(std::string_view text, std::size_t k);
    std::size_t count(std::string_view query) const;
};
```

`AnagramIndex(text, k)`: `k ≥ 1` (otherwise throw `std::invalid_argument`). Construction takes expected O(|text|).

- `count(query)`: `query` must have length `k` (otherwise throw `std::invalid_argument`). Return the anagram substring count in expected O(k).
- `k` may be longer than `text`, in which case the answer to every query is 0; when `k == |text|`, there is exactly one window.

The core idea: map each length-k window to a fixed-size table of character frequencies, and store that table as a key in a hash map. When the window slides by one, only two entries of the table change, which is O(1).

You need to write a good enough hash for `std::array<std::uint32_t, 26>`.

## Stage 2 — Batch queries

```cpp
inline std::vector<std::size_t> count_anagram_substrings(std::string_view text,
                                                         std::span<const std::string> queries);
```

`count_anagram_substrings(text, queries)`: the strings in `queries` all have the same length (k). Return the answer to each query, in expected O(|text| + n·k) total time. If `queries` is empty, return an empty vector.

## Stage 3 — Real-world extension: streaming appends

```cpp
// in class AnagramIndex
void append(char ch);
std::size_t length() const;
```

The text is a log that keeps arriving, so the index must be able to update online:

- `append(ch)`: append one character to the end of `text` and update the index at the same time. Amortized expected O(1).
- `length()`: the current length of the text.

After an append, `count` must equal the result of rebuilding from the full text. To know in O(1) time which old character leaves the window, you only need to keep the last `k` characters, not the whole `text`; use O(k) extra space (not counting the hash map).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. What happens if you replace the frequency table with a shorter key, such as the "sum of ord"? (Look at `ad` and `bc`.)
2. If you save memory by using a 64-bit hash of the frequency table as the key, what does it cost? How would you estimate the probability of a collision?
3. If the character set becomes all of Unicode, can the frequency table still be O(1)? What data structure fits?

---

**Official solution:** [Problem Set 3 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9aede1539e086a709a75d604a89f08b6_MIT6_006S20_ps3-solutions.pdf). Read it after you finish.
