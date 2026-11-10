---
title: Open Addressing (linear probing and deletion)
unit: 03 · Hashing & Linear Sorting (PS3)
source: 6.006 Lecture 4, Recitation 4
pdf: materials/6.006/recitations/rec04.pdf
read: materials/6.006/lectures/lec04.pdf
kind: code
minutes: 70
---

# 0302 · Open Addressing

**Based on:** [Recitation 4 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/bd220fb629aefdf30f416b5abf45d38d_MIT6_006S20_r04.pdf) · **Read first:** [Lecture 4: Hashing notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/ce9e94705b914598ce78a00a70a1f734_MIT6_006S20_lec4.pdf)

> **Why this comes up in practice:** Redis, Google's `absl::flat_hash_map`, and V8's property tables all use open addressing, because one contiguous array is far friendlier to the cache than chaining's nodes. The whole cost is in deletion: one slip and you lose a key that is still in the table, or let tombstones slowly drag the whole table down.

## Your task

Implement `OpenAddressingSet<K, Hash>` with linear probing: all the data lives in a single array of slots, and on a collision you try the next slot. Do not use `std::unordered_set`.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Insert and lookup

```cpp
template <typename K, typename Hash = std::hash<K>>
class OpenAddressingSet {
public:
    explicit OpenAddressingSet(Hash hash = Hash{});

    bool insert(K key);
    bool contains(const K& key) const;
    std::size_t size() const;
    std::size_t capacity() const;
};
```

| Operation | Behavior |
| --- | --- |
| `insert(key)` | Return `true` for a new key, or `false` if it is already present |
| `contains(key)` | Whether the key is present |
| `size()` | The current number of keys |
| `capacity()` | The number of slots; `size() < capacity()` must hold at all times (so that a probe always finds an empty slot to stop at) |

- `Hash` may be terrible. libc++'s `std::hash<int>` is the identity, so if the capacity is a power of 2 and you take the low bits directly, every multiple of `1 << 20` lands in the same slot; the table must **mix** the hash once more internally.
- Rehash when the table gets too full. Any threshold you like is fine, but operations must be amortized expected O(1).

## Stage 2 — Deletion (tombstones)

```cpp
// in class OpenAddressingSet<K, Hash>
bool erase(const K& key);
```

`erase(key)`: if the key is present, remove it and return `true`; otherwise return `false`.

The traps in this stage:

- You must not simply empty the slot: keys further along the same probe path would become unreachable.
- `insert` must not stop as soon as it meets a tombstone. It must keep going to confirm that the key doesn't appear further along; otherwise the same key gets stored twice.
- Once you have confirmed that the key is absent, the new key should go into the first tombstone you passed on the path.

## Stage 3 — Tombstones must not accumulate

- When keys keep coming and going, tombstones must not pile up without bound; otherwise looking up a missing key scans the whole table. The fix is your choice: count tombstones toward the load and rebuild periodically, or switch to backward-shift deletion.
- When deletions leave very few keys, the table must shrink: `capacity() ≤ 8 × size() + 64`.
- A rebuild must not lose any key.

## Stage 4 — Real-world extension: dedup window

```cpp
template <typename K, typename Hash = std::hash<K>>
class DedupWindow {
public:
    explicit DedupWindow(std::int64_t ttl, Hash hash = Hash{});

    bool offer(const K& key, std::int64_t now);
    std::size_t size() const;
};
```

Use your set to build `DedupWindow<K>`, which filters out duplicate messages when a message queue redelivers them:

- `explicit DedupWindow(ttl)`: `ttl ≥ 1`; otherwise throw `std::invalid_argument`.
- `offer(key, now)`: `now` never decreases. If the key was **accepted** within `(now - ttl, now]`, return `false` (a duplicate, which doesn't affect the timer). Otherwise accept it, return `true`, and start counting its `ttl` from `now` (the same semantics as Redis `SET key NX EX ttl`).
- `size()`: the number of keys the window still remembers. Expired keys must actually be deleted, so that memory doesn't grow without bound.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why do linear probing's clusters keep snowballing? How do double hashing and Robin Hood hashing each deal with it?
2. What are the trade-offs between tombstones and backward-shift deletion?
3. Why is the load factor limit for open addressing lower than for chaining? How does the cost of a lookup change as the load approaches 1?
