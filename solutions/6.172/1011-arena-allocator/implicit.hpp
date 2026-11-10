// implicit.hpp — Stage 2: first-fit over an implicit free list with boundary tags.
//
// Heap layout: [padding word][prologue block: 16 bytes, allocated][blocks...][epilogue header].
// The prologue and the epilogue (size 0, allocated) remove every boundary special case.
// Every block, allocated or free, has a header and a matching footer.
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>

#include "arena.hpp"
#include "block.hpp"

class ImplicitAllocator {
public:
    explicit ImplicitAllocator(std::size_t capacity) : arena_(capacity) {
        std::byte* start = arena_.grow(kInitialBytes);
        write_block(start + kPrologueOffset, kPrologueSize, true);
        first_ = start + kPrologueOffset + kPrologueSize;
        heap::store(first_, heap::pack(0, heap::kAllocated));
    }

    void* malloc(std::size_t size) {
        if (size == 0 || size > arena_.capacity()) {
            return nullptr;
        }
        const std::size_t need = block_size_for(size);
        std::byte* block = find_fit(need);
        if (block == nullptr) {
            block = extend_heap(need);
        }
        if (block == nullptr) {
            return nullptr;
        }
        place(block, need);
        return heap::payload_of(block);
    }

    void free(void* ptr) {
        if (ptr == nullptr) {
            return;
        }
        std::byte* block = heap::block_of(ptr);
        std::size_t size = heap::size_of(block);
        release(block, size);
    }

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
        std::byte* block = heap::block_of(ptr);
        const std::size_t have = heap::size_of(block);
        const std::size_t need = block_size_for(size);
        if (need <= have) {
            shrink(block, have, need);
            return ptr;
        }

        std::byte* next = block + have;
        std::size_t available = have;
        if (!heap::is_allocated(next)) {
            available += heap::size_of(next);
        }
        if (available >= need) {
            write_block(block, available, true);
            shrink(block, available, need);
            return ptr;
        }
        std::byte* after = block + available;
        if (is_epilogue(after) && arena_.grow(need - available) != nullptr) {
            write_block(block, need, true);
            heap::store(block + need, heap::pack(0, heap::kAllocated));
            return ptr;
        }

        void* fresh = malloc(size);
        if (fresh != nullptr) {
            std::memcpy(fresh, ptr, std::min(have - 2 * heap::kWord, size));
            free(ptr);
        }
        return fresh;
    }

    bool check() const {
        const std::byte* epilogue = epilogue_position();
        const std::byte* block = first_;
        bool previous_free = false;
        while (block != epilogue) {
            if (block > epilogue) {
                return false;
            }
            const auto limit = static_cast<std::size_t>(epilogue - block);
            const std::size_t size = heap::size_of(block);
            if (size < kMinBlock || size > limit) {
                return false;
            }
            if (heap::load(block) != heap::load(block + size - heap::kWord)) {
                return false;
            }
            const bool is_free = !heap::is_allocated(block);
            if (is_free && previous_free) {
                return false;
            }
            previous_free = is_free;
            block += size;
        }
        return heap::load(epilogue) == heap::pack(0, heap::kAllocated);
    }

    std::size_t heap_bytes() const { return arena_.used(); }
    const Arena& arena() const { return arena_; }

private:
    static constexpr std::size_t kMinBlock = 32;
    static constexpr std::size_t kPrologueOffset = heap::kWord;
    static constexpr std::size_t kPrologueSize = 2 * heap::kWord;
    static constexpr std::size_t kInitialBytes = kPrologueOffset + kPrologueSize + heap::kWord;

    static std::size_t block_size_for(std::size_t payload) {
        return std::max(kMinBlock, heap::align_up(payload + 2 * heap::kWord));
    }

    std::byte* epilogue_position() const { return arena_.base() + arena_.used() - heap::kWord; }
    bool is_epilogue(const std::byte* block) const { return block == epilogue_position(); }

    static void write_block(std::byte* block, std::size_t size, bool allocated) {
        const std::uint64_t word = heap::pack(size, allocated ? heap::kAllocated : 0);
        heap::store(block, word);
        heap::store(block + size - heap::kWord, word);
    }

    static bool previous_is_free(const std::byte* block) {
        return !heap::is_allocated(block - heap::kWord);
    }

    std::byte* find_fit(std::size_t need) const {
        for (std::byte* block = first_; !is_epilogue(block); block += heap::size_of(block)) {
            if (!heap::is_allocated(block) && heap::size_of(block) >= need) {
                return block;
            }
        }
        return nullptr;
    }

    std::byte* extend_heap(std::size_t need) {
        std::byte* block = epilogue_position();
        std::size_t tail = 0;
        if (previous_is_free(block)) {
            tail = heap::size_of(block - heap::kWord);
            block -= tail;
        }
        if (arena_.grow(need - tail) == nullptr) {
            return nullptr;
        }
        write_block(block, need, false);
        heap::store(block + need, heap::pack(0, heap::kAllocated));
        return block;
    }

    static void place(std::byte* block, std::size_t need) {
        const std::size_t have = heap::size_of(block);
        if (have - need >= kMinBlock) {
            write_block(block, need, true);
            write_block(block + need, have - need, false);
        } else {
            write_block(block, have, true);
        }
    }

    // Keeps an allocated block at `need` bytes, returning any big enough tail to the free list.
    void shrink(std::byte* block, std::size_t have, std::size_t need) {
        if (have - need < kMinBlock) {
            return;
        }
        write_block(block, need, true);
        release(block + need, have - need);
    }

    // Frees [block, block + size), merging with both neighbours.
    void release(std::byte* block, std::size_t size) {
        std::byte* next = block + size;
        if (!heap::is_allocated(next)) {
            size += heap::size_of(next);
        }
        if (previous_is_free(block)) {
            const std::size_t previous = heap::size_of(block - heap::kWord);
            block -= previous;
            size += previous;
        }
        write_block(block, size, false);
    }

    Arena arena_;
    std::byte* first_ = nullptr;
};
