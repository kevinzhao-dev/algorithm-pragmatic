// Replaces the global allocation functions so a test can assert how many heap allocations a
// call makes. This is how the 6.006 exercises check "O(1) extra space" deterministically.
//
// Include this header in exactly one test source file of an exercise.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>

namespace practice6006 {

inline std::uint64_t& allocation_count() {
    static std::uint64_t count = 0;
    return count;
}

// Counts the allocations made during its lifetime.
class AllocationScope {
public:
    AllocationScope() : start_(allocation_count()) {}
    std::uint64_t allocations() const { return allocation_count() - start_; }

private:
    std::uint64_t start_;
};

}  // namespace practice6006

void* operator new(std::size_t size) {
    ++practice6006::allocation_count();
    if (void* memory = std::malloc(size != 0 ? size : 1)) {
        return memory;
    }
    throw std::bad_alloc{};
}

void* operator new[](std::size_t size) { return ::operator new(size); }

void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }
