# Roadmap

由 `./practice roadmap` 依各題 README 的資料產生，請勿手動修改。

## MIT 6.006 — Introduction to Algorithms (Spring 2020)

80 題，估計約 102 小時。


### 00 · Warm-up（PS0）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0011 | [Sorted Set 的交、聯、差（Posting List）](problems/6.006/0011-sorted-set-algebra/README.md) | 實作 | 50 | 6.006 PS0 Problem 0-1 |
| 0012 | [精確的期望值（Fraction 與離散分布）](problems/6.006/0012-exact-expectation/README.md) | 實作 | 60 | 6.006 PS0 Problem 0-2 |
| 0013 | [同餘運算（floor mod、模冪、週期撞期）](problems/6.006/0013-modular-arithmetic/README.md) | 實作 | 50 | 6.006 PS0 Problem 0-3 |
| 0014 | [把歸納法寫成程式（立方和與多項式驗證）](problems/6.006/0014-induction-checker/README.md) | 實作 | 55 | 6.006 PS0 Problem 0-4 |
| 0015 | [判斷樹與 Leaf Peeling（歸納法變演算法）](problems/6.006/0015-tree-leaf-peeling/README.md) | 實作 | 60 | 6.006 PS0 Problem 0-5 |
| 0016 | [最長遞增區段的計數（串流版）](problems/6.006/0016-longest-increasing-runs/README.md) | 實作 | 35 | 6.006 PS0 Problem 0-6 |

### 01 · Sequences（PS1）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0101 | [Static Array 與 Array Sequence](problems/6.006/0101-array-sequence/README.md) | 實作 | 70 | 6.006 Lecture 1, Recitation 1-2 |
| 0102 | [Singly Linked List 與 Floyd 環偵測](problems/6.006/0102-singly-linked-list/README.md) | 實作 | 80 | 6.006 Recitation 2 |
| 0103 | [Dynamic Array（攤提分析與 Exception Safety）](problems/6.006/0103-dynamic-array/README.md) | 實作 | 120 | 6.006 Lecture 2, Recitation 2 |
| 0104 | [環狀 Deque 與滑動視窗限流器](problems/6.006/0104-ring-deque/README.md) | 實作 | 70 | 6.006 Lecture 2, Recitation 2 |
| 0111 | [函數成長率排序](problems/6.006/0111-asymptotic-ordering/README.md) | 書面 | 30 | 6.006 PS1 Problem 1-1 |
| 0112 | [在 Sequence 介面上做批次操作](problems/6.006/0112-sequence-reductions/README.md) | 實作 | 50 | 6.006 PS1 Problem 1-2 |
| 0113 | [雙游標行緩衝區（Binder Bookmarks）](problems/6.006/0113-two-cursor-buffer/README.md) | 實作 | 90 | 6.006 PS1 Problem 1-3 |
| 0114 | [Doubly Linked List（O(1) 區段搬移）](problems/6.006/0114-doubly-linked-list/README.md) | 實作 | 75 | 6.006 PS1 Problem 1-4 |

### 02 · Sorting & Recurrences（PS2）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0201 | [Sorted Array Set（Binary Search 與順序查詢）](problems/6.006/0201-sorted-array-set/README.md) | 實作 | 75 | 6.006 Lecture 3, Recitation 3 |
| 0202 | [Selection Sort、Insertion Sort 與 Top-K Tracker](problems/6.006/0202-elementary-sorts/README.md) | 實作 | 55 | 6.006 Lecture 3, Recitation 3 |
| 0203 | [Merge Sort 與反序數](problems/6.006/0203-merge-sort/README.md) | 實作 | 70 | 6.006 Lecture 3, Recitation 3 |
| 0211 | [Recurrence 求解器（Master Theorem 與遞迴樹）](problems/6.006/0211-recurrence-solver/README.md) | 實作 | 60 | 6.006 PS2 Problem 2-1 |
| 0212 | [依成本模型選排序法](problems/6.006/0212-sorting-sorts/README.md) | 實作 | 45 | 6.006 PS2 Problem 2-2 |
| 0213 | [Exponential Search 與 Finger Cursor](problems/6.006/0213-exponential-search/README.md) | 實作 | 70 | 6.006 PS2 Problem 2-3 |
| 0214 | [聊天室與封鎖（組合資料結構）](problems/6.006/0214-chat-moderation/README.md) | 實作 | 80 | 6.006 PS2 Problem 2-4 |
| 0215 | [資源預約排程（Merge、分治、線上版本）](problems/6.006/0215-reservation-schedules/README.md) | 實作 | 100 | 6.006 PS2 Problem 2-5 |

