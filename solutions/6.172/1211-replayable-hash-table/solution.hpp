#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>
#include <random>
#include <span>
#include <utility>
#include <vector>

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

inline std::vector<Op> generate_ops(std::uint64_t seed, std::size_t count) {
    std::mt19937_64 engine(seed);
    std::uniform_int_distribution<std::uint64_t> keys(1, ~std::uint64_t{0});
    std::uniform_int_distribution<int> percent(0, 99);
    std::vector<Op> ops;
    std::vector<std::uint64_t> inserted;
    ops.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        if (percent(engine) < 70 || inserted.empty()) {
            const std::uint64_t key = keys(engine);
            ops.push_back({Op::Kind::Insert, key, keys(engine) | 1});
            inserted.push_back(key);
        } else {
            std::uniform_int_distribution<std::size_t> pick(0, inserted.size() - 1);
            ops.push_back({Op::Kind::Find, inserted[pick(engine)], 0});
        }
    }
    return ops;
}

template <typename Table>
std::vector<std::optional<std::uint64_t>> run_ops(Table& table, std::span<const Op> ops) {
    std::vector<std::optional<std::uint64_t>> results;
    for (const Op& op : ops) {
        if (op.kind == Op::Kind::Insert) {
            table.insert(op.key, op.value);
        } else {
            results.push_back(table.find(op.key));
        }
    }
    return results;
}

inline std::optional<std::uint64_t> find_failing_seed(std::uint64_t first, std::uint64_t last,
                                                      std::size_t ops_per_run,
                                                      const FailureCheck& fails) {
    for (std::uint64_t seed = first; seed < last; ++seed) {
        const std::vector<Op> ops = generate_ops(seed, ops_per_run);
        if (fails(ops)) {
            return seed;
        }
    }
    return std::nullopt;
}

inline std::vector<Op> shrink_ops(std::vector<Op> ops, const FailureCheck& fails) {
    if (!fails(ops)) {
        return ops;
    }
    bool reduced = true;
    while (reduced) {
        reduced = false;
        for (std::size_t i = 0; i < ops.size();) {
            std::vector<Op> candidate = ops;
            candidate.erase(candidate.begin() + static_cast<std::ptrdiff_t>(i));
            if (fails(candidate)) {
                ops = std::move(candidate);
                reduced = true;
            } else {
                ++i;
            }
        }
    }
    return ops;
}

struct MultiplicativeHash {
    std::uint64_t operator()(std::uint64_t key) const { return key * 0x9E3779B97F4A7C15ULL; }
};

template <typename Hash = MultiplicativeHash>
class ProbingTable {
public:
    explicit ProbingTable(std::size_t capacity, Hash hash = Hash{})
        : slots_(capacity), hash_(std::move(hash)) {}

    bool insert(std::uint64_t key, std::uint64_t value) {
        const std::size_t capacity = slots_.size();
        std::size_t slot = home(key);
        for (std::size_t probes = 0; probes < capacity; ++probes) {
            Slot& candidate = slots_[slot];
            if (candidate.key == 0) {
                candidate = {key, value};
                ++size_;
                return true;
            }
            if (candidate.key == key) {
                candidate.value = value;
                return true;
            }
            slot = (slot + 1) % capacity;
        }
        return false;
    }

    std::optional<std::uint64_t> find(std::uint64_t key) const {
        const std::size_t capacity = slots_.size();
        std::size_t slot = home(key);
        for (std::size_t probes = 0; probes < capacity && slots_[slot].key != 0; ++probes) {
            if (slots_[slot].key == key) {
                return slots_[slot].value;
            }
            slot = (slot + 1) % capacity;
        }
        return std::nullopt;
    }

    std::size_t size() const { return size_; }

private:
    struct Slot {
        std::uint64_t key = 0;
        std::uint64_t value = 0;
    };

    std::size_t home(std::uint64_t key) const {
        return static_cast<std::size_t>(hash_(key) >> 32) % slots_.size();
    }

    std::vector<Slot> slots_;
    Hash hash_;
    std::size_t size_ = 0;
};
