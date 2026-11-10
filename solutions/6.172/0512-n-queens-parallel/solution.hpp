#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "01_exec.hpp"

using Board = std::uint64_t;

template <typename T>
class ResultList {
public:
    ResultList() = default;
    ~ResultList() { clear(); }

    ResultList(const ResultList&) = delete;
    ResultList& operator=(const ResultList&) = delete;

    ResultList(ResultList&& other) noexcept
        : head_(std::exchange(other.head_, nullptr)),
          tail_(std::exchange(other.tail_, nullptr)),
          size_(std::exchange(other.size_, 0)) {}

    ResultList& operator=(ResultList&& other) noexcept {
        if (this != &other) {
            clear();
            head_ = std::exchange(other.head_, nullptr);
            tail_ = std::exchange(other.tail_, nullptr);
            size_ = std::exchange(other.size_, 0);
        }
        return *this;
    }

    void push_back(T value) {
        Node* node = new Node{std::move(value), nullptr};
        (tail_ != nullptr ? tail_->next : head_) = node;
        tail_ = node;
        ++size_;
    }

    void merge(ResultList& other) {
        if (&other == this || other.head_ == nullptr) {
            return;
        }
        (tail_ != nullptr ? tail_->next : head_) = other.head_;
        tail_ = other.tail_;
        size_ += other.size_;
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    std::vector<T> to_vector() const {
        std::vector<T> out;
        out.reserve(size_);
        for (const Node* node = head_; node != nullptr; node = node->next) {
            out.push_back(node->value);
        }
        return out;
    }

private:
    struct Node {
        T value;
        Node* next;
    };

    void clear() {
        while (head_ != nullptr) {
            delete std::exchange(head_, head_->next);
        }
        tail_ = nullptr;
        size_ = 0;
    }

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;
};

inline int board_column(Board board, int row) { return static_cast<int>((board >> (4 * row)) & 0xF); }

inline Board board_place(Board board, int row, int column) {
    return board | (static_cast<Board>(column) << (4 * row));
}

namespace detail_0512 {

template <typename Exec>
ResultList<Board> search(Exec& exec, int n, int row, std::uint32_t down, std::uint32_t left,
                         std::uint32_t right, Board board, int serial_depth);

// Places a queen in one of `candidates` (columns of `row`), halving the set so the two halves
// can run as parallel branches. Results are merged only after both branches have finished.
template <typename Exec>
ResultList<Board> branch(Exec& exec, int n, int row, std::uint32_t down, std::uint32_t left,
                         std::uint32_t right, Board board, std::uint32_t candidates,
                         int serial_depth) {
    if ((candidates & (candidates - 1)) == 0) {
        const std::uint32_t all = (std::uint32_t{1} << n) - 1;
        const std::uint32_t place = candidates;
        return search(exec, n, row + 1, down | place, ((left | place) << 1) & all,
                      (right | place) >> 1, board_place(board, row, std::countr_zero(place)),
                      serial_depth);
    }
    std::uint32_t low = 0;
    std::uint32_t rest = candidates;
    for (int to_take = std::popcount(candidates) / 2; to_take > 0; --to_take) {
        const std::uint32_t place = rest & (~rest + 1);
        low |= place;
        rest ^= place;
    }
    ResultList<Board> left_results;
    ResultList<Board> right_results;
    exec.fork2(
        [&] {
            left_results = branch(exec, n, row, down, left, right, board, low, serial_depth);
        },
        [&] {
            right_results = branch(exec, n, row, down, left, right, board, rest, serial_depth);
        });
    left_results.merge(right_results);
    return left_results;
}

template <typename Exec>
ResultList<Board> search(Exec& exec, int n, int row, std::uint32_t down, std::uint32_t left,
                         std::uint32_t right, Board board, int serial_depth) {
    exec.charge();
    ResultList<Board> found;
    if (row == n) {
        found.push_back(board);
        return found;
    }
    const std::uint32_t all = (std::uint32_t{1} << n) - 1;
    std::uint32_t available = ~(down | left | right) & all;
    if (available == 0) {
        return found;
    }
    if (row < serial_depth) {
        return branch(exec, n, row, down, left, right, board, available, serial_depth);
    }
    while (available != 0) {
        const std::uint32_t place = available & (~available + 1);
        available ^= place;
        ResultList<Board> below = search(exec, n, row + 1, down | place, ((left | place) << 1) & all,
                                         (right | place) >> 1,
                                         board_place(board, row, std::countr_zero(place)), serial_depth);
        found.merge(below);
    }
    return found;
}

}  // namespace detail_0512

inline ResultList<Board> queens_serial(int n) {
    exec::Meter ignored;  // with serial_depth 0 it never forks; the bookkeeping is simply discarded
    return detail_0512::search(ignored, n, 0, 0, 0, 0, 0, 0);
}

template <typename Exec>
ResultList<Board> queens_parallel(Exec& exec, int n, int serial_depth) {
    return detail_0512::search(exec, n, 0, 0, 0, 0, 0, serial_depth);
}
