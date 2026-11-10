---
title: Sanitizer 除錯與 Crash Triage
unit: 01 · C++ 基本功與工具（HW1）
source: 6.172 HW1 §5 Basic tools（Write-up 5–8）
pdf: materials/6.172/homework/hw01/handout.pdf
read: materials/6.172/homework/hw01/handout.pdf
kind: code
minutes: 120
---

# 0112 · Sanitizer 除錯與 Crash Triage

> **實務上為什麼會遇到**：CI 跑 fuzz 一個晚上吐出 300 份 ASan 報告，其實只有 3 個 bug。會看報告、會分類、會用 lldb 重現，是 C++ 工程師的基本生存技能。這題把 HW1 的 GDB／Valgrind 流程換成這台機器上真正有的工具。

`support/planted_bugs.hpp` 有九個故意埋了 bug 的函式，都是常見手滑。先動手重現，再開始寫程式：

```bash
D=problems/6.172/0112-sanitizer-bug-hunt
clang++ -std=c++20 -g -O0 -fsanitize=address,undefined -fno-sanitize-recover=undefined \
  -fno-omit-frame-pointer -I$D/support $D/lab/bugzoo.cpp -o /tmp/bugzoo
/tmp/bugzoo use-after-realloc      # 讀 ASan 報告：哪種錯、哪一行、誰 free 的
lldb -- /tmp/bugzoo overflow       # r, bt, frame select, frame variable, p, breakpoint set -n
leaks --atExit -- /tmp/bugzoo leak # Apple Silicon 沒有 LeakSanitizer，leak 要靠 leaks
```

每個 scenario 都跑一次，把報告讀懂。注意工具的盲區：沒初始化的讀取 ASan 抓不到（要 MSan，macOS 沒有），所以養成用 RAII 和 `{}` 初始化的習慣；記憶體洩漏這台機器上要靠 `leaks`。

## Stage 1 — 解析報告

`SanitizerReport parse_report(std::string_view log)`，只看 `log` 裡**第一份**報告：

| 欄位 | 內容 |
| --- | --- |
| `bug` | `BugClass`，見下表，看不出來是 `Unknown`；完全沒有報告是 `None` |
| `tool` | `"AddressSanitizer"` 或 `"UndefinedBehaviorSanitizer"`，沒有報告時為空字串 |
| `access` / `access_size` | ASan 的 `READ of size 4`、`WRITE of size 1`；沒有時為 `""` / `0` |
| `top_frame` | ASan 第一個「有 `file:line` 位置」的 stack frame 的函式名（跳過 `__asan_memcpy`、runtime 內部 frame）；UBSan 報告為空字串 |
| `location` | `"檔名:行號"`，去掉目錄和欄號。ASan 用同一個 frame；UBSan 用開頭的位置 |

| 報告內容（節錄） | `BugClass` |
| --- | --- |
| ASan `heap-use-after-free` / `heap-buffer-overflow` / `stack-buffer-overflow` | `HeapUseAfterFree` / `HeapBufferOverflow` / `StackBufferOverflow` |
| ASan `attempting double-free` | `DoubleFree` |
| ASan `SEGV on unknown address`，位址 `< 4096` | `NullDereference`（否則 `Unknown`） |
| UBSan `signed integer overflow` | `SignedIntegerOverflow` |
| UBSan `shift exponent`、`left shift of`、`right shift of` | `ShiftOutOfRange` |
| UBSan `misaligned address` | `MisalignedAccess` |
| UBSan 含 `null pointer` | `NullDereference` |
| UBSan `out of bounds` | `OutOfBoundsIndex` |

## Stage 2 — 在子行程跑一段程式並攔截 stderr

`CaptureResult capture_stderr(std::function<void()> fn)`：用 `fork` 跑 `fn`，回傳 `{exit_code, signal, output}`。

- 正常回傳（`fn` 回傳）視為 exit code 0；被 signal 終止時 `exit_code == -1`、`signal` 為訊號編號。
- `fn` 丟出例外時子行程 exit code 為 1，**不可**讓例外往上傳到父行程的測試流程（`fork` 之後要 `_exit`）。
- `output` 是子行程寫到 stderr（fd 2）的全部內容。輸出可能很長，超過 pipe 緩衝區也不可以死結。

## Stage 3 — 現場分類

`SanitizerReport triage(std::function<void()> fn)`：在子行程跑 `fn`，解析它的報告。測試會真的讓九個 planted bug 爆在 ASan/UBSan 底下。沒有錯誤就回傳 `BugClass::None`。

## Stage 4 — 修 bug

寫修正版，同樣的輸入不可以再踩到 sanitizer（`solution.hpp` 有宣告）：`first_after_push`、`clone_prefix`（`std::unique_ptr<int[]>`）、`copy_truncated`（截斷複製，總是 NUL 結尾，回傳完整長度，`strlcpy` 語意）、`OwnedBuffer`（deep copy 的 rule of three）、`sum_wide`、`low_bits_mask_safe`（`count` 可為 0～32）、`read_u32_safe`、`second_value_safe`、`record_bucket_safe`（越界丟 `std::out_of_range`）。

## Stage 5 — 實務延伸：Crash 分組

`group_reports(std::span<const std::string> logs)`：把多份報告分組，以 `(bug, top_frame, location)` 為 key，回傳 `CrashGroup{report, count}`（`report` 為該組第一份），依 `count` 由大到小，同數再依 `location` 字典序。報告內容無法解析（`None`）的略過。

## 面試追問

1. 為什麼 ASan 抓不到讀到未初始化的記憶體？它的 shadow memory 只記錄了什麼？
2. Debug build（`-O0`）常常掩蓋 bug，有些 bug 只在 `-O2` 才現形。舉一個例子。
3. HW1 手動做的 code coverage（gcov）在什麼時候比 sanitizer 更有用？
