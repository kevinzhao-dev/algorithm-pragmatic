#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "forkjoin.hpp"

template <template <typename> class Atomic = std::atomic>
class DeterministicTable {
public:
    explicit DeterministicTable(std::size_t capacity) : slots_(capacity), mask_(capacity - 1) {}

    void insert(std::uint64_t key) {
        std::size_t index = key & mask_;
        while (true) {
            std::uint64_t seen = slots_[index].load(std::memory_order_acquire);
            if (seen == key) {
                return;
            }
            if (seen == 0 || seen < key) {
                if (!slots_[index].compare_exchange_strong(seen, key, std::memory_order_acq_rel,
                                                           std::memory_order_acquire)) {
                    continue;  // the slot changed under us: decide again from its new contents
                }
                if (seen == 0) {
                    return;
                }
                key = seen;  // carry the displaced key onward
            }
            index = (index + 1) & mask_;
        }
    }

    std::vector<std::uint64_t> layout() const {
        std::vector<std::uint64_t> result;
        result.reserve(slots_.size());
        for (const auto& slot : slots_) {
            result.push_back(slot.load(std::memory_order_acquire));
        }
        return result;
    }

    std::pair<bool, std::size_t> lookup(std::uint64_t key) const {
        std::size_t index = key & mask_;
        std::size_t probes = 0;
        while (probes < slots_.size()) {
            const std::uint64_t seen = slots_[index].load(std::memory_order_acquire);
            ++probes;
            if (seen == key) {
                return {true, probes};
            }
            if (seen == 0 || seen < key) {
                return {false, probes};
            }
            index = (index + 1) & mask_;
        }
        return {false, probes};
    }

    bool contains(std::uint64_t key) const { return lookup(key).first; }

    void insert_all(std::span<const std::uint64_t> keys) {
        practice::parallel_for(0, keys.size(), [&](std::size_t i) { insert(keys[i]); }, 32);
    }

private:
    std::vector<Atomic<std::uint64_t>> slots_;
    std::size_t mask_;
};
