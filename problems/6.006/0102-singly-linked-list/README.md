---
title: Singly Linked List and Floyd Cycle Detection
unit: 01 · Sequences (PS1)
source: 6.006 Recitation 2
pdf: materials/6.006/recitations/rec02.pdf
read: materials/6.006/lectures/lec02.pdf
kind: code
minutes: 80
---

# 0102 · Singly Linked List

**Based on:** [Recitation 2 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/c08a3b63dfe5f6f6b32257d35f86ae63_MIT6_006S20_r02.pdf) · **Read first:** [Lecture 2: Data Structures notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/79a07dc1cb47d76dae2ffedc701e3d2b_MIT6_006S20_lec2.pdf)

> **Why this comes up in practice:** free lists, the bucket chains of a hash table, and the kernel's task list are all singly linked lists. One of the most common linked-list bugs is pointers that form a cycle, so a traversal never stops, and that is exactly the problem Floyd's algorithm solves.

## Your task

Implement `LinkedListSeq<T>`, a singly linked list that owns its nodes: operations at the front, index-based operations with STL-style iterators, and in-place reversal. Stage 4 then detects cycles with Floyd's algorithm on any node type that has a `next` pointer.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Nodes and ownership

```cpp
template <typename T>
class LinkedListSeq {
public:
    struct Node {
        explicit Node(T value) : item(std::move(value)) {}
        T item;
        Node* next = nullptr;
    };

    LinkedListSeq();
    explicit LinkedListSeq(std::span<const T> items);

    ~LinkedListSeq();

    LinkedListSeq(const LinkedListSeq&) = delete;
    LinkedListSeq& operator=(const LinkedListSeq&) = delete;

    LinkedListSeq(LinkedListSeq&& other);             // should be noexcept once implemented
    LinkedListSeq& operator=(LinkedListSeq&& other);  // should be noexcept once implemented

    std::size_t size() const;
    bool empty() const;
    Node* head() const;

    Node* insert_first(T value);
    T delete_first();
};
```

`LinkedListSeq<T>` owns its nodes, and the `Node` struct is already written in the skeleton.

| Operation | Behavior | Complexity |
| --- | --- | --- |
| `LinkedListSeq()` / `explicit LinkedListSeq(span<const T>)` | Empty list / build in order | O(1) / O(n) |
| `insert_first(v)` | Insert at the front and return the new node | O(1) |
| `delete_first()` | Remove and return the front value; throw `std::out_of_range` if the list is empty | O(1) |
| `head()`, `size()`, `empty()` | | O(1) |
| Destructor | Free every node, without recursion | O(n) |

Copying is already disabled. Add a move constructor and move assignment (a moved-from list must become empty, with size 0).

## Stage 2 — Index operations and iterators

```cpp
// in class LinkedListSeq<T>
template <bool IsConst>
class BasicIterator {
public:
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using reference = std::conditional_t<IsConst, const T&, T&>;
    using pointer = std::conditional_t<IsConst, const T*, T*>;
    using iterator_category = std::forward_iterator_tag;

    BasicIterator() = default;

    reference operator*() const;
    pointer operator->() const;
    BasicIterator& operator++();
    BasicIterator operator++(int);
    friend bool operator==(const BasicIterator& a, const BasicIterator& b);
};

using iterator = BasicIterator<false>;
using const_iterator = BasicIterator<true>;

const T& get_at(std::size_t i) const;
void set_at(std::size_t i, T value);
Node* insert_at(std::size_t i, T value);
T delete_at(std::size_t i);
Node* insert_last(T value);
T delete_last();

iterator begin();
iterator end();
const_iterator begin() const;
const_iterator end() const;
```

| Operation | Behavior | Complexity |
| --- | --- | --- |
| `get_at(i)` / `set_at(i, v)` | Read or write element `i`; throw `std::out_of_range` if it is out of range | O(i) |
| `insert_at(i, v)` | Insert before index `i`, `0 ≤ i ≤ size()`, and return the new node | O(i) |
| `delete_at(i)` | Delete and return element `i` | O(i) |
| `insert_last(v)` / `delete_last()` | Operations at the end (this list has no tail, so O(n) is fine) | O(n) |

A failed operation must not change the list's contents.

Then add STL-style forward iterators, `iterator` and `const_iterator` (they work with range-for and `std::find`), which must satisfy the `std::forward_iterator` concept. `begin()`/`end()` each have a `const` and a non-`const` version.

## Stage 3 — In-place reversal

```cpp
// in class LinkedListSeq<T>
void reverse();
```

`reverse()`: reverse the whole list. **O(n) time and O(1) extra space, changing only pointers**: do not rebuild nodes, and do not copy or move elements (the tests check node addresses and how many times elements are moved). Do not recurse.

## Stage 4 — Floyd cycle detection

The function below works on **any node type that has a `next` pointer** (not on your list). `head` may be `nullptr`, and the chain of `next` pointers may form a cycle (a ρ shape: a tail that runs into a loop).

```cpp
template <typename Node>
struct CycleInfo { std::size_t length; const Node* entry; };

std::optional<CycleInfo<Node>> find_cycle(const Node* head);
```

If there is no cycle, return `std::nullopt`. If there is one, return the number of nodes on the cycle and the **first** node on the cycle that you reach when walking from `head`.

It must take **O(n) time and O(1) extra space** (the tests confirm that there is no heap allocation at all, which means you can't use a visited set).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why are the fast and slow pointers guaranteed to meet? How is the meeting point related to the entry of the cycle, and why does the second phase meet exactly at the entry?
2. When `next` is a `std::unique_ptr<Node>`, why can the destructor overflow the stack? How do you fix it?
3. When is a linked list actually faster than `std::vector`? How do the caches of a modern CPU affect that judgment?
