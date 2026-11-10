---
title: Arena Allocator（bump → implicit → binned）
unit: 10 · Memory Allocator（Project 3）
source: 6.172 Project 3
pdf: materials/6.172/projects/p3/handout.pdf
read: materials/6.172/lectures/lec11.pdf, materials/6.172/lectures/lec12.pdf, materials/6.172/homework/hw06/handout.pdf, materials/6.172/recitations/r6/problems.pdf
kind: project
minutes: 780
---

# 1011 · Arena Allocator

> **實務上為什麼會遇到**：服務的尖峰記憶體常常不是因為資料真的那麼多，而是碎片加上 metadata。同一份 trace 上 allocator 的 utilization 差 20%，就是伺服器帳單差 20%；resident set 爆掉的 on-call，最後常常追到這裡。

在一塊 `mmap` 來的固定 arena 上實作三代 allocator，全部是 `malloc` / `free` / `realloc` 的語意，再用 trace 量空間和速度。`support/` 已備好：`trace.hpp`（seeded trace，七種 workload shape）、`replay.hpp`（黑箱 validator：每個 byte 都寫入 pattern 再驗，抓 overlap、misalignment、資料遺失）。

## 全部 allocator 共用的合約

- `malloc(n)`：回傳 16-byte aligned 的指標；`n == 0` 回傳 `nullptr`；放不下就回傳 `nullptr` 且不留下痕跡。`n` 可能接近 `SIZE_MAX`，**小心 overflow**。
- `free(p)`：`p == nullptr` 無作用。
- `realloc(p, n)`：`p == nullptr` 等同 `malloc(n)`；`n == 0` 等同 `free(p)` 並回傳 `nullptr`；前 `min(舊大小, n)` bytes 必須保留；失敗時回傳 `nullptr`，`p` 完好。
- `check() const`：一致性檢查。堅固到**給它損毀的 heap 也不會 crash 或無窮迴圈**，只回傳 `false`。
- `heap_bytes()` 是目前用掉的 arena bytes；`arena()` 回傳 `const Arena&`（validator 用它確認指標在 heap 內）。

所有 metadata 都要放在 arena 裡：validator 看到你在 `malloc`/`free`/`realloc` 裡呼叫 `operator new` 就判錯；物件本身不超過 8 KiB。

## Stage 1 — Arena 與 bump allocator

`Arena`：建構子 `mmap` 匿名區段（capacity 往上取整到 page，0 也至少一頁），解構子 `munmap`，只能 move。`grow(n)` 是 sbrk：回傳舊的尾端，塞不下就回傳 `nullptr` 且 `used()` 不變。

`BumpAllocator`：只往前推，`free` 什麼都不做。`realloc` 最近一塊要原地長大（別拖累 utilization）；其他情況配置新的再 copy，所以每個區塊要記得自己的大小。

## Stage 2 — Implicit free list

**區塊格式是公開合約，測試會直接讀寫：**

- 每個區塊（已配置或 free）開頭都是 8-byte header，緊接在 `payload − 8`。bit 0 = 已配置；bits 4 以上是**整個區塊**的大小（16 的倍數，至少 32）；bits 1–3 歸你用。
- 區塊在位址上連續：下一塊的 payload 在 `payload + size`。heap 尾端可以有一個 size 0 的 sentinel。
- 任何時刻都沒有兩個相鄰的 free block。

`ImplicitAllocator`：把所有區塊串成 implicit list 走訪，在裡面找 fit、切割（剩下夠大就切），`free` 立刻 coalesce 前後鄰居（boundary tag）。`realloc` 能原地就原地：縮小不搬；往後一塊 free 的併；位在 heap 尾端就直接 `grow`。

`check()` 走完整個 heap，要能抓到：size 被清成 0、size 指出 heap 外、size 被改了讓走訪跑到 payload 中間、相鄰 free block。

## Stage 3 — Binned free lists

`BinnedAllocator`：同一個 layout，但 free block 依大小掛進 bins，`malloc` 要接近 O(1)：**malloc 成本不能隨 free block 數量成長**（有 `PERF` 擋）。free block 的 `next` / `prev` links 要放在 payload 最前面 16 bytes，所以寫過已釋放記憶體的 use-after-free 會打壞它們，`check()` 要抓得到。

`check()` 再加上：free block 必須真的在 list 裡、list 裡的都要標記為 free、list 的指標要指在 heap 內並彼此一致。在 list 裡繞圈也不能卡死。

## Stage 4 — 計分

用 `replay.hpp` 的 trace 比較，要過下列門檻（前兩項不靠時脈，是確定性的）：

- 每種 shape 的 **utilization**（`peak_live / heap_bytes`）都要高於固定 floor（`tests/test.cpp` 的 `kFloors`）。
- 七種 shape 的 utilization 幾何平均不輸 implicit list 太多（≥ 0.92 倍），並且遠勝 bump。
- 同一支執行檔裡對基準線的**throughput**：比 implicit list 快至少 5 倍（`long-lived`）、每種 shape 至少是 libc `malloc` 的 1/3。
- 綜合指數 `P = √(U × min(1, T/T_libc))` 平均要高於 implicit list，且 ≥ 0.6。

想看每一種 shape 的成績單（`scorecard` 這個 `PERF`）：`./practice test 1011` 建置後執行 `build/release/bin/6.172/1011-arena-allocator/run --perf-only --stage 4 --verbose`。

## 面試追問

1. 已配置區塊沒有 footer，`free` 時要怎麼知道前一塊是不是 free？省下這 8 bytes 在小物件 workload 上大概值多少 utilization？
2. `realloc` 不能原地擴張時，舊區塊在被釋放前 heap 同時有新舊兩份。為什麼這會推高 `heap_bytes`？怎麼讓這件事少發生？
3. 把這顆 allocator 給多執行緒用：全域一把鎖的瓶頸在哪？per-thread cache 為什麼可能讓 false sharing 和跨執行緒釋放出問題？（lec12）
