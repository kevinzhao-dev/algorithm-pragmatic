// Replaces global new/delete with versions that record counts and live bytes.
// Include this from exactly one translation unit.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>

namespace alloc_counter {

struct Stats {
    std::uint64_t allocations = 0;
    std::int64_t live_bytes = 0;
    std::int64_t peak_bytes = 0;
};

inline Stats& stats() {
    static Stats value;
    return value;
}

inline void start() {
    Stats& s = stats();
    s.allocations = 0;
    s.peak_bytes = s.live_bytes;
}

inline void* record_new(std::size_t size) {
    // Every block carries its size in a 16-byte header, so frees can update live_bytes.
    auto* block = static_cast<std::byte*>(std::malloc(size + 16));
    if (block == nullptr) {
        throw std::bad_alloc();
    }
    *reinterpret_cast<std::size_t*>(block) = size;
    Stats& s = stats();
    ++s.allocations;
    s.live_bytes += static_cast<std::int64_t>(size);
    if (s.live_bytes > s.peak_bytes) {
        s.peak_bytes = s.live_bytes;
    }
    return block + 16;
}

inline void* record_new_nothrow(std::size_t size) noexcept {
    auto* block = static_cast<std::byte*>(std::malloc(size + 16));
    if (block == nullptr) {
        return nullptr;
    }
    *reinterpret_cast<std::size_t*>(block) = size;
    Stats& s = stats();
    ++s.allocations;
    s.live_bytes += static_cast<std::int64_t>(size);
    if (s.live_bytes > s.peak_bytes) {
        s.peak_bytes = s.live_bytes;
    }
    return block + 16;
}

inline void record_delete(void* pointer) noexcept {
    if (pointer == nullptr) {
        return;
    }
    auto* block = static_cast<std::byte*>(pointer) - 16;
    stats().live_bytes -= static_cast<std::int64_t>(*reinterpret_cast<std::size_t*>(block));
    std::free(block);
}

}  // namespace alloc_counter

// Every non-aligned form is replaced: libc++ also allocates through the nothrow overloads
// (temporary buffers in std::stable_sort) and frees through the same delete family.
void* operator new(std::size_t size) { return alloc_counter::record_new(size); }
void* operator new[](std::size_t size) { return alloc_counter::record_new(size); }
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    return alloc_counter::record_new_nothrow(size);
}
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    return alloc_counter::record_new_nothrow(size);
}
void operator delete(void* p) noexcept { alloc_counter::record_delete(p); }
void operator delete[](void* p) noexcept { alloc_counter::record_delete(p); }
void operator delete(void* p, std::size_t) noexcept { alloc_counter::record_delete(p); }
void operator delete[](void* p, std::size_t) noexcept { alloc_counter::record_delete(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { alloc_counter::record_delete(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { alloc_counter::record_delete(p); }
