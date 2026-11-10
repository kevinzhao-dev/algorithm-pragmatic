#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

#include "practice.hpp"

struct Op {
    enum class Kind { Insert, Find };
    Kind kind;
    std::uint64_t key;
    std::uint64_t value;
    friend bool operator==(const Op&, const Op&) = default;
};

inline std::ostream& operator<<(std::ostream& os, const Op& op) {
    return os << (op.kind == Op::Kind::Insert ? "insert(" : "find(") << op.key << ')';
}

using FailureCheck = std::function<bool(std::span<const Op>)>;

// Stage 1
inline std::vector<Op> generate_ops(std::uint64_t seed, std::size_t count) { PRACTICE_TODO(); }

template <typename Table>
std::vector<std::optional<std::uint64_t>> run_ops(Table& table, std::span<const Op> ops) {
    PRACTICE_TODO();
}

// Stage 2
inline std::optional<std::uint64_t> find_failing_seed(std::uint64_t first, std::uint64_t last,
                                                      std::size_t ops_per_run,
                                                      const FailureCheck& fails) {
    PRACTICE_TODO();
}

// Stage 3
inline std::vector<Op> shrink_ops(std::vector<Op> ops, const FailureCheck& fails) { PRACTICE_TODO(); }

// Stage 4
struct MultiplicativeHash {
    std::uint64_t operator()(std::uint64_t key) const { return key * 0x9E3779B97F4A7C15ULL; }
};

template <typename Hash = MultiplicativeHash>
class ProbingTable {
public:
    explicit ProbingTable(std::size_t capacity, Hash hash = Hash{}) { PRACTICE_TODO(); }

    bool insert(std::uint64_t key, std::uint64_t value) { PRACTICE_TODO(); }
    std::optional<std::uint64_t> find(std::uint64_t key) const { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
};
