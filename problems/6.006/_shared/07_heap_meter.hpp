// 07_heap_meter.hpp — measures heap usage so tests can assert memory bounds deterministically.
//
// Include from exactly one .cpp per test binary: it replaces the global operator new/delete.
// Sizes come from malloc_size, so nothing changes about how blocks are laid out and sanitizers
// keep working through malloc and free.
//
//     TEST(3, "keeps a single row of the table") {
//         const auto input = make_input();
//         heap_meter::Scope scope;
//         const auto answer = solve(input);
//         CHECK_LE(scope.peak_bytes(), limit);
//     }
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <new>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

namespace heap_meter {

inline std::size_t live_bytes = 0;
inline std::size_t high_water_bytes = 0;

// Reports the peak heap usage above the level at construction.
class Scope {
public:
    Scope() : baseline_(live_bytes) { high_water_bytes = live_bytes; }
    std::size_t peak_bytes() const { return high_water_bytes - baseline_; }

private:
    std::size_t baseline_;
};

inline std::size_t block_size(void* block) {
#if defined(__APPLE__)
    return malloc_size(block);
#else
    return malloc_usable_size(block);
#endif
}

}  // namespace heap_meter

namespace heap_meter::detail {

inline void* acquire(std::size_t size) noexcept {
    void* block = std::malloc(size == 0 ? 1 : size);
    if (block != nullptr) {
        live_bytes += block_size(block);
        high_water_bytes = std::max(high_water_bytes, live_bytes);
    }
    return block;
}

inline void release(void* block) noexcept {
    if (block != nullptr) {
        live_bytes -= block_size(block);
        std::free(block);
    }
}

}  // namespace heap_meter::detail

void* operator new(std::size_t size) {
    if (void* block = heap_meter::detail::acquire(size)) {
        return block;
    }
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) {
    if (void* block = heap_meter::detail::acquire(size)) {
        return block;
    }
    throw std::bad_alloc();
}
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    return heap_meter::detail::acquire(size);
}
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    return heap_meter::detail::acquire(size);
}
void operator delete(void* block) noexcept { heap_meter::detail::release(block); }
void operator delete[](void* block) noexcept { heap_meter::detail::release(block); }
void operator delete(void* block, std::size_t) noexcept { heap_meter::detail::release(block); }
void operator delete[](void* block, std::size_t) noexcept { heap_meter::detail::release(block); }
void operator delete(void* block, const std::nothrow_t&) noexcept {
    heap_meter::detail::release(block);
}
void operator delete[](void* block, const std::nothrow_t&) noexcept {
    heap_meter::detail::release(block);
}
