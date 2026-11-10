#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename HashFn>
std::vector<std::vector<std::int64_t>> chain_layout(std::span<const std::int64_t> keys,
                                                    std::size_t buckets, HashFn hash) {
    std::vector<std::vector<std::int64_t>> table(buckets);
    for (const std::int64_t key : keys) {
        table[static_cast<std::size_t>(hash(key))].push_back(key);
    }
    return table;
}

namespace detail {

inline bool injective_under(std::span<const std::int64_t> keys, std::int64_t a, std::int64_t b,
                            std::int64_t c, std::int64_t buckets, std::vector<int>& stamp,
                            int round) {
    for (const std::int64_t key : keys) {
        const std::int64_t slot = ((a * key + b) % c) % buckets;
        if (stamp[static_cast<std::size_t>(slot)] == round) {
            return false;
        }
        stamp[static_cast<std::size_t>(slot)] = round;
    }
    return true;
}

}  // namespace detail

inline std::optional<std::int64_t> smallest_injective_modulus(std::span<const std::int64_t> keys,
                                                              std::int64_t a, std::int64_t b,
                                                              std::int64_t buckets,
                                                              std::int64_t limit) {
    if (static_cast<std::int64_t>(keys.size()) > buckets) {
        return std::nullopt;  // pigeonhole: no modulus can help
    }
    std::vector<int> stamp(static_cast<std::size_t>(buckets), 0);
    for (std::int64_t c = 1; c <= limit; ++c) {
        if (detail::injective_under(keys, a, b, c, buckets, stamp, static_cast<int>(c))) {
            return c;
        }
    }
    return std::nullopt;
}

class PerfectHashSet {
public:
    explicit PerfectHashSet(std::span<const std::int64_t> keys) {
        buckets_ = static_cast<std::int64_t>(4 * keys.size() + 1);
        slots_.assign(static_cast<std::size_t>(buckets_), kEmpty);
        std::optional<std::int64_t> found;
        for (std::int64_t a = 1; !found.has_value(); ++a) {
            a_ = a;
            found = smallest_injective_modulus(keys, a_, 0, buckets_, kModulusLimit);
        }
        c_ = *found;
        for (const std::int64_t key : keys) {
            slots_[slot_of(key)] = key;
        }
    }

    bool contains(std::int64_t key) const { return slots_[slot_of(key)] == key; }
    std::size_t table_size() const { return slots_.size(); }
    std::size_t slot_of(std::int64_t key) const {
        return static_cast<std::size_t>((a_ * key % c_) % buckets_);
    }

private:
    static constexpr std::int64_t kEmpty = -1;
    static constexpr std::int64_t kModulusLimit = 20'000;

    std::int64_t buckets_ = 1;
    std::int64_t a_ = 1;
    std::int64_t c_ = 1;
    std::vector<std::int64_t> slots_;
};
