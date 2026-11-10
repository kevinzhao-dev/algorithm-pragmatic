#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "practice.hpp"

// Stage 1
class FixedAligned64 {
public:
    explicit FixedAligned64(std::span<std::byte> heap) { PRACTICE_TODO(); }

    void* allocate() { PRACTICE_TODO(); }
    void deallocate(void* p) { PRACTICE_TODO(); }
    std::size_t used_bytes() const { PRACTICE_TODO(); }
};

// Stage 2
class SmartPtr {
public:
    SmartPtr() = default;

    static SmartPtr make(void* address, bool is_small) { PRACTICE_TODO(); }

    void* get() const { PRACTICE_TODO(); }
    bool is_small() const { PRACTICE_TODO(); }
    explicit operator bool() const { PRACTICE_TODO(); }

    friend bool operator==(const SmartPtr& a, const SmartPtr& b) { PRACTICE_TODO(); }

private:
    std::uintptr_t bits_ = 0;
};

// Stage 3
class SmartAllocator {
public:
    explicit SmartAllocator(std::span<std::byte> heap) { PRACTICE_TODO(); }

    SmartPtr allocate(std::size_t bytes) { PRACTICE_TODO(); }
    void deallocate(SmartPtr p) { PRACTICE_TODO(); }
    std::size_t used_bytes() const { PRACTICE_TODO(); }
};
