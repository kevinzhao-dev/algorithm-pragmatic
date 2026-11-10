#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>

template <typename Seq>
void reverse_range(Seq& seq, std::size_t i, std::size_t k) {
    if (i + k > seq.size()) {
        throw std::invalid_argument("range out of bounds");
    }
    // Repeatedly pull the last element of the remaining suffix to the next front position.
    for (std::size_t placed = 0; placed + 1 < k; ++placed) {
        auto item = seq.delete_at(i + k - 1);
        seq.insert_at(i + placed, std::move(item));
    }
}

template <typename Seq>
void move_range(Seq& seq, std::size_t i, std::size_t k, std::size_t j) {
    if (i + k > seq.size() || j > seq.size() || (i <= j && j < i + k)) {
        throw std::invalid_argument("invalid range or target");
    }
    if (j >= i + k) {
        // Moving right: the block keeps its start at i, and each deletion moves the target
        // item one slot left, so inserting at j - 1 stays just before it.
        for (std::size_t moved = 0; moved < k; ++moved) {
            auto item = seq.delete_at(i);
            seq.insert_at(j - 1, std::move(item));
        }
    } else {
        // Moving left: every insertion lands before the remaining block, so the next block
        // element stays at the same index relative to i + moved.
        for (std::size_t moved = 0; moved < k; ++moved) {
            auto item = seq.delete_at(i + moved);
            seq.insert_at(j + moved, std::move(item));
        }
    }
}

template <typename Seq>
void rotate_left(Seq& seq, std::size_t i, std::size_t k, std::size_t m) {
    if (m > k) {
        throw std::invalid_argument("rotation amount exceeds the range");
    }
    if (i + k > seq.size()) {
        throw std::invalid_argument("range out of bounds");
    }
    if (m <= k - m) {
        move_range(seq, i, m, i + k);
    } else {
        move_range(seq, i + m, k - m, i);
    }
}
