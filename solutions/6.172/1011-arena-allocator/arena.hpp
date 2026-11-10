// arena.hpp — a fixed anonymous mapping that only ever grows from the front, like sbrk.
#pragma once

#include <sys/mman.h>
#include <unistd.h>

#include <cstddef>
#include <new>
#include <utility>

class Arena {
public:
    explicit Arena(std::size_t capacity) {
        const auto page = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
        const std::size_t wanted = capacity == 0 ? 1 : capacity;
        capacity_ = (wanted + page - 1) / page * page;
        void* mapped = mmap(nullptr, capacity_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (mapped == MAP_FAILED) {
            throw std::bad_alloc();
        }
        base_ = static_cast<std::byte*>(mapped);
    }

    ~Arena() { release(); }

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    Arena(Arena&& other) noexcept
        : base_(std::exchange(other.base_, nullptr)),
          capacity_(std::exchange(other.capacity_, 0)),
          used_(std::exchange(other.used_, 0)) {}

    Arena& operator=(Arena&& other) noexcept {
        if (this != &other) {
            release();
            base_ = std::exchange(other.base_, nullptr);
            capacity_ = std::exchange(other.capacity_, 0);
            used_ = std::exchange(other.used_, 0);
        }
        return *this;
    }

    // Extends the used prefix and returns its old end; nullptr (and no change) if it does not fit.
    std::byte* grow(std::size_t bytes) {
        if (bytes > capacity_ - used_) {
            return nullptr;
        }
        std::byte* start = base_ + used_;
        used_ += bytes;
        return start;
    }

    std::byte* base() const { return base_; }
    std::size_t used() const { return used_; }
    std::size_t capacity() const { return capacity_; }

private:
    void release() noexcept {
        if (base_ != nullptr) {
            munmap(base_, capacity_);
            base_ = nullptr;
        }
    }

    std::byte* base_ = nullptr;
    std::size_t capacity_ = 0;
    std::size_t used_ = 0;
};
