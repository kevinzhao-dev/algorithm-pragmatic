---
title: 用 Compiler Instrumentation 寫 Function Profiler
unit: 09 · Dynamic Analysis（HW7）
source: 6.172 HW7 §2 Checkoff 1–5（函式覆蓋率與 profiling）
pdf: materials/6.172/homework/hw07/handout.pdf
kind: code
minutes: 90
---

# 0911 · 用 Compiler Instrumentation 寫 Function Profiler

> **實務上為什麼會遇到**：perf 能看 sample，但有些問題要精確到「這個函式到底被呼叫幾次」，或者要在每次呼叫時做事（coverage、allocation attribution、tracing）。這時候就要在編譯時插 hook，LLVM XRay、gcov、各種 sanitizer 走的都是這條路。

作業用的 CSI 工具鏈在 Apple clang 上沒有。這裡用 clang 內建的 `-finstrument-functions`：編譯器會在每個被 instrument 的函式進入和離開時呼叫下面兩個 hook，你提供它們的實作：

```cpp
extern "C" void __cyg_profile_func_enter(void* this_fn, void* call_site);
extern "C" void __cyg_profile_func_exit(void* this_fn, void* call_site);
```

`support/workload.cpp` 是受測程式，只有它用 `-finstrument-functions` 編譯（看 `practice.cmake`）。Hook 和 `profiler::` 介面寫在這個資料夾的 `profiler.cpp`。`this_fn` 就是函式位址，也就是測試裡的 `&workload::fib`，可以直接當 key。

Hook 所在的檔案**沒有**被 instrument，所以在 hook 裡用 `std::unordered_map` 不會遞迴呼叫 hook，放心用。

測試是單執行緒的，不需要處理 race。

## Stage 1 — 函式覆蓋率（Checkoff 1、3、4）

`start()` 清掉資料並開始記錄，`stop()` 停止記錄（資料保留可查）；`calls(fn)` 回傳該函式在這次錄製中被進入的次數。

## Stage 2 — 報表（Checkoff 2）

`report()` 回傳這次錄製裡**真的被呼叫過**的函式，依名稱排序，名稱要 demangle（例如 `workload::fib(int)`）。提示：`dladdr` 加 `abi::__cxa_demangle`。

## Stage 3 — Profiling（Checkoff，time permitting）

`stats(fn)` 回傳 `FunctionStats`：

- `calls`：進入次數。
- `exclusive_ticks`：花在這個函式本身（不含 callee）的時間。
- `inclusive_ticks`：這個函式的 outermost activation 從進入到離開的總時間。遞迴函式不可重複累加：`f` 在 `f` 裡被呼叫時，內層那段時間不能算兩次（作業特別提醒的陷阱）。

時間來源用 `set_clock(clock)` 注入（`Clock` 是回傳 `std::uint64_t` 的函式指標；傳 `nullptr` 恢復預設的 `std::chrono::steady_clock` 奈秒）。測試用假的虛擬時鐘，所以結果是精確的整數。

## Stage 4 — 從呼叫次數估計複雜度

`estimate_growth(function, run, sizes)`：對每個 `n ∈ sizes` 錄製一次 `run(n)`，收集 `function` 的呼叫次數，用 `_shared/08_ols.hpp` 的 `practice::ols::classify_growth` 回傳複雜度分類。

測試會把它用在線性、平方、merge sort 與 fib 上。

## 面試追問

1. CSI 能拿到 basic block 和記憶體存取，而 `-finstrument-functions` 只有函式進出。各自適合做什麼工具？
2. Instrumentation 會改變被測程式的行為（時間、inlining）。Hook 的成本要怎麼量？（作業 Write-up 2）
3. 函式因為 exception 離開時，`__cyg_profile_func_exit` 不一定會被呼叫。你的 shadow stack 要怎麼處理？
