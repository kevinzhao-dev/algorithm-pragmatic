# algorithm-pragmatic

把 MIT 三門課的作業變成一條可以依序走完的 C++ 練功路線，目標是 frontier lab 那種「把程式寫對、一輪一輪擴充、測試和 debug」的 coding interview。

這是個人整理的非官方改編，與 MIT 無關。授權與出處見[文末](#授權與出處)。

| 順序 | 課程 | 練什麼 | 份量 |
| --- | --- | --- | --- |
| 1 | MIT 6.006 Introduction to Algorithms（Spring 2020） | 資料結構、排序、圖、動態規劃 | 80 題，約 100 小時 |
| 2 | MIT 6.046J Design and Analysis of Algorithms（Spring 2015） | 進階設計與分析 | 50 題，約 105 小時 |
| 3 | MIT 6.172 Performance Engineering（Fall 2018） | 量測、快取、平行化、allocator | 50 題，約 117 小時 |

每個單元先做「核心實作」（把講義教的資料結構親手寫一次），再做該單元的每一道官方作業題。後面的題目常會沿用你前面寫好的程式，例如 AVL tree 寫完之後，排行榜和文字緩衝區都建在它上面。

6.172 的官方先修不包含 6.046J，走完 6.006 就可以開始，兩門課也能交錯做。完整題目清單在 [ROADMAP.md](ROADMAP.md)。

## 開始練習

需要 Apple clang（或其他支援 C++20 的 clang）、CMake 和 Ninja。

```bash
./practice next
```

它會告訴你下一題是什麼、做之前該讀哪份講義、題目在哪裡。打開該題的 `solution.hpp` 實作，然後：

```bash
./practice test 0114
```

`0114` 是題目編號，也可以寫 slug 的片段，例如 `doubly`。

只想走某一門課：

```bash
./practice next --course 6.172
```

## 關卡制

每題的 `README.md` 分成幾個 stage，需求一關一關來，就像面試官在你寫完之後才加新需求。

- 前面的測試會一直跑。後面的需求弄壞前面的功能，馬上就會知道。
- 功能測試跑在 AddressSanitizer + UBSan 下，記憶體錯誤不會被放過。
- 複雜度要求由 Release 建置的效能測試把關，寫成 O(n²) 會逾時。
- 每題最後的「面試追問」要能不看程式回答。

```
6.006/0114-doubly-linked-list — Doubly Linked List（O(1) 區段搬移）

  stage 1  ✓ 10/10
  stage 2  ✗ 2/6  ← 目前關卡
  stage 3  · 尚未解鎖
```

題型有四種：`code`（實作）、`perf`（比倍數的效能工程）、`project`（多階段大題），以及 `paper`（純證明或分析，寫完對照解答後用 `./practice done <編號>` 標記）。

## 怎麼練才像面試

- 一次只讀一關，做完再往下看。
- 模擬 live coding 時關掉 AI 補全，用 README 的 `minutes` 計時，先講清楚需求和資料結構再動手。
- 卡住時先對照講義；`solutions/` 是參考解，**會直接爆雷**，把面試追問答完再看。
- 想練 agentic coding：自己先寫驗收條件，再讓 agent 實作，然後審它的 diff、補測試，最後自己說得出每個關鍵決策。

## 目錄

| 路徑 | 內容 |
| --- | --- |
| `problems/` | 題目、骨架、測試，依課程和編號排序 |
| `problems/<課程>/_shared/` | 測試共用的輔助程式。標了 `SPOILER` 的檔案是某題的解答，題目 README 會提醒 |
| `solutions/` | 參考解（爆雷） |
| `materials/` | MIT 的講義、投影片、作業 PDF 和程式包 |
| `harness/` | 測試框架，以及取代 Cilk 的 fork-join runtime |
| `docs/AUTHORING.md` | 出題規格，想自己加題目時看這份 |
| `progress.json` | 你的進度，由 `./practice` 自動更新 |

編號 `UUNN`：`UU` 是單元；`NN` 的 `01`–`09` 是核心實作，`11`–`19` 是作業題（個位數對應題號），`21` 起是延伸。

## 教材

`materials/` 包含投影片／講義和作業，沒有影片。PDF 和 ZIP 不進版控，`MANIFEST.csv` 記錄了每個檔案的來源和 SHA-256。重新下載或檢查：

```bash
python3 -I tools/fetch_materials.py
```

```bash
python3 -I tools/fetch_materials.py --verify
```

這些檔案的著作權屬 MIT 和原作者。repo 只放下載腳本，不重新散布檔案本身，出處和授權見[授權與出處](#授權與出處)。

## 環境說明

原課程的部分工具在 Apple Silicon 上不存在，所以題目做了替換：

| 原課程 | 這裡 |
| --- | --- |
| Python 範本 | C++20 |
| Cilk | `harness/forkjoin.hpp` 的 `fork2` / `parallel_for` |
| Cilksan | ThreadSanitizer |
| perf / cachegrind | 計時、計數器、確定性的快取模擬器，以及 `sample` / `xctrace` |
| Valgrind | ASan / UBSan，加上 `Tracked` 型別檢查洩漏 |
| AVX2 / x86 組語 | 編譯器向量化報告與 ARM NEON |

## 授權與出處

本 repo 的所有內容（題目、測試、參考解、工具和文件）以 [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/) 授權，條款全文見 [LICENSE](LICENSE)。你可以自由分享和改作，但要標示出處、不能用於商業目的，分享改作版本時也要用同一個授權。

題目改編自以下三門 MIT OpenCourseWare 課程，原教材同樣以 CC BY-NC-SA 4.0 授權：

- Erik Demaine, Jason Ku, and Justin Solomon. *6.006 Introduction to Algorithms*. Spring 2020. Massachusetts Institute of Technology: MIT OpenCourseWare, <https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/>. License: Creative Commons BY-NC-SA.
- Erik Demaine, Srini Devadas, and Nancy Lynch. *6.046J Design and Analysis of Algorithms*. Spring 2015. Massachusetts Institute of Technology: MIT OpenCourseWare, <https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/>. License: Creative Commons BY-NC-SA.
- Charles Leiserson and Julian Shun. *6.172 Performance Engineering of Software Systems*. Fall 2018. Massachusetts Institute of Technology: MIT OpenCourseWare, <https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/>. License: Creative Commons BY-NC-SA.

改編方式：題目敘述、介面、測試和參考解都是用 C++ 重新撰寫的。原課程的 Python 範本、Cilk 和 x86 工具換成了[環境說明](#環境說明)裡的替代品，6.172 的 Leiserchess 專案換成另一個原創遊戲。repo 不包含原課程的講義和作業檔案；用 `tools/fetch_materials.py` 下載的檔案仍適用 [OCW 使用條款](https://ocw.mit.edu/pages/privacy-and-terms-of-use/)。

這是個人學習用的非官方改編，與 MIT 和 MIT OpenCourseWare 無關，也沒有經過它們審閱或認可。
