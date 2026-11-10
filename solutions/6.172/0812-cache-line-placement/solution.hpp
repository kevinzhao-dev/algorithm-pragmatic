#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "08_cache_sim.hpp"

inline constexpr std::size_t kCacheLine = 64;

// Upstream provides void* allocate(std::size_t) (8-byte aligned) and void deallocate(void*).
template <typename Upstream>
class CacheAlignedWrapper {
public:
    explicit CacheAlignedWrapper(Upstream& upstream) : upstream_(upstream) {}

    void* allocate(std::size_t bytes) {
        if (bytes == 0) {
            return nullptr;
        }
        const std::size_t request = bytes + kCacheLine - 1 + sizeof(void*);
        void* raw = upstream_.allocate(request);
        overhead_ += request - bytes;
        const auto after_back_pointer = reinterpret_cast<std::uintptr_t>(raw) + sizeof(void*);
        const std::uintptr_t aligned = (after_back_pointer + kCacheLine - 1) & ~(kCacheLine - 1);
        auto* user = reinterpret_cast<char*>(aligned);
        std::memcpy(user - sizeof(void*), &raw, sizeof(raw));
        return user;
    }

    std::size_t overhead_bytes() const { return overhead_; }

    void deallocate(void* p) {
        if (p == nullptr) {
            return;
        }
        void* raw = nullptr;
        std::memcpy(&raw, static_cast<char*>(p) - sizeof(void*), sizeof(raw));
        upstream_.deallocate(raw);
    }

private:
    Upstream& upstream_;
    std::size_t overhead_ = 0;
};

class PackedAllocator {
public:
    explicit PackedAllocator(std::span<std::byte> heap) : heap_(heap) {}

    void* allocate(std::size_t bytes) {
        if (bytes == 0) {
            return nullptr;
        }
        const auto base = reinterpret_cast<std::uintptr_t>(heap_.data());
        std::uintptr_t start = align_up(base + cursor_ + kHeaderSize, 8);
        if (lines_spanned(start, bytes) > (bytes + kCacheLine - 1) / kCacheLine) {
            start = align_up(start, kCacheLine);
        }
        const std::size_t offset = static_cast<std::size_t>(start - base);
        if (offset + bytes > heap_.size()) {
            return nullptr;
        }
        const std::uint64_t header = bytes;
        std::memcpy(heap_.data() + offset - kHeaderSize, &header, kHeaderSize);
        cursor_ = offset + bytes;
        return heap_.data() + offset;
    }

    std::size_t size_of(const void* p) const {
        std::uint64_t header = 0;
        std::memcpy(&header, static_cast<const char*>(p) - kHeaderSize, kHeaderSize);
        return static_cast<std::size_t>(header);
    }

    std::size_t used_bytes() const { return cursor_; }

private:
    static constexpr std::size_t kHeaderSize = 8;

    static std::uintptr_t align_up(std::uintptr_t value, std::uintptr_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    static std::size_t lines_spanned(std::uintptr_t start, std::size_t bytes) {
        return (start + bytes - 1) / kCacheLine - start / kCacheLine + 1;
    }

    std::span<std::byte> heap_;
    std::size_t cursor_ = 0;
};

struct PlacementStats {
    double lines_per_access = 0.0;
    std::uint64_t misses = 0;
};

inline PlacementStats measure_placement(std::span<const std::uint64_t> addresses,
                                        std::span<const std::size_t> sizes,
                                        std::span<const std::size_t> order,
                                        practice::cachesim::Cache& cache) {
    const std::uint64_t misses_before = cache.misses();
    std::uint64_t lines = 0;
    for (const std::size_t i : order) {
        const std::uint64_t first = addresses[i] / kCacheLine;
        const std::uint64_t last = (addresses[i] + sizes[i] - 1) / kCacheLine;
        lines += last - first + 1;
        cache.access_range(addresses[i], sizes[i]);
    }
    PlacementStats stats;
    stats.lines_per_access =
        order.empty() ? 0.0 : static_cast<double>(lines) / static_cast<double>(order.size());
    stats.misses = cache.misses() - misses_before;
    return stats;
}