### 03 · Hashing & Linear Sorting（PS3）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0301 | [Chained Hash Table（universal hashing）](problems/6.006/0301-chained-hash-table/README.md) | 實作 | 75 | 6.006 Lecture 4, Recitation 4 |
| 0302 | [Open Addressing（linear probing 與刪除）](problems/6.006/0302-open-addressing/README.md) | 實作 | 70 | 6.006 Lecture 4, Recitation 4 |
| 0303 | [Counting、Tuple 與 Radix Sort](problems/6.006/0303-counting-radix-sort/README.md) | 實作 | 75 | 6.006 Lecture 5, Recitation 5 |
| 0311 | [Hash Practice（程式化手算）](problems/6.006/0311-hash-practice/README.md) | 實作 | 30 | 6.006 PS3 Problem 3-1 |
| 0312 | [攻擊 Hash Family（誰能保證碰撞）](problems/6.006/0312-hash-family-attack/README.md) | 實作 | 50 | 6.006 PS3 Problem 3-2 |
| 0313 | [依鍵的型態選排序法](problems/6.006/0313-sort-by-key-kind/README.md) | 實作 | 50 | 6.006 PS3 Problem 3-3 |
| 0314 | [距離限制的兩數和（close pair）](problems/6.006/0314-close-pair/README.md) | 實作 | 60 | 6.006 PS3 Problem 3-4 |
| 0315 | [Anagram 子字串索引（滑動視窗指紋）](problems/6.006/0315-anagram-index/README.md) | 實作 | 60 | 6.006 PS3 Problem 3-5 |

### 04 · Trees & Heaps（PS4）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0401 | [Binary Tree 與 Traversal Order](problems/6.006/0401-binary-search-tree/README.md) | 實作 | 100 | 6.006 Lecture 6, Recitation 6 |
| 0402 | [AVL Tree 引擎（rotation 與 subtree augmentation）](problems/6.006/0402-avl-tree/README.md) | 實作 | 150 | 6.006 Lecture 7, Recitation 7 |
| 0403 | [AVL Set／Map（order statistics）](problems/6.006/0403-avl-map/README.md) | 實作 | 80 | 6.006 Lecture 7, Recitation 7 |
| 0404 | [Sequence AVL（索引與區間 fold）](problems/6.006/0404-avl-sequence/README.md) | 實作 | 80 | 6.006 Lecture 7, Recitation 7 |
| 0405 | [Binary Heap 與 In-Place Heapsort](problems/6.006/0405-binary-heap/README.md) | 實作 | 75 | 6.006 Lecture 8, Recitation 8 |
| 0406 | [可更新優先權的 Heap（handle）](problems/6.006/0406-updatable-heap/README.md) | 實作 | 75 | 6.006 Lecture 8, Recitation 8 |
| 0411 | [Binary Tree Practice（程式化手算）](problems/6.006/0411-binary-tree-practice/README.md) | 實作 | 45 | 6.006 PS4 Problem 4-1 |
| 0412 | [Heap Practice（分類與 swap trace）](problems/6.006/0412-heap-practice/README.md) | 實作 | 30 | 6.006 PS4 Problem 4-2 |
| 0413 | [Top-K（heap 的三種用法）](problems/6.006/0413-top-k-rewards/README.md) | 實作 | 50 | 6.006 PS4 Problem 4-3 |
| 0414 | [容量排程（heap＋hash 的組合）](problems/6.006/0414-capacity-placement/README.md) | 實作 | 75 | 6.006 PS4 Problem 4-4 |
| 0415 | [矩陣連乘的即時更新（augmented sequence）](problems/6.006/0415-transform-chain/README.md) | 實作 | 55 | 6.006 PS4 Problem 4-5 |
| 0416 | [最佳象限切片（max-prefix 擴充）](problems/6.006/0416-dominance-slice/README.md) | 實作 | 90 | 6.006 PS4 Problem 4-6 |

### 05 · Graph Search（PS5）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0501 | [Graph 表示法與 BFS](problems/6.006/0501-graph-and-bfs/README.md) | 實作 | 90 | 6.006 Lecture 9, Recitation 9 |
| 0502 | [DFS、拓撲排序與環偵測](problems/6.006/0502-dfs-toposort/README.md) | 實作 | 100 | 6.006 Lecture 10, Recitation 10 |
| 0511 | [Graph 手算題轉成程式（表示法、BFS/DFS 軌跡、單邊解環）](problems/6.006/0511-graph-practice/README.md) | 實作 | 60 | 6.006 PS5 Problem 5-1 |
| 0512 | [故障域分析（誰的備援最划算、一條線斷掉會怎樣）](problems/6.006/0512-failure-domains/README.md) | 實作 | 80 | 6.006 PS5 Problem 5-2 |
| 0513 | [兩個維護窗口（有解就給排法、沒解就給證據）](problems/6.006/0513-two-windows/README.md) | 實作 | 80 | 6.006 PS5 Problem 5-3 |
| 0514 | [機房走線（沿著磁磚邊界的最短路徑）](problems/6.006/0514-cable-tray-route/README.md) | 實作 | 70 | 6.006 PS5 Problem 5-4 |
| 0515 | [倉儲機器人的取貨任務（門禁、識別證、來回最短）](problems/6.006/0515-robot-errand/README.md) | 實作 | 110 | 6.006 PS5 Problem 5-5 |
| 0516 | [傾斜托盤（狀態空間搜尋）](problems/6.006/0516-tilt-tray/README.md) | 實作 | 120 | 6.006 PS5 Problem 5-6 |

