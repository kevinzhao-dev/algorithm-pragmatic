# 出題規格

這份文件定義每一題的格式。想自己加題目時照著做就好。範例：

- 實作題：[`problems/6.006/0114-doubly-linked-list`](../problems/6.006/0114-doubly-linked-list/README.md)（參考解在 `solutions/` 同路徑）
- 書面題：[`problems/6.006/0111-asymptotic-ordering`](../problems/6.006/0111-asymptotic-ordering/README.md)

## 目錄

```
problems/<course>/<UUNN-slug>/
    README.md        題目
    solution.hpp     骨架，練習者要填的地方（可再加 *.cpp）
    tests/*.cpp      測試
    support/         （可選）固定輔助程式，兩種建置都會用到
    practice.cmake   （可選）額外編譯旗標，可使用 ${target}
solutions/<course>/<UUNN-slug>/
    solution.hpp     參考解，介面與骨架相同（可再加 *.cpp）
problems/<course>/_shared/   （可選）同一門課共用的固定輔助標頭
```

測試一律寫 `#include "solution.hpp"`，建置時用 include path 切換骨架或參考解，所以 `tests/` 內不能放 `solution.hpp`。

## 編號 `UUNN`

- `UU` 是單元。6.006 與 6.046J 用 PS 編號；6.172 依課程時間排列：

  | UU | 內容 | UU | 內容 |
  | --- | --- | --- | --- |
  | 01 | HW1 | 08 | HW6 |
  | 02 | HW2 | 09 | HW7 |
  | 03 | Project 1 | 10 | Project 3 |
  | 04 | HW3 | 11 | HW8 |
  | 05 | HW4 | 12 | HW9 |
  | 06 | HW5 | 13 | HW10 |
  | 07 | Project 2 | 14 | Project 4 |

- `NN`：
  - `01`–`09`：**核心實作**，把該單元 lecture / recitation 教的資料結構與演算法親手寫一次，排在作業之前。
  - `11`–`19`：**作業題**，個位數對應題號，例如 PS1 的 Problem 1-4 是 `0114`。
  - `21`–`29`：同一單元中放不下的延伸或拆分題。

## README

開頭是 front matter，一行一個 `key: value`：

| 欄位 | 必要 | 說明 |
| --- | --- | --- |
| `title` | 是 | 短標題 |
| `unit` | 是 | 例如 `01 · Sequences（PS1）`，同單元要完全相同，ROADMAP 用它分組 |
| `source` | 是 | 例如 `6.006 PS1 Problem 1-4`；核心實作寫 `6.006 Recitation 2` |
| `pdf` | 是 | 原始題目或講義 PDF，相對 repo 根目錄 |
| `answer` | 否 | 官方解答 PDF |
| `read` | 否 | 做題前該讀的 PDF，逗號分隔 |
| `kind` | 是 | `code`、`perf`、`project` 或 `paper` |
| `minutes` | 是 | 預估分鐘數，要誠實 |
| `sanitizer` | 否 | 設成 `tsan` 時用 ThreadSanitizer 跑功能測試 |
| `timeout` | 否 | 單次執行上限秒數，預設 60 |

內文結構：

1. 標題：`# UUNN · 名稱`
2. 一段引用：**實務上為什麼會遇到**。一兩句，要具體，講真實系統會在哪裡碰到；寧可不寫也不要硬掰。
3. 規格與 stage。把作業的小題對應成 stage，需求一關一關來，前面的測試持續有效。最後一關可以是「實務延伸」：用這題的東西做出一個小而真實的元件。
4. `## 面試追問`：做完後要能口頭回答的 2–3 題，著重取捨、失敗情境和為什麼。

語言用繁體中文（台灣用語），專有名詞保留英文。識別字與程式註解用英文，註解只寫非顯而易見的事。

## 題型

- `code`：一般實作，自動評測。**優先把題目轉成這種。** 作業裡「describe an algorithm」的設計題，都應該變成有介面、有測試的程式題。
- `perf`：效能工程題。用同一個執行檔裡的 baseline 比倍數（例如至少快 3 倍），不要寫死絕對時間。
- `project`：跨多個檔案、多個 stage 的大題。
- `paper`：只有純證明或純分析、真的沒辦法寫成程式時才用。作答寫在該題資料夾的 `answer.md`，完成後對照解答並執行 `./practice done <編號>`。

## 骨架

- `solution.hpp` 只放介面和 `PRACTICE_TODO();`，要能直接編譯，不放提示。
- `noexcept` 函式裡不能呼叫 `PRACTICE_TODO()`，否則會直接 terminate。
- 解構子不可丟例外，留空即可，由測試抓遺漏。

## 測試

用 `harness/practice.hpp` 的 `TEST`、`PERF`、`CHECK*`、`REQUIRE*`、`INFO`，以及 `Rng`、`Counted`、`Tracked`。

每題的基本要求：

- **邊界案例各有一個測試**，測試名稱描述行為，例如 `"deleting the only node empties both ends"`。
- **能比對的都要做隨機對照測試**：用暴力解或標準函式庫當模型。`Rng` 預設固定 seed，失敗可重現；用 `INFO` 帶上步數或輸入。
- **有複雜度要求就要有 `PERF`**，或用 `Counted` 數比較次數（確定性更高，優先）。`PERF` 的輸入要大到暴力解會超時至少 10 倍；預算至少是參考解實測時間的 3 倍。
- 擁有資源的型別要用 `Tracked` 檢查有沒有洩漏。Apple Silicon 沒有 LeakSanitizer。
- 測試只能使用 README 寫明的公開介面，不可依賴實作細節。
- 不可以 flaky：不依賴 `unordered_map` 走訪順序、執行緒排程或機器絕對速度。

功能測試要在 ASan/UBSan 下跑完，單題總時間最好在 10 秒內。

## 驗收

每題都要通過：

```bash
./practice audit --only '6\.006/0114'
```

它會確認骨架編譯得過、回報「尚未實作」，也會確認參考解在 ASan/UBSan 和 Release 下全部通過。參考解必須是真的實作，不能針對測試資料寫死。

關鍵題要另外確認測試抓得到錯：故意寫一個暴力或有 bug 的版本，看 `PERF` 或功能測試有沒有失敗。

## 原創性

MIT 的講義和作業放在 `materials/`，只供閱讀。**不要複製它們的文字、程式碼或測試資料**到題目、骨架、測試或參考解。

- 題目敘述用自己的話重寫，換掉故事情節和人物，改成工程情境。題號、演算法名稱、複雜度這類事實可以引用。
- 介面、測試和參考解都自己設計與撰寫。
- 細節或數學排版太複雜時，指向 PDF 的頁碼，不要轉錄。

## 環境

macOS arm64、Apple clang 21、libc++、C++20，只用標準函式庫。

- 沒有 `perf`、`valgrind`、`cachegrind`、`gdb`、`awsrun`。可用 `lldb`、`leaks`、`sample`、`xctrace`，以及 clang 的 `-Rpass`。
- 沒有 Cilk：用 [`harness/forkjoin.hpp`](../harness/forkjoin.hpp) 的 `fork2` 和 `parallel_for`。
- x86 組語或 AVX 的內容：改寫成可攜 C++ 搭配編譯器報告，或改用 ARM NEON。
- 快取行為：用確定性的快取模擬器，或用 baseline 比倍數。
