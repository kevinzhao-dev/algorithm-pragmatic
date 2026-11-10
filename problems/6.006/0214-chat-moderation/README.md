---
title: Chat Room and Banning (composing data structures)
unit: 02 · Sorting & Recurrences (PS2)
source: 6.006 PS2 Problem 2-4
pdf: materials/6.006/psets/ps2/questions.pdf
answer: materials/6.006/psets/ps2/solutions.pdf
read: materials/6.006/lectures/lec02.pdf, materials/6.006/recitations/rec03.pdf
kind: code
minutes: 80
---

# 0214 · Chat Room and Banning

**Original problem:** [Problem Set 2 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/70d9c6b212c6dfce6723259ffb429521_MIT6_006S20_ps2-questions.pdf) · **Read first:** [Lecture 2: Data Structures notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/79a07dc1cb47d76dae2ffedc701e3d2b_MIT6_006S20_lec2.pdf), [Recitation 3 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/1869dbf640ded6b31f1bd369d2001ef5_MIT6_006S20_r03.pdf)

> **Why this comes up in practice:** moderating a live-stream chat or a message feed: when a user is muted, everything they have ever said must disappear immediately, but you can't scan the whole message stream to make that happen. The point of problems like this is to combine a few simple structures so that each operation touches only the parts it has to.

## Original problem

6.006 PS2 Problem 2-4, "MixBookTube.tv Chat," is about a site where people chat alongside a live video-game stream. Viewers are identified by distinct integer IDs that are known in advance (as in lecture, input integers are assumed to fit in a machine word, so two of them can be compared in constant time). The chat is one linear stream of messages, each posted by some viewer, and a viewer sees the k most recent messages, where k depends on their screen size. The streamer can ban a viewer who misbehaves: from then on the viewer can't post, and all of their earlier messages are removed from the chat.

Describe a database for the chat that supports the operations below, all in worst-case time, where n is the number of viewers in the database (banned or not) when the operation runs:

- `build(V)`: start a new chat room with the n = |V| viewers in V, in O(n log n) time.
- `send(v, m)`: post message m from viewer v, unless v is banned, in O(log n) time.
- `recent(k)`: return the k most recent messages that haven't been deleted (all of them, if there are fewer than k), in O(k) time.
- `ban(v)`: ban viewer v and delete all of v's messages, in O(n_v + log n) time, where n_v is the number of messages v sent before being banned.

The answer is written; there is no code.

## Your task

The original problem: a chat room has `n` viewers, and every operation must meet its complexity in the **worst case**, so you can't rely on a hash table's expected O(1) (`Viewer` is only guaranteed to have `<` and `==`; the tests check the number of comparisons with `Counted`). You implement `ChatRoom<Viewer, Message>`.

Stage 3 adds a per-viewer retention limit, which the original problem doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Sending and reading

```cpp
template <typename Viewer, typename Message>
class ChatRoom {
public:
    explicit ChatRoom(std::vector<Viewer> viewers,
                      std::size_t per_viewer_limit = std::numeric_limits<std::size_t>::max());  // used in Stage 3

    bool send(const Viewer& viewer, Message message);
    std::vector<Message> recent(std::size_t k) const;
    std::size_t message_count() const;
};
```

| Operation | Behavior | Complexity |
| --- | --- | --- |
| `explicit ChatRoom(viewers)` | Build the room; the viewers are distinct (throw `std::invalid_argument` on duplicates) | O(n log n) |
| `send(v, m)` | Viewer `v` posts a message. Return `false` if `v` doesn't exist and `true` on success | O(log n) |
| `recent(k)` | Return the `k` newest messages (all of them if there are fewer than `k`), **newest first** | O(k) |
| `message_count()` | The number of messages currently visible | O(1) |

The time `recent(k)` takes may depend only on `k`, not on the total number of messages.

## Stage 2 — Banning

```cpp
// in class ChatRoom<Viewer, Message>
bool ban(const Viewer& viewer);
bool is_banned(const Viewer& viewer) const;
```

- `ban(v)`: ban `v` and remove all of `v`'s past messages. Return whether it succeeded (return `false` if `v` doesn't exist or is already banned). Complexity **O(n_v + log n)**, where `n_v` is the number of `v`'s messages that are still visible; you must not scan anyone else's messages.
- `is_banned(v)`: whether `v` has been banned, O(log n).
- When a banned viewer calls `send` again, return `false`, and no new message appears.

`recent(k)` must still be O(k): your approach must not leave "tombstones" that `recent` has to skip over (the tests check this after a large number of bans).

## Stage 3 — Real-world extension: a per-viewer retention limit

The constructor takes one more argument, `ChatRoom(viewers, per_viewer_limit)` (`≥ 1`; otherwise throw `std::invalid_argument`): each viewer keeps **only their newest `per_viewer_limit`** messages. When a `send` goes over the limit, remove that viewer's own oldest message; `send` must still be O(log n). Passing only one argument means there is no limit.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why do you need the "viewer → that viewer's message nodes" index? With only a doubly linked list, which operation would fall short?
2. When you need worst-case rather than expected complexity, why is a sorted array (or `std::map`) a better fit than `unordered_map`? How much does that matter in real systems?
3. If `recent` became "a given viewer's k newest messages," how would you extend your structure?
4. In a multithreaded environment, what do `send` and `ban` contend for? How would you split up the locks?

---

**Official solution:** [Problem Set 2 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/b06b8a8baf9b15f98947e75bae55b8bf_MIT6_006S20_ps2-solutions.pdf). Read it after you finish.
