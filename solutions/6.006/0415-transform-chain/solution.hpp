#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "../0404-avl-sequence/solution.hpp"

// M must provide static M::identity() and an associative operator*; commutativity is not assumed.
template <typename M>
struct ProductAug {
    using Value = M;
    static Value identity() { return M::identity(); }
    static Value of(const M& item) { return item; }
    static Value combine(const Value& a, const Value& b) { return a * b; }
};

template <typename M>
class TransformChain {
public:
    explicit TransformChain(std::vector<M> joints) : joints_(std::move(joints)), identity_(M::identity()) {}

    std::size_t size() const { return joints_.size(); }

    void update_joint(std::size_t index, M matrix) { joints_.set_at(index, std::move(matrix)); }

    const M& full_transformation() const {
        const auto* root = joints_.tree().root();
        return root != nullptr ? root->aug : identity_;
    }

    M segment(std::size_t left, std::size_t right) const { return joints_.fold(left, right); }

    void insert_joint(std::size_t index, M matrix) { joints_.insert_at(index, std::move(matrix)); }

    M remove_joint(std::size_t index) { return joints_.delete_at(index); }

private:
    AvlSequence<M, ProductAug<M>> joints_;
    M identity_;
};
