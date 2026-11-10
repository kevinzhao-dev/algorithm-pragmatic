// Replaces the global operator new so tests can observe heap use by the allocator under test.
#include "alloc_counter.hpp"

#include <atomic>
#include <cstdlib>
#include <new>

namespace {
std::atomic<std::uint64_t> new_calls{0};
}

namespace alloc_counter {
std::uint64_t operator_new_calls() { return new_calls.load(std::memory_order_relaxed); }
}  // namespace alloc_counter

void* operator new(std::size_t size) {
    new_calls.fetch_add(1, std::memory_order_relaxed);
    if (void* block = std::malloc(size == 0 ? 1 : size)) {
        return block;
    }
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return operator new(size); }
void operator delete(void* block) noexcept { std::free(block); }
void operator delete[](void* block) noexcept { std::free(block); }
void operator delete(void* block, std::size_t) noexcept { std::free(block); }
void operator delete[](void* block, std::size_t) noexcept { std::free(block); }
