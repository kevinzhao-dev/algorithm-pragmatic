#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <list>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class CacheSimulator {
public:
    CacheSimulator(std::size_t capacity_bytes, std::size_t line_bytes, std::size_t ways)
        : line_bytes_(line_bytes), ways_(ways) {
        if (capacity_bytes == 0 || !std::has_single_bit(line_bytes) || ways == 0 ||
            capacity_bytes % (line_bytes * ways) != 0) {
            throw std::invalid_argument("capacity must be sets * ways * line");
        }
        sets_ = capacity_bytes / (line_bytes * ways);
        if (!std::has_single_bit(sets_)) {
            throw std::invalid_argument("the number of sets must be a power of two");
        }
        shift_ = std::countr_zero(line_bytes);
        lines_ = sets_ * ways_;
        recency_.assign(sets_, {});
    }

    bool access(std::uint64_t address) {
        const std::uint64_t line = address >> shift_;
        ++accesses_;
        const bool shadow_hit = touch_shadow(line);
        std::vector<std::uint64_t>& set = recency_[static_cast<std::size_t>(line & (sets_ - 1))];
        const auto found = std::find(set.begin(), set.end(), line);
        if (found != set.end()) {
            std::rotate(set.begin(), found, found + 1);
            return true;
        }
        ++misses_;
        if (seen_.insert(line).second) {
            ++cold_;
        } else if (shadow_hit) {
            ++conflict_;
        } else {
            ++capacity_;
        }
        set.insert(set.begin(), line);
        if (set.size() > ways_) {
            set.pop_back();
        }
        return false;
    }

    void access_range(std::uint64_t address, std::size_t bytes) {
        if (bytes == 0) {
            return;
        }
        const std::uint64_t first = address >> shift_;
        const std::uint64_t last = (address + (bytes - 1)) >> shift_;
        for (std::uint64_t line = first;; ++line) {  // also ends at the top of the address space
            access(line << shift_);
            if (line == last) {
                break;
            }
        }
    }

    std::uint64_t accesses() const { return accesses_; }
    std::uint64_t misses() const { return misses_; }
    std::uint64_t hits() const { return accesses_ - misses_; }

    std::uint64_t cold_misses() const { return cold_; }
    std::uint64_t conflict_misses() const { return conflict_; }
    std::uint64_t capacity_misses() const { return capacity_; }

    bool contains(std::uint64_t address) const {
        const std::uint64_t line = address >> shift_;
        const auto& set = recency_[static_cast<std::size_t>(line & (sets_ - 1))];
        return std::find(set.begin(), set.end(), line) != set.end();
    }

    void evict(std::uint64_t address) {
        const std::uint64_t line = address >> shift_;
        auto& set = recency_[static_cast<std::size_t>(line & (sets_ - 1))];
        set.erase(std::remove(set.begin(), set.end(), line), set.end());
    }

    std::size_t line_bytes() const { return line_bytes_; }

private:
    bool touch_shadow(std::uint64_t line) {
        const auto found = shadow_index_.find(line);
        if (found != shadow_index_.end()) {
            shadow_.splice(shadow_.begin(), shadow_, found->second);
            return true;
        }
        shadow_.push_front(line);
        shadow_index_[line] = shadow_.begin();
        if (shadow_.size() > lines_) {
            shadow_index_.erase(shadow_.back());
            shadow_.pop_back();
        }
        return false;
    }

    std::size_t line_bytes_;
    std::size_t ways_;
    std::size_t sets_ = 0;
    std::size_t lines_ = 0;
    int shift_ = 0;
    std::vector<std::vector<std::uint64_t>> recency_;

    std::uint64_t accesses_ = 0;
    std::uint64_t misses_ = 0;
    std::uint64_t cold_ = 0;
    std::uint64_t conflict_ = 0;
    std::uint64_t capacity_ = 0;
    std::unordered_set<std::uint64_t> seen_;
    std::list<std::uint64_t> shadow_;
    std::unordered_map<std::uint64_t, std::list<std::uint64_t>::iterator> shadow_index_;
};

class CoherentCaches {
public:
    CoherentCaches(std::size_t cores, std::size_t capacity_bytes, std::size_t line_bytes,
                   std::size_t ways)
        : line_bytes_(line_bytes), invalidated_(cores) {
        caches_.reserve(cores);
        for (std::size_t i = 0; i < cores; ++i) {
            caches_.emplace_back(capacity_bytes, line_bytes, ways);
        }
    }

    bool read(std::size_t core, std::uint64_t address) { return touch(core, address); }

    bool write(std::size_t core, std::uint64_t address) {
        const bool hit = touch(core, address);
        const std::uint64_t line = address / line_bytes_;
        for (std::size_t other = 0; other < caches_.size(); ++other) {
            if (other != core && caches_[other].contains(address)) {
                caches_[other].evict(address);
                invalidated_[other].insert(line);
            }
        }
        return hit;
    }

    std::uint64_t coherence_misses() const { return coherence_; }

private:
    bool touch(std::size_t core, std::uint64_t address) {
        const std::uint64_t line = address / line_bytes_;
        const bool hit = caches_[core].access(address);
        if (!hit && invalidated_[core].erase(line) == 1) {
            ++coherence_;
        }
        return hit;
    }

    std::size_t line_bytes_;
    std::vector<CacheSimulator> caches_;
    std::vector<std::unordered_set<std::uint64_t>> invalidated_;
    std::uint64_t coherence_ = 0;
};
