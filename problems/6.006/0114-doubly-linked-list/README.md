---
title: Doubly Linked List (O(1) range moves)
unit: 01 · Sequences (PS1)
source: 6.006 PS1 Problem 1-4
pdf: materials/6.006/psets/ps1/questions.pdf
answer: materials/6.006/psets/ps1/solutions.pdf
read: materials/6.006/lectures/lec02.pdf, materials/6.006/recitations/rec02.pdf
kind: code
minutes: 75
---

# 0114 · Doubly Linked List

**Original problem:** [Problem Set 1 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/dc56bd1995baf20cfcdd3fbabdd2dbc9_MIT6_006S20_ps1-questions.pdf) · **Read first:** [Lecture 2: Data Structures notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/79a07dc1cb47d76dae2ffedc701e3d2b_MIT6_006S20_lec2.pdf), [Recitation 2 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/c08a3b63dfe5f6f6b32257d35f86ae63_MIT6_006S20_r02.pdf)

> **Why this comes up in practice:** an LRU cache, a scheduler's ready queue and an editor's line buffer all need to move a node (or a whole run of nodes) out in O(1) once you already hold it. `std::vector` can't do that, and it is the only reason a linked list is still worth having.

## Original problem

6.006 PS1 Problem 1-4 extends the singly linked list from Lecture 2 into a doubly linked list. Each node keeps a `prev` and a `next` pointer, the list keeps both its `head` and its `tail`, and the list must not store its length.

- (a) Describe O(1) algorithms for `insert_first(x)`, `insert_last(x)`, `delete_first()` and `delete_last()`.
- (b) Given nodes x1 and x2 of a list L, where x1 comes before x2, describe an O(1) algorithm that removes every node from x1 through x2, inclusive, and returns them as a new doubly linked list.
- (c) Given a node x of a list L1 and a second list L2, describe an O(1) algorithm that splices L2 into L1 right after x. Afterwards L1 holds the items of both lists and L2 is empty.
- (d) Implement all of the above in the problem set's Python code template, without changing its node class.

## Your task

Implement a doubly linked list in `solution.hpp` that owns its nodes. **Do not maintain a length:** once a whole range can move out in O(1), there is no way to keep a length up to date. Stage 4 then uses the list to build an LRU cache, which the original problem doesn't have.

Each stage lists the declarations it adds, copied from `solution.hpp`.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Operations at both ends, and ownership

```cpp
template <typename T>
class DoublyLinkedList {
public:
    struct Node {
        explicit Node(T value) : item(std::move(value)) {}
        T item;
        Node* prev = nullptr;
        Node* next = nullptr;
    };

    DoublyLinkedList() = default;
    ~DoublyLinkedList();
    DoublyLinkedList(const DoublyLinkedList&) = delete;
    DoublyLinkedList& operator=(const DoublyLinkedList&) = delete;

    bool empty() const;  // provided
    Node* head() const;  // provided
    Node* tail() const;  // provided

    Node* insert_first(T value);
    Node* insert_last(T value);
    T delete_first();
    T delete_last();
};
```

| Operation | Behavior | Complexity |
| --- | --- | --- |
| `insert_first(v)` / `insert_last(v)` | Insert and return the new node | O(1) |
| `delete_first()` / `delete_last()` | Remove and return the value; throw `std::out_of_range` if the list is empty | O(1) |
| `head()` / `tail()` / `empty()` | `head()` and `tail()` are `nullptr` for an empty list | O(1) |
| Destructor | Free every node without recursion (a million nodes would overflow the stack) | O(n) |

Edge case to get right: deleting the only remaining node must reset both `head` and `tail` to `nullptr`.

## Stage 2 — Cutting out a range

```cpp
// in class DoublyLinkedList<T>
DoublyLinkedList remove(Node* first, Node* last);
```

`remove(first, last)`: `first` and `last` are nodes of this list, and `first` comes before `last` (or is the same node). Cut that range, both ends included, out of the list and return it as a new list in **O(1)**. The nodes themselves must move; do not copy them.

Edge cases: cutting from the head, cutting from the tail, cutting the whole list, and cutting a single node.

## Stage 3 — Splicing back in

```cpp
// in class DoublyLinkedList<T>
void splice(Node* after, DoublyLinkedList& other);
DoublyLinkedList(DoublyLinkedList&& other);             // should be noexcept once implemented
DoublyLinkedList& operator=(DoublyLinkedList&& other);  // should be noexcept once implemented
```

`splice(after, other)`: attach all of `other`'s nodes right after node `after` of this list, in **O(1)**. Afterwards `other` is empty.

Edge cases: `other` is empty, and `after` is the current `tail`.

Also add a move constructor and move assignment (Stage 2 already needs them to return a new list).

## Stage 4 — Real-world extension: LRU Cache

```cpp
template <typename K, typename V>
class LruCache {
public:
    explicit LruCache(std::size_t capacity);
    std::optional<V> get(const K& key);
    std::optional<K> put(K key, V value);
    std::size_t size() const;
};
```

Use your list plus `std::unordered_map` to build `LruCache<K, V>`:

- `explicit LruCache(std::size_t capacity)`, where `capacity` is at least 1.
- `std::optional<V> get(const K&)`: if the key exists, return its value and mark it as most recently used.
- `std::optional<K> put(K, V)`: insert or overwrite; this also counts as a use. If it evicts a key, return the evicted key; otherwise return `std::nullopt`.
- `std::size_t size() const`.

Both `get` and `put` must be O(1) on average.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why can't this list have an O(1) `size()`? If the requirements insisted on one, which operation would you give up?
2. What complexity does the standard require of `std::list::splice` since C++11? How does that relate to your design trade-offs?
3. Make `LruCache` thread-safe: where is the bottleneck with one big lock? Why does `get` also need a write lock?

---

**Official solution:** [Problem Set 1 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/10e4e3444c1bde4c662a47a8f14dadd3_MIT6_006S20_ps1-solutions.pdf). Read it after you finish.
