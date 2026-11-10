#pragma once

#include <cstddef>
#include <vector>

#include "practice.hpp"

template <typename M>
class TransformChain {
public:
    // Stage 1
    explicit TransformChain(std::vector<M> joints) { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
    void update_joint(std::size_t index, M matrix) { PRACTICE_TODO(); }
    const M& full_transformation() const { PRACTICE_TODO(); }

    // Stage 2
    M segment(std::size_t left, std::size_t right) const { PRACTICE_TODO(); }

    // Stage 3
    void insert_joint(std::size_t index, M matrix) { PRACTICE_TODO(); }
    M remove_joint(std::size_t index) { PRACTICE_TODO(); }
};
