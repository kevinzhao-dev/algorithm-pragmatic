#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>
#include <utility>

#include "practice.hpp"

enum class Cursor { A, B };

template <typename T>
class TwoCursorBuffer {
public:
    // Stage 1
    explicit TwoCursorBuffer(std::span<const T> items) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    const T& read(std::size_t i) const { PRACTICE_TODO(); }
    void place_cursor(Cursor c, std::size_t gap) { PRACTICE_TODO(); }
    std::size_t cursor(Cursor c) const { PRACTICE_TODO(); }

    // Stage 2
    void shift_cursor(Cursor c, int direction) { PRACTICE_TODO(); }

    // Stage 3
    void move_item(Cursor c) { PRACTICE_TODO(); }

    // Stage 4
    void insert_before_cursor(Cursor c, T item) { PRACTICE_TODO(); }
    T delete_before_cursor(Cursor c) { PRACTICE_TODO(); }
};
