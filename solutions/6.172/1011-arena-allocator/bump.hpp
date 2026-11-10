// bump.hpp — Stage 1: allocate by bumping a pointer; free is a no-op.
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>

#include "arena.hpp"
#include "block.hpp"

class BumpAllocator {
public:
    explicit BumpAllocator(std::size_t capacity) : arena_(capacity) {}

    void* malloc(std::size_t size) {
        if (size == 0 || size > arena_.capacity()) {
            return nullptr;
        }
        const std::size_t rounded = heap::align_up(size);
        const std::size_t payload_offset = heap::align_up(arena_.used() + heap::kWord);
        if (rounded > arena_.capacity() - payload_offset) {
            return nullptr;
        }
        arena_.grow(payload_offset + rounded - arena_.used());
        std::byte* payload = arena_.base() + payload_offset;
        heap::store(payload - heap::kWord, rounded);
        last_ = payload;
        return payload;
    }

    void free(void* ptr) {}

    void* realloc(void* ptr, std::size_t size) {
        if (ptr == nullptr) {
            return malloc(size);
        }
        if (size == 0) {
            free(ptr);
            return nullptr;
        }
        if (size > arena_.capacity()) {
            return nullptr;
        }
        auto* payload = static_cast<std::byte*>(ptr);
        const std::size_t capacity = heap::load(payload - heap::kWord);
        if (size <= capacity) {
            return payload;
        }
        if (payload == last_ && extend_last(payload, size)) {
            return payload;
        }
        void* fresh = malloc(size);
        if (fresh != nullptr) {
            std::memcpy(fresh, payload, capacity);
        }
        return fresh;
    }

    bool check() const {
        const std::size_t used = arena_.used();
        if (used == 0) {
            return true;
        }
        std::size_t offset = 2 * heap::kWord;
        while (offset <= used) {
            const std::size_t capacity = heap::load(arena_.base() + offset - heap::kWord);
            if (capacity == 0 || capacity % heap::kAlignment != 0 || capacity > used - offset) {
                return false;
            }
            if (offset + capacity == used) {
                return true;
            }
            offset = heap::align_up(offset + capacity + heap::kWord);
        }
        return false;
    }

    std::size_t heap_bytes() const { return arena_.used(); }
    const Arena& arena() const { return arena_; }

private:
    bool extend_last(std::byte* payload, std::size_t size) {
        const auto payload_offset = static_cast<std::size_t>(payload - arena_.base());
        const std::size_t rounded = heap::align_up(size);
        if (rounded > arena_.capacity() - payload_offset) {
            return false;
        }
        arena_.grow(payload_offset + rounded - arena_.used());
        heap::store(payload - heap::kWord, rounded);
        return true;
    }

    Arena arena_;
    std::byte* last_ = nullptr;
};
