#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "08_cache_sim.hpp"

struct LevelConfig {
    std::size_t capacity_bytes;
    std::size_t line_bytes;
    std::size_t ways;
};

class CacheHierarchy {
public:
    explicit CacheHierarchy(std::vector<LevelConfig> levels) {
        if (levels.empty()) {
            throw std::invalid_argument("a hierarchy needs at least one level");
        }
        for (const LevelConfig& level : levels) {
            if (level.line_bytes != levels.front().line_bytes) {
                throw std::invalid_argument("every level must use the same line size");
            }
            caches_.emplace_back(level.capacity_bytes, level.line_bytes, level.ways);
        }
        line_bytes_ = levels.front().line_bytes;
    }

    std::size_t access(std::uint64_t address) {
        for (std::size_t level = 0; level < caches_.size(); ++level) {
            if (caches_[level].access(address)) {
                return level;
            }
        }
        return caches_.size();
    }

    std::size_t access_range(std::uint64_t address, std::size_t bytes) {
        if (bytes == 0) {
            return 0;
        }
        const std::uint64_t first = address / line_bytes_;
        const std::uint64_t last = (address + (bytes - 1)) / line_bytes_;
        std::size_t from_memory = 0;
        for (std::uint64_t line = first;; ++line) {
            if (access(line * line_bytes_) == caches_.size()) {
                ++from_memory;
            }
            if (line == last) {
                break;
            }
        }
        return from_memory;
    }

    std::uint64_t accesses(std::size_t level) const { return caches_.at(level).accesses(); }
    std::uint64_t misses(std::size_t level) const { return caches_.at(level).misses(); }

private:
    std::vector<practice::cachesim::Cache> caches_;
    std::uint64_t line_bytes_ = 1;
};

inline std::vector<std::uint64_t> miss_counts(std::span<const std::uint64_t> addresses,
                                              std::size_t line_bytes,
                                              std::span<const std::size_t> capacities) {
    // marks[i] is 1 while access i is the latest access of its line; the marks strictly between
    // two accesses of one line count the distinct lines touched in between.
    const std::size_t n = addresses.size();
    std::vector<std::int32_t> fenwick(n + 1, 0);
    const auto add = [&](std::size_t position, std::int32_t delta) {
        for (std::size_t i = position + 1; i <= n; i += i & (0 - i)) {
            fenwick[i] += delta;
        }
    };
    const auto prefix = [&](std::size_t end) {  // sum of marks in [0, end)
        std::int64_t sum = 0;
        for (std::size_t i = end; i > 0; i -= i & (0 - i)) {
            sum += fenwick[i];
        }
        return sum;
    };

    std::unordered_map<std::uint64_t, std::size_t> latest;
    latest.reserve(n);
    std::vector<std::uint64_t> at_distance;  // at_distance[d]: reuses with stack distance d
    std::uint64_t first_touches = 0;
    for (std::size_t i = 0; i < n; ++i) {
        const std::uint64_t line = addresses[i] / line_bytes;
        const auto found = latest.find(line);
        if (found == latest.end()) {
            ++first_touches;
            latest.emplace(line, i);
        } else {
            const std::size_t previous = found->second;
            const auto distance = static_cast<std::size_t>(prefix(i) - prefix(previous + 1));
            if (distance >= at_distance.size()) {
                at_distance.resize(distance + 1, 0);
            }
            ++at_distance[distance];
            add(previous, -1);
            found->second = i;
        }
        add(i, 1);
    }

    // at_least[d]: reuses whose distance is d or more, i.e. misses for a cache of d lines.
    std::vector<std::uint64_t> at_least(at_distance.size() + 1, 0);
    for (std::size_t d = at_distance.size(); d-- > 0;) {
        at_least[d] = at_least[d + 1] + at_distance[d];
    }
    std::vector<std::uint64_t> result;
    result.reserve(capacities.size());
    for (const std::size_t capacity : capacities) {
        result.push_back(first_touches + (capacity < at_least.size() ? at_least[capacity] : 0));
    }
    return result;
}
