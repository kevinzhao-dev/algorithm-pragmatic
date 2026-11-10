---
title: 函數成長率排序
unit: 01 · Sequences（PS1）
source: 6.006 PS1 Problem 1-1
pdf: materials/6.006/psets/ps1/questions.pdf
answer: materials/6.006/psets/ps1/solutions.pdf
read: materials/6.006/lectures/lec01.pdf, materials/6.006/recitations/rec01.pdf
kind: paper
minutes: 30
---

# 0111 · 函數成長率排序

> **實務上為什麼會遇到**：面試官問「這個解能不能撐到 n = 10⁹」時，你要在 30 秒內把 `n log n`、`n^1.5`、`2^√n` 排出來。這題是在練這個反射動作。

翻到題目 PDF 第 1 頁，共有四組、每組五個函數（數學式請以 PDF 排版為準）。每一組都要由慢到快排好：如果 `f` 排在 `g` 前面，就代表 `f = O(g)`。兩個函數同階時要標出來。

## 作答

在這個資料夾新增 `answer.md`，每組寫下排序結果，以及**一句話的理由**。可以用的工具：

- 取 log 再比較（指數塔幾乎都要這樣拆）。
- Stirling：`log(n!) = Θ(n log n)`。
- 常數底數換底只差常數倍，但常數**放在指數**就不是常數倍了。

寫完再打開 `answer` 欄位指的解答對照，然後執行 `./practice done 0111`。

## 面試追問

1. `log(n^6006)` 和 `(log n)^6006`，哪個比較大？差多少階？
2. `2^n` 和 `6006^n` 為什麼不同階，但 `log₂ n` 和 `log₆₀₀₆ n` 同階？
