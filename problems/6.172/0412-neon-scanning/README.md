---
title: NEON 手寫掃描（AVX2 換成 NEON）
unit: 04 · Vectorization（HW3）
source: 6.172 HW3（§3.1.3 Vector flavors，AVX2 轉 NEON）
pdf: materials/6.172/homework/hw03/handout.pdf
read: materials/6.172/homework/hw03/handout.pdf
kind: perf
minutes: 120
---

# 0412 · NEON 手寫掃描

> **實務上為什麼會遇到**：`memchr`、JSON／CSV parser、字串比對的內層迴圈有 early exit，編譯器的自動向量化幫不上忙，正是 simdjson 這類函式庫要手寫 SIMD 的原因。Apple Silicon 上的工具是 NEON，它和 AVX2 最大的差別是：**沒有 `movemask`**。

這題用 `<arm_neon.h>` 的 intrinsics（`vld1q_u8`、`vceqq_u8`、`vdupq_n_u8`、`vandq_u8`…）。每一題的共同規則：

- **不可讀出 buffer 之外**：剩下不到 16 byte 的尾巴要用純量處理。測試用剛好大小的配置跑 ASan，多讀一個 byte 就會被抓。
- 比起同一支執行檔裡的純量迴圈（`#pragma clang loop vectorize(disable)`）要有明顯加速（下面每關有倍數）。
- 找不到時回傳 `npos`（定義為 `haystack.size()`）。

## Stage 1 — `find_byte(haystack, needle)`

回傳第一個等於 `needle` 的索引。比純量快至少 **4 倍**（長且沒有符合項的 buffer）。

沒有 `movemask` 怎麼辦？用 `vshrn_n_u16(vreinterpretq_u16_u8(eq), 4)` 把 16 個 mask byte 壓成 64 位元（每個 byte 對應 4 位元），然後算 trailing zeros 除以 4。

## Stage 2 — `count_byte(haystack, needle)`

回傳出現次數，比純量快至少 **4 倍**。每個 lane 的比較結果是 `0x00` 或 `0xFF`。

## Stage 3 — `to_lower_ascii(text)`

只把 `'A'..'Z'` 轉小寫，其他 byte（含 `>= 0x80` 的非 ASCII）一律不變。比純量快至少 **4 倍**。沒有 branch：用 mask 加上 `0x20`。

## Stage 4 — `find_any_of3(haystack, a, b, c)`

第一個等於 `a`、`b` 或 `c` 的索引，比純量快至少 **3 倍**。

## Stage 5 — 實務延伸：CSV 欄位切割

`split_fields(std::string_view line, char sep)`：回傳每個欄位的 `std::string_view`（空欄位也要算；`""` 切成一個空欄位；沒有 quote 處理）。用前面的 `find_byte`。

## 面試追問

1. 為什麼有 early exit 的迴圈編譯器不敢向量化？（提示：讀到第 `i` 個 byte 才知道要不要繼續，讀過頭可能跨到不該讀的 page）
2. AVX2 的 `_mm256_movemask_epi8` 在 NEON 上為什麼這麼麻煩？`vshrn` 法有什麼代價？
3. 256-bit AVX2 一次看 32 byte，NEON 只有 16，為什麼 Apple Silicon 的 `memchr` 還是這麼快？（提示：解碼寬度與快取）
