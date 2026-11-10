#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "01_exec.hpp"
#include "practice.hpp"

using Board = std::uint64_t;

// Stage 1
template <typename T>
class ResultList {
public:
    ResultList() = default;
    ~ResultList() {}

    ResultList(const ResultList&) = delete;
    ResultList& operator=(const ResultList&) = delete;

    ResultList(ResultList&& other) { PRACTICE_TODO(); }
    ResultList& operator=(ResultList&& other) { PRACTICE_TODO(); }

    void push_back(T value) { PRACTICE_TODO(); }
    void merge(ResultList& other) { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    std::vector<T> to_vector() const { PRACTICE_TODO(); }
};

// Stage 2
inline int board_column(Board board, int row) { PRACTICE_TODO(); }
inline Board board_place(Board board, int row, int column) { PRACTICE_TODO(); }
inline ResultList<Board> queens_serial(int n) { PRACTICE_TODO(); }

// Stage 3 and 4
template <typename Exec>
ResultList<Board> queens_parallel(Exec& exec, int n, int serial_depth) {
    PRACTICE_TODO();
}