### 06 · Weighted Shortest Paths（PS6）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0601 | [DAG Relaxation、最短路徑樹與 Critical Path](problems/6.006/0601-dag-relaxation/README.md) | 實作 | 100 | 6.006 Lecture 11, Recitation 11 |
| 0602 | [Bellman-Ford（負環偵測與回報）](problems/6.006/0602-bellman-ford/README.md) | 實作 | 110 | 6.006 Lecture 12, Recitation 12 |
| 0603 | [Changeable Priority Queue 與 Dijkstra](problems/6.006/0603-indexed-heap-dijkstra/README.md) | 實作 | 120 | 6.006 Lecture 13, Recitation 13 |
| 0611 | [手算 Dijkstra 轉成程式（relaxation 順序與合法性檢查）](problems/6.006/0611-relaxation-order/README.md) | 實作 | 60 | 6.006 PS6 Problem 6-1 |
| 0612 | [距離向量路由（幾跳就收斂的最短路徑）](problems/6.006/0612-few-hop-routing/README.md) | 實作 | 70 | 6.006 PS6 Problem 6-2 |
| 0613 | [重力管線的一次性閥門（狀態改變地形）](problems/6.006/0613-one-time-valve/README.md) | 實作 | 80 | 6.006 PS6 Problem 6-3 |
| 0614 | [守恆的狀態機（每個迴圈的點數差都必須為 0）](problems/6.006/0614-conservative-ledger/README.md) | 實作 | 90 | 6.006 PS6 Problem 6-4 |
| 0615 | [電動車路線規劃（電量也是座標的一部分）](problems/6.006/0615-ev-route-planner/README.md) | 實作 | 110 | 6.006 PS6 Problem 6-5 |
| 0616 | [Johnson's Algorithm（全點對最短路徑與路由表）](problems/6.006/0616-johnson-apsp/README.md) | 實作 | 110 | 6.006 PS6 Problem 6-6, Lecture 14 |

### 07 · Dynamic Programming I（PS7）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0701 | [Memo 與 Bowling（SRT BOT 線性 DP）](problems/6.006/0701-memo-bowling/README.md) | 實作 | 75 | 6.006 Lecture 15 |
| 0702 | [LCS 與最小 diff（Hirschberg）](problems/6.006/0702-lcs-diff/README.md) | 實作 | 110 | 6.006 Lecture 16 |
| 0703 | [Edit Distance 與拼字建議](problems/6.006/0703-edit-distance/README.md) | 實作 | 100 | 6.006 Recitation 16 |
| 0704 | [Max Subarray 與 LIS（受限子問題）](problems/6.006/0704-max-subarray-lis/README.md) | 實作 | 90 | 6.006 Lecture 16, Recitation 16 |
| 0705 | [Alternating Coin Game（兩人零和區間 DP）](problems/6.006/0705-coin-game/README.md) | 實作 | 60 | 6.006 Lecture 16 |
| 0706 | [Text Justification（最佳換行）](problems/6.006/0706-text-justification/README.md) | 實作 | 60 | 6.006 Recitation 15 |
| 0711 | [Burst 排程與冷卻（狀態式線性 DP）](problems/6.006/0711-burst-cooldown/README.md) | 實作 | 70 | 6.006 PS7 Problem 7-1 |
| 0712 | [依優先序把工作放到機器上（序列對齊 DP）](problems/6.006/0712-priority-placement/README.md) | 實作 | 70 | 6.006 PS7 Problem 7-2 |
| 0713 | [DAG 路徑計數與同餘（狀態擴充）](problems/6.006/0713-path-parity/README.md) | 實作 | 70 | 6.006 PS7 Problem 7-3 |
| 0714 | [環狀切分賽局（環形區間 DP 與 minimax）](problems/6.006/0714-ring-partition-game/README.md) | 實作 | 90 | 6.006 PS7 Problem 7-4 |
| 0715 | [不跳時段的最長下降趨勢（受限 LIS）](problems/6.006/0715-gapless-decline/README.md) | 實作 | 110 | 6.006 PS7 Problem 7-5 |

### 08 · Dynamic Programming II（PS8）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0801 | [Rod Cutting、Unbounded Knapsack 與 Coin Change](problems/6.006/0801-rod-cutting/README.md) | 實作 | 80 | 6.006 Lecture 18, Recitation 18 |
| 0802 | [Subset Sum、Bitset 與 Partition（pseudopolynomial）](problems/6.006/0802-subset-sum/README.md) | 實作 | 90 | 6.006 Lecture 18, Recitation 18 |
| 0803 | [0-1 Knapsack（滾動陣列與兩種維度的取捨）](problems/6.006/0803-knapsack/README.md) | 實作 | 75 | 6.006 Recitation 18 |
| 0804 | [括號化與 Matrix Chain（區間 DP）](problems/6.006/0804-interval-parenthesization/README.md) | 實作 | 85 | 6.006 Lecture 17 |
| 0805 | [最短路徑的 DP 視角（Bellman-Ford 與 Floyd-Warshall）](problems/6.006/0805-dp-shortest-paths/README.md) | 實作 | 80 | 6.006 Lecture 17 |
| 0806 | [Trellis DP、Viterbi 與棋盤放置（狀態擴充）](problems/6.006/0806-trellis-viterbi/README.md) | 實作 | 100 | 6.006 Lecture 17, Recitation 17 |
| 0811 | [賣出定量的電（0-1 Knapsack 加懲罰）](problems/6.006/0811-power-offers/README.md) | 實作 | 65 | 6.006 PS8 Problem 8-1 |
| 0812 | [動態相鄰的序列壓實（括號式區間 DP）](problems/6.006/0812-sequence-compaction/README.md) | 實作 | 95 | 6.006 PS8 Problem 8-2, Recitation 17 |
| 0813 | [四台機器的 makespan 平衡（多維 Subset Sum）](problems/6.006/0813-four-machine-balance/README.md) | 實作 | 90 | 6.006 PS8 Problem 8-3 |
| 0814 | [還原 log 的空白（切割 DP 與 Trie）](problems/6.006/0814-word-restoration/README.md) | 實作 | 100 | 6.006 PS8 Problem 8-4 |

## MIT 6.046J — Design and Analysis of Algorithms (Spring 2015)

50 題，估計約 106 小時。


### 01 · Scheduling 與 Divide & Conquer（PS1）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0101 | [Interval Scheduling（Greedy 與 Weighted DP）](problems/6.046j/0101-interval-scheduling/README.md) | 實作 | 75 | 6.046J Lecture 1, Recitation 1 |
| 0102 | [Convex Hull（Divide & Conquer）](problems/6.046j/0102-convex-hull/README.md) | 實作 | 150 | 6.046J Lecture 2 |
| 0103 | [Median of Medians（確定性 Select）](problems/6.046j/0103-median-select/README.md) | 實作 | 90 | 6.046J Lecture 2 |
| 0111 | [Tree Placement（反例、Tree DP、Greedy、NP-hard）](problems/6.046j/0111-restaurant-placement/README.md) | 實作 | 120 | 6.046J PS1 Problem 1-1 |
| 0112 | [Close Stations（Divide & Conquer 找近距離點）](problems/6.046j/0112-close-stations/README.md) | 實作 | 120 | 6.046J PS1 Problem 1-2 |

### 02 · FFT 與 B-Trees（PS2）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0201 | [FFT 與多項式乘法（含數值合約與 NTT）](problems/6.046j/0201-fft-polynomial/README.md) | 實作 | 180 | 6.046J Lecture 3 |
| 0202 | [B-Tree（插入、刪除、範圍掃描）](problems/6.046j/0202-b-tree/README.md) | 實作 | 180 | 6.046J Recitation 2 |
| 0211 | [Wildcard Matching（用 FFT 做字串比對）](problems/6.046j/0211-wildcard-matching/README.md) | 實作 | 90 | 6.046J PS2 Problem 2-1 |
| 0212 | [Combine B-Trees（O(高度差) 合併）](problems/6.046j/0212-b-tree-combine/README.md) | 實作 | 150 | 6.046J PS2 Problem 2-2 |

### 03 · van Emde Boas（PS3）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0301 | [van Emde Boas Tree](problems/6.046j/0301-van-emde-boas/README.md) | 實作 | 240 | 6.046J Lecture 4 |
| 0311 | [vEB 的兩種變形（Policy-based 設計）](problems/6.046j/0311-veb-variants/README.md) | 實作 | 150 | 6.046J PS3 Problem 3-1 |

### 04 · Amortization 與 Randomization（PS4）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0401 | [Table Doubling 與 Binary Counter（攤銷分析）](problems/6.046j/0401-amortized-containers/README.md) | 實作 | 120 | 6.046J Lecture 5 |
| 0402 | [Union-Find（鏈結串列到 Path Compression）](problems/6.046j/0402-union-find/README.md) | 實作 | 100 | 6.046J Recitation 3 |
| 0403 | [Randomized Quicksort 與 Select](problems/6.046j/0403-randomized-sorting/README.md) | 實作 | 90 | 6.046J Lecture 6, Recitation 4 |
| 0404 | [Matrix Multiply 與 Freivalds 驗證](problems/6.046j/0404-matrix-verification/README.md) | 實作 | 90 | 6.046J Recitation 1, Lecture 6 |
| 0411 | [Min Queue（攤銷 O(1) 取最小值的 FIFO）](problems/6.046j/0411-min-queue/README.md) | 實作 | 75 | 6.046J PS4 Problem 4-1 |
| 0412 | [Quicksort 分析（用實驗驗證高機率界）](problems/6.046j/0412-quicksort-analysis/README.md) | 實作 | 100 | 6.046J PS4 Problem 4-2 |

### 05 · Skip Lists、Hashing 與 DP（PS5）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0501 | [Skip List](problems/6.046j/0501-skip-list/README.md) | 實作 | 150 | 6.046J Lecture 7 |
| 0502 | [Universal Hashing（抵抗 Hash Flooding）](problems/6.046j/0502-universal-hashing/README.md) | 實作 | 100 | 6.046J Lecture 8 |
| 0503 | [Perfect Hashing（FKS 兩層雜湊）](problems/6.046j/0503-perfect-hashing/README.md) | 實作 | 100 | 6.046J Lecture 8 |
| 0511 | [Skip List 的 Finger Search 與 Rank](problems/6.046j/0511-skip-list-finger-rank/README.md) | 實作 | 210 | 6.046J PS5 Problem 5-1 |
| 0512 | [Release Planning（四種帶限制的挑選 DP）](problems/6.046j/0512-choosing-prizes/README.md) | 實作 | 180 | 6.046J PS5 Problem 5-2 |

### 06 · Augmentation、APSP 與 MST（PS6）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0601 | [2D Range Tree（矩形範圍查詢）](problems/6.046j/0601-range-tree/README.md) | 實作 | 120 | 6.046J Lecture 9 |
| 0602 | [區間 DP 工具箱（回文、最優 BST、取硬幣）](problems/6.046j/0602-dp-toolbox/README.md) | 實作 | 100 | 6.046J Lecture 10, Recitation 5 |
| 0603 | [All-Pairs Shortest Paths 四件套](problems/6.046j/0603-apsp/README.md) | 實作 | 130 | 6.046J Lecture 11 |
| 0604 | [Minimum Spanning Tree（Kruskal、Prim、clustering）](problems/6.046j/0604-mst/README.md) | 實作 | 80 | 6.046J Lecture 12 |
| 0605 | [貪婪排程（SPT、SRPT、Interval Partitioning）](problems/6.046j/0605-greedy-scheduling/README.md) | 實作 | 75 | 6.046J Recitation 6 |
| 0611 | [動態與限跳數 All-Pairs Shortest Paths](problems/6.046j/0611-dynamic-bounded-apsp/README.md) | 實作 | 150 | 6.046J PS6 Problem 6-1 |
| 0612 | [MST 變形（Borůvka、D&C 反例、Cycle-Breaking）](problems/6.046j/0612-mst-variants/README.md) | 實作 | 130 | 6.046J PS6 Problem 6-2 |

### 07 · Network Flow（PS7）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0701 | [Maximum Flow（Ford-Fulkerson、Edmonds-Karp、Min Cut）](problems/6.046j/0701-max-flow/README.md) | 實作 | 120 | 6.046J Lecture 13, Lecture 14 |
| 0702 | [Bipartite Matching（König 定理與 Hall 憑證）](problems/6.046j/0702-bipartite-matching/README.md) | 實作 | 110 | 6.046J Lecture 14, Recitation 7 |
| 0703 | [Baseball Elimination（聯賽誰還有希望）](problems/6.046j/0703-baseball-elimination/README.md) | 實作 | 80 | 6.046J Lecture 14 (Baseball Elimination) |
| 0711 | [動態 Max Flow（容量更新不重算）](problems/6.046j/0711-dynamic-max-flow/README.md) | 實作 | 150 | 6.046J PS7 Problem 7-1 |
| 0712 | [Edge-Disjoint 路由（Menger 憑證與 Hub 限制）](problems/6.046j/0712-disjoint-routes/README.md) | 實作 | 110 | 6.046J PS7 Problem 7-2 |
| 0713 | [配額分派（最少退單與增量重排）](problems/6.046j/0713-quota-assignment/README.md) | 實作 | 110 | 6.046J PS7 Problem 7-3 |

### 08 · LP 與 NP-Completeness（PS8）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0801 | [LP 建模（Standard Form、Slack Form、對偶）](problems/6.046j/0801-lp-modeling/README.md) | 實作 | 110 | 6.046J Lecture 15 |
| 0802 | [NP 完全性：把 Reduction 寫成程式](problems/6.046j/0802-np-reductions/README.md) | 實作 | 150 | 6.046J Lecture 16, Recitation 8 |
| 0811 | [Simplex Solver（對偶憑證）](problems/6.046j/0811-simplex/README.md) | 實作 | 180 | 6.046J PS8 Problem 8-1 |
| 0812 | [NP 完全性證明寫成程式（Triple-SAT、DONUT、SCHED）](problems/6.046j/0812-np-reductions-ps8/README.md) | 實作 | 160 | 6.046J PS8 Problem 8-2 |

### 09 · Approximation 與 FPT（PS9）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0901 | [近似演算法工具箱（Vertex Cover、Set Cover、Partition、TSP）](problems/6.046j/0901-approx-toolbox/README.md) | 實作 | 130 | 6.046J Lecture 17, Recitation 9 |
| 0902 | [FPT：Vertex Cover 的 Bounded Search 與 Kernel](problems/6.046j/0902-fpt-vertex-cover/README.md) | 實作 | 120 | 6.046J Lecture 18 |
| 0911 | [Knapsack：Greedy、2-Approx、DP 與 FPTAS](problems/6.046j/0911-knapsack-approx/README.md) | 實作 | 140 | 6.046J PS9 Problem 9-1 |
| 0912 | [Tournament Feedback Arc：Kernel 與 FPT 搜尋](problems/6.046j/0912-tournament-fpt/README.md) | 實作 | 160 | 6.046J PS9 Problem 9-2 |

### 10 · Distributed Algorithms（PS10）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 1001 | [同步網路演算法（Leader、BFS Tree、MIS、Bellman-Ford）](problems/6.046j/1001-sync-network/README.md) | 實作 | 140 | 6.046J Lecture 19, Recitation 10 |
| 1002 | [非同步網路（Max、BFS 校正、節點計數、Bellman-Ford）](problems/6.046j/1002-async-network/README.md) | 實作 | 130 | 6.046J Lecture 20, Recitation 10 |
| 1003 | [UID Ring 上的 Leader Election（LCR 與 Hirschberg-Sinclair）](problems/6.046j/1003-uid-ring-election/README.md) | 實作 | 100 | 6.046J Recitation 10 |
| 1011 | [匿名環上的 Leader Election（對稱性、隨機與不可能性）](problems/6.046j/1011-ring-election/README.md) | 實作 | 130 | 6.046J PS10 Problem 10-1 |
| 1012 | [非同步網路上的 Layered BFS（不做 correction）](problems/6.046j/1012-layered-bfs/README.md) | 實作 | 150 | 6.046J PS10 Problem 10-2 |
| 1021 | [van Emde Boas Layout（Cache-Oblivious Search）](problems/6.046j/1021-veb-layout/README.md) | 效能 | 110 | 6.046J Lecture 24 |
| 1022 | [Merkle Tree（驗證儲存與新鮮度）](problems/6.046j/1022-merkle-tree/README.md) | 實作 | 110 | 6.046J Recitation 11, Lecture 21 |

## MIT 6.172 — Performance Engineering of Software Systems (Fall 2018)

50 題，估計約 118 小時。


### 01 · C++ 基本功與工具（HW1）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0111 | [記憶體配置、物件生命週期與 UB](problems/6.172/0111-memory-layout-and-ub/README.md) | 實作 | 120 | 6.172 HW1 §4 C Primer（Write-up 2–4） |
| 0112 | [Sanitizer 除錯與 Crash Triage](problems/6.172/0112-sanitizer-bug-hunt/README.md) | 實作 | 120 | 6.172 HW1 §5 Basic tools（Write-up 5–8） |
| 0113 | [矩陣乘法的迴圈順序](problems/6.172/0113-matrix-loop-order/README.md) | 效能 | 90 | 6.172 HW1 §6（Write-up 9–10）、Lecture 1 |

### 02 · Profiling（HW2）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0201 | [Phase Profiler（自己做 perf report）](problems/6.172/0201-phase-profiler/README.md) | 實作 | 90 | 6.172 HW2 §2 Recitation（perf／Cachegrind 概念） |
| 0211 | [Merge Sort 逐步調校](problems/6.172/0211-merge-sort-tuning/README.md) | 效能 | 240 | 6.172 HW2 §3 Homework: Sorting（Write-up 1–7） |

### 03 · Bit Hacks（Project 1）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0301 | [Bit Tricks 工具箱](problems/6.172/0301-bit-tricks/README.md) | 實作 | 80 | 6.172 Lecture 3（Bit Hacks） |
| 0311 | [Bit Array 與快速旋轉](problems/6.172/0311-bit-rotate/README.md) | 專案 | 300 | 6.172 Project 1: Bit Hacks |

### 04 · Vectorization（HW3）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0411 | [讓編譯器幫你向量化](problems/6.172/0411-autovec-loops/README.md) | 效能 | 180 | 6.172 HW3（§2 Example 1–4、§3.1–3.2、Write-up 1–15）、Recitation R3 |
| 0412 | [NEON 手寫掃描（AVX2 換成 NEON）](problems/6.172/0412-neon-scanning/README.md) | 效能 | 120 | 6.172 HW3（§3.1.3 Vector flavors，AVX2 轉 NEON） |

### 05 · Fork-Join 與 Reducer（HW4）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0501 | [Work／Span Meter（自己做 Cilkscale）](problems/6.172/0501-work-span-meter/README.md) | 實作 | 120 | 6.172 HW4 §2 Recitation（Checkoff 1–3、5）、Recitation R4、Lecture 8 |
| 0511 | [平行 Quicksort 與 Race 修復](problems/6.172/0511-parallel-quicksort/README.md) | 實作 | 100 | 6.172 HW4 §2.2–2.3（Checkoff 4–5） |
| 0512 | [N-Queens 從循序到平行](problems/6.172/0512-n-queens-parallel/README.md) | 實作 | 140 | 6.172 HW4 §3.1–3.2（Write-up 1–6） |
| 0513 | [Reducer：不用鎖的並行累積](problems/6.172/0513-reducer-views/README.md) | 實作 | 180 | 6.172 HW4 §3.3 Reducers（Write-up 7–8） |

### 06 · Work/Span 理論（HW5）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0611 | [Greedy Scheduler 與 Work／Span 定律](problems/6.172/0611-greedy-scheduler/README.md) | 實作 | 120 | 6.172 HW5 Write-up 1（CLRS 27.1-3）、Write-up 4；Recitation R4 |
| 0612 | [效能量測稽核（Work／Span 定律當 LP）](problems/6.172/0612-measurement-auditor/README.md) | 實作 | 100 | 6.172 HW5 Write-up 2、3、4（CLRS 27.1-5） |
| 0615 | [遞迴平行轉置](problems/6.172/0615-recursive-transpose/README.md) | 實作 | 90 | 6.172 HW5 Write-up 5（CLRS 27.2-5） |
| 0616 | [平行 Floyd-Warshall 與遞移閉包](problems/6.172/0616-parallel-floyd-warshall/README.md) | 實作 | 110 | 6.172 HW5 Write-up 6（CLRS 27.2-6） |
| 0617 | [平行 Reduce 與 Prefix Scan](problems/6.172/0617-parallel-reduce-scan/README.md) | 實作 | 150 | 6.172 HW5 Write-up 7（CLRS Problem 27-4） |

### 07 · Collision Detection（Project 2）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0711 | [2D Collision World（quadtree + fork-join）](problems/6.172/0711-collision-world/README.md) | 專案 | 540 | 6.172 Project 2 |

### 08 · Storage Allocation（HW6）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0801 | [Fixed-Size Block Pool（free list 與 object pool）](problems/6.172/0801-fixed-block-pool/README.md) | 效能 | 100 | 6.172 Lecture 11 Fixed-Size Allocation；Recitation 6 第 2 題 |
| 0802 | [Small-Object Bins（size class 配置器）](problems/6.172/0802-small-object-bins/README.md) | 效能 | 90 | 6.172 Lecture 11 Variable-Size Allocation（binned free lists） |
| 0803 | [Binned Free Lists 與 Buddy Coalescing](problems/6.172/0803-buddy-allocator/README.md) | 實作 | 90 | 6.172 Lecture 11 Allocation for Binned Free Lists、Coalescing |
| 0811 | [Trace-Driven 固定區塊配置與 Autotuning](problems/6.172/0811-trace-autotuner/README.md) | 實作 | 45 | 6.172 HW6 Checkoff 1–2；Recitation 6 第 1 題 |
| 0812 | [Cache Line 對齊 vs 緊密排列（配置策略實驗）](problems/6.172/0812-cache-line-placement/README.md) | 實作 | 80 | 6.172 HW6 §5–§6 Write-up 1–3（cache-friendly allocation） |
| 0813 | [零額外空間的雙尺寸配置器（pointer tagging）](problems/6.172/0813-zero-overhead-two-sizes/README.md) | 實作 | 70 | 6.172 HW6 §6 Write-up 4–6（fixed aligned 與 smart allocator） |
| 0814 | [切分、合併與 Compaction（兩種尺寸的 heap）](problems/6.172/0814-split-coalesce-compact/README.md) | 實作 | 80 | 6.172 HW6 §6 Write-up 7–8（split／coalesce 兩種尺寸） |

### 09 · Dynamic Analysis（HW7）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 0901 | [Least Squares 與複雜度分類](problems/6.172/0901-least-squares/README.md) | 實作 | 70 | 6.172 HW7 §3（Linear-regression analysis） |
| 0911 | [用 Compiler Instrumentation 寫 Function Profiler](problems/6.172/0911-function-profiler/README.md) | 實作 | 90 | 6.172 HW7 §2 Checkoff 1–5（函式覆蓋率與 profiling） |
| 0912 | [用 Regression 量 Function Call 的成本](problems/6.172/0912-regression-call-cost/README.md) | 實作 | 90 | 6.172 HW7 §3 Write-up 1–3（linear-regression analysis） |
| 0913 | [用 Regression 量 Spawn 的成本](problems/6.172/0913-regression-spawn-cost/README.md) | 實作 | 60 | 6.172 HW7 §3 Write-up 4–5（cost of a spawn） |

### 10 · Memory Allocator（Project 3）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 1011 | [Arena Allocator（bump → implicit → binned）](problems/6.172/1011-arena-allocator/README.md) | 專案 | 780 | 6.172 Project 3 |

### 11 · Caching（HW8）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 1101 | [Set-Associative LRU 快取模擬器](problems/6.172/1101-cache-simulator/README.md) | 實作 | 120 | 6.172 Lecture 14（快取結構、3C miss）；Recitation 8 第 1–2 題 |
| 1102 | [矩陣乘法：迴圈順序、Tiling 與 Cache-Oblivious](problems/6.172/1102-cache-oblivious-matmul/README.md) | 實作 | 90 | 6.172 Lecture 14；Recitation 8 第 3 題 |
| 1103 | [多層快取與 Miss-Ratio Curve](problems/6.172/1103-cache-hierarchy-mrc/README.md) | 實作 | 80 | 6.172 Lecture 14（多層快取）；取代 HW2 §2.2 的 Cachegrind |
| 1111 | [矩形矩陣乘法的快取分析](problems/6.172/1111-rectangular-matmul/README.md) | 實作 | 70 | 6.172 HW8 §1 Checkoff 1 |
| 1112 | [Tableau 的 2N−1 空間壓縮與快取分析](problems/6.172/1112-tableau-diagonal-space/README.md) | 實作 | 60 | 6.172 HW8 §2.1 Write-up 1–2（iterative tableau） |
| 1113 | [遞迴與平行 Tableau（k²-way 的 work 與 span）](problems/6.172/1113-parallel-recursive-tableau/README.md) | 實作 | 70 | 6.172 HW8 §2.2 Write-up 3（k²-way work／span） |
| 1114 | [遞迴 Tableau 的快取複雜度（k²-way 的運氣問題）](problems/6.172/1114-recursive-tableau-cache/README.md) | 實作 | 70 | 6.172 HW8 §2.2 Write-up 4–5 |

### 12 · Deterministic Execution（HW9）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 1201 | [確定性排程器（把競態變成可重現的 bug）](problems/6.172/1201-deterministic-scheduler/README.md) | 實作 | 130 | 6.172 Lecture 16 Determinism；HW9 §4 replay 概念；Recitation 9 第 4 題 |
| 1202 | [避免死鎖（轉帳、哲學家、release-sort-reacquire）](problems/6.172/1202-deadlock-avoidance/README.md) | 實作 | 90 | 6.172 Lecture 16 Deadlock；Recitation 9 第 1 題 |
| 1211 | [可重現的除錯流程（seed、replay、shrink）](problems/6.172/1211-replayable-hash-table/README.md) | 實作 | 70 | 6.172 HW9 §2–§4 Checkoff 1–3（環境相依、non-deterministic hashtable、replay debugging） |
| 1212 | [Hash Table 的鎖切分（striping 與 false sharing）](problems/6.172/1212-striped-lock-table/README.md) | 實作 | 90 | 6.172 HW9 §5 Write-up 1–2（hashlock、fair insert） |
| 1213 | [不用鎖的 Hash Table 插入（兩個字、一次 CAS）](problems/6.172/1213-lockless-insert/README.md) | 實作 | 80 | 6.172 HW9 §6 Write-up 3（lockless hashtable） |
| 1214 | [插入順序不影響結果的 Hash Table](problems/6.172/1214-deterministic-table/README.md) | 實作 | 70 | 6.172 HW9 §7 Write-up 4（deterministic hashtable） |

### 13 · Data Synchronization（HW10）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 1301 | [從 Atomic 做出 Mutex（TTAS、競爭式、Peterson）](problems/6.172/1301-mutex-zoo/README.md) | 實作 | 80 | 6.172 Lecture 16 Properties of Mutexes；Lecture 17 Peterson's Algorithm、Mutex Using CAS |
| 1302 | [Lock-Free Stack 與 ABA 問題](problems/6.172/1302-lock-free-stack/README.md) | 實作 | 90 | 6.172 Lecture 17 Lock-Free Stack、ABA |
| 1311 | [雙鎖 FIFO Queue（head 鎖與 tail 鎖）](problems/6.172/1311-two-lock-queue/README.md) | 實作 | 80 | 6.172 HW10 check-off 1、2、7（two-lock queue） |
| 1312 | [Michael-Scott Lock-Free Queue](problems/6.172/1312-michael-scott-queue/README.md) | 實作 | 120 | 6.172 HW10 check-off 3、4、5、6、9（lock-free queue） |
| 1313 | [單一 Consumer 的 Lock-Free Queue（MPSC）](problems/6.172/1313-mpsc-queue/README.md) | 實作 | 60 | 6.172 HW10 check-off 8（single dequeuer） |

### 14 · Game-Playing Engine（Project 4）

| 編號 | 題目 | 類型 | 分鐘 | 來源 |
| --- | --- | --- | ---: | --- |
| 1411 | [Furrow Search Engine（bitboard → alpha-beta → fork-join）](problems/6.172/1411-furrow-engine/README.md) | 專案 | 900 | 6.172 Project 4 |
