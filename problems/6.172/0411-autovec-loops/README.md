---
title: 讓編譯器幫你向量化
unit: 04 · Vectorization（HW3）
source: 6.172 HW3（§2 Example 1–4、§3.1–3.2、Write-up 1–15）、Recitation R3
pdf: materials/6.172/homework/hw03/handout.pdf
answer: materials/6.172/recitations/r3/solutions.pdf
read: materials/6.172/homework/hw03/handout.pdf, materials/6.172/recitations/r3/problems.pdf
kind: perf
minutes: 180
---

# 0411 · 讓編譯器幫你向量化

> **實務上為什麼會遇到**：embedding 比對、影像處理、col-store 掃描，幾乎都是把同一個運算套到一長串數字上。編譯器會幫你把它變成 NEON，但只要迴圈的形狀稍微不對，它就安靜地放棄。這題練的是「怎麼讓編譯器幫你做這件事，以及怎麼確認它真的做了」。

每個 stage 都拿你的函式和同一個執行檔裡、用 `#pragma clang loop vectorize(disable) interleave(disable)` 強制純量化的版本比較（Release，`-O2`）。

**看編譯器的決定**（AVX2 換成 NEON，寬度 128-bit）：`practice.cmake` 已開 `-Rpass=loop-vectorize` 等報告。測試過一次後：

```bash
touch problems/6.172/0411-autovec-loops/tests/test.cpp
cmake --build build/release --target t_6_172_0411_autovec_loops 2>&1 | grep -E 'remark|error'
```

看 `solution.hpp` 每個迴圈的 remark，再用 `-S` 或 `sample` 看實際指令。**remark 說 vectorized 不等於有效**（Stage 3 會遇到）。

函式已經標好 `[[gnu::noinline]]`，別拿掉，否則比較會被 inlining 干擾。

## Stage 1 — Aliasing（Example 1）

`add_bytes(a, b, n)`：`a[i] += b[i]`。比純量快至少 **6 倍**。`a`、`b` 可能重疊，**語意要和循序迴圈完全一致**（`a` 在 `b` 之後時，後面的元素要看到前面寫入的結果）。編譯器會產生 runtime overlap check，所以不需要 C 的 `restrict`；C++ 沒有標準的 `restrict`，這台機器上 `__restrict` 也不見得省得掉那個檢查（用 `-S` 看）。

## Stage 2 — 用 select 取代條件寫入（Example 2）

`max_bytes(a, b, n)`：`a[i] = max(a[i], b[i])`。比純量快至少 **6 倍**。`if (b[i] > a[i]) a[i] = b[i];` 這種寫法因為「條件寫入」過不了（想想 NEON 有沒有 masked store）；要改寫成無條件的 select。

## Stage 3 — 浮點 reduction（Example 4）

- `sum_doubles(x, n)` 比純量快至少 **3 倍**，`dot_floats(a, b, n)` 至少 **4 倍**。
- 浮點加法**不滿足結合律**，所以 `y += x[i]` 預設只能照順序算：編譯器會說 vectorized，但其實還是一個個 `fadd`（看 assembly 就知道）。在函式 block 開頭用 `#pragma clang fp reassociate(on)` 明確允許重排；不要用 `-ffast-math` 蓋掉整個翻譯單元。

## Stage 4 — Data-parallel 與元素寬度（§3.1）

`template <class T> add_lanes(out, a, b, n)`：`out[i] = a[i] + b[i]`，型別為 `uint8/16/32/64_t`。128-bit NEON 暫存器放得下 16／8／4／2 個 lane，所以速度比要隨型別變窄而變高：分別至少 **8／4／2／1.4 倍**。迴圈上界是執行期參數。把運算改成 `*`，對 `uint64_t` 速度比會掉到 1（NEON 沒有 64×64 乘法），自己試。

## Stage 5 — Stride 與 SoA（§3.2.2）

`ParticleStore`：`x, y, z, vx, vy, vz` 六個分開的 `float` 欄位（SoA），`integrate(dt)` 執行 `x += vx*dt`（y、z 同）。測試用陣列結構（AoS）的 baseline：至少快 **1.4 倍**。stride 是向量化的天敵，不是因為編譯器不會，而是 AoS 的資料本來就不連續。

## Stage 6 — Reduction 進陣列（R3）

`row_sums(matrix, rows, cols, out)`（row-major）：每一列加總寫到 `out`。比純量快至少 **3 倍**。

## Stage 7 — 實務延伸：Brute-force 最近鄰

`best_match(query, matrix, dim)`：`matrix` 是 `rows × dim` 的 row-major 浮點陣列（`rows = matrix.size() / dim`，`dim > 0`），回傳內積最大的列；同分取編號最小者；沒有列回傳 `std::nullopt`。比純量快至少 **4 倍**。

## 面試追問

1. Example 1 的 assembly 把索引從 `-2^16` 加到 0，為什麼不是從 0 加到 `2^16`？C 的 `restrict` 能省掉什麼？為什麼 C++ 標準沒有它？
2. AVX2 的 aligned load/store 和 NEON 的 `ld1` 有什麼差別？對齊資料在 Apple Silicon 上還有差嗎？（HW3 Write-up 2）
3. 為什麼 `a[i] = b[i + 1]` 的迴圈用不到向量暫存器？它比 `vectorized` 慢嗎？（HW3 Write-up 4，loop idiom）
