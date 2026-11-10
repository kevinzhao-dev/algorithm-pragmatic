#pragma once

#include <bit>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

class BuddyAllocator {
public:
    explicit BuddyAllocator(int max_order) : max_order_(max_order) {
        if (max_order < 0 || max_order > 62) {
            throw std::invalid_argument("max_order out of range");
        }
        bins_.resize(static_cast<std::size_t>(max_order) + 1);
        bins_[static_cast<std::size_t>(max_order)].insert(0);
        free_units_ = std::size_t{1} << max_order;
    }

    std::optional<std::size_t> allocate(std::size_t units) {
        if (units == 0) {
            throw std::invalid_argument("units must be positive");
        }
        const int order = order_for(units);
        if (order > max_order_) {
            return std::nullopt;
        }
        int found = order;
        while (found <= max_order_ && bin(found).empty()) {
            ++found;
        }
        if (found > max_order_) {
            return std::nullopt;
        }
        const std::size_t offset = *bin(found).begin();
        bin(found).erase(bin(found).begin());
        // Split down: the high half of each split goes to the smaller bin.
        for (int k = found - 1; k >= order; --k) {
            bin(k).insert(offset + (std::size_t{1} << k));
        }
        order_of_[offset] = order;
        free_units_ -= std::size_t{1} << order;
        return offset;
    }

    std::size_t free_blocks(int order) const { return bins_[static_cast<std::size_t>(order)].size(); }
    std::size_t free_units() const { return free_units_; }

    void free(std::size_t offset) {
        const auto found = order_of_.find(offset);
        if (found == order_of_.end()) {
            throw std::invalid_argument("offset is not an allocated block");
        }
        int order = found->second;
        order_of_.erase(found);
        free_units_ += std::size_t{1} << order;
        std::size_t merged = offset;
        while (order < max_order_) {
            const std::size_t buddy = merged ^ (std::size_t{1} << order);
            auto& level = bin(order);
            const auto it = level.find(buddy);
            if (it == level.end()) {
                break;
            }
            level.erase(it);
            merged = std::min(merged, buddy);
            ++order;
        }
        bin(order).insert(merged);
    }

    std::size_t largest_free_block() const {
        for (int order = max_order_; order >= 0; --order) {
            if (!bins_[static_cast<std::size_t>(order)].empty()) {
                return std::size_t{1} << order;
            }
        }
        return 0;
    }

    double external_fragmentation() const {
        if (free_units_ == 0) {
            return 0.0;
        }
        return 1.0 - static_cast<double>(largest_free_block()) / static_cast<double>(free_units_);
    }

private:
    static int order_for(std::size_t units) {
        return units == 1 ? 0 : std::bit_width(units - 1);
    }

    std::set<std::size_t>& bin(int order) { return bins_[static_cast<std::size_t>(order)]; }

    int max_order_;
    std::vector<std::set<std::size_t>> bins_;
    std::map<std::size_t, int> order_of_;
    std::size_t free_units_ = 0;
};
