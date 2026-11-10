// binned.hpp — Stage 3: segregated free lists with bitmap lookup.
//
// Allocated blocks carry only a header; free blocks also hold two links and a footer. The
// "previous block is allocated" bit in every header lets an allocated block skip its footer.
#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstring>

#include "arena.hpp"
#include "block.hpp"

class BinnedAllocator {
public:
    explicit BinnedAllocator(std::size_t capacity) : arena_(capacity) {
        std::byte* start = arena_.grow(kInitialBytes);
        heap::store(start + heap::kWord, heap::pack(kPrologueSize, heap::kAllocated | heap::kPrevAllocated));
        first_ = start + heap::kWord + kPrologueSize;
        heap::store(first_, heap::pack(0, heap::kAllocated | heap::kPrevAllocated));
    }

    void* malloc(std::size_t size) {
        if (size == 0 || size > arena_.capacity()) {
            return nullptr;
        }
        const std::size_t need = block_size_for(size);
        if (std::byte* block = find_fit(need)) {
            place(block, need);
            return heap::payload_of(block);
        }
        std::byte* block = extend_heap(need);
        return block == nullptr ? nullptr : heap::payload_of(block);
    }

    void free(void* ptr) {
        if (ptr == nullptr) {
            return;
        }
        std::byte* block = heap::block_of(ptr);
        release(block, heap::size_of(block), previous_allocated(block));
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
            unlink_free(next);
            available += heap::size_of(next);
        }
        if (available >= need) {
            write_allocated(block, available, previous_allocated(block));
            set_previous_allocated(block + available, true);
            shrink(block, available, need);
            return ptr;
        }
        std::byte* after = block + available;
        if (is_epilogue(after) && arena_.grow(need - available) != nullptr) {
            write_allocated(block, need, previous_allocated(block));
            write_epilogue(block + need, true);
            return ptr;
        }
        if (available != have) {
            // Could not grow in place after all: give the neighbour back to its bin.
            link_free(next);
        }

        void* fresh = malloc(size);
        if (fresh != nullptr) {
            std::memcpy(fresh, ptr, std::min(have - heap::kWord, size));
            free(ptr);
        }
        return fresh;
    }

    bool check() const {
        const std::byte* epilogue = epilogue_position();
        const std::byte* block = first_;
        bool previous_allocated_block = true;
        std::size_t free_blocks = 0;
        while (block != epilogue) {
            if (block > epilogue) {
                return false;
            }
            const auto limit = static_cast<std::size_t>(epilogue - block);
            const std::size_t size = heap::size_of(block);
            if (size < kMinBlock || size > limit || previous_allocated(block) != previous_allocated_block) {
                return false;
            }
            if (heap::is_allocated(block)) {
                previous_allocated_block = true;
            } else {
                if (!previous_allocated_block || heap::load(block + size - heap::kWord) != heap::load(block)) {
                    return false;
                }
                previous_allocated_block = false;
                ++free_blocks;
            }
            block += size;
        }
        return heap::load(epilogue) == epilogue_word(previous_allocated_block) &&
               bins_are_consistent(free_blocks);
    }

    std::size_t heap_bytes() const { return arena_.used(); }
    const Arena& arena() const { return arena_; }

private:
    static constexpr std::size_t kMinBlock = 32;
    static constexpr std::size_t kPrologueSize = 2 * heap::kWord;
    static constexpr std::size_t kInitialBytes = heap::kWord + kPrologueSize + heap::kWord;

    // Bins: exact sizes 32, 48, ..., 1008, then four bins per power of two.
    static constexpr std::size_t kExactLimit = 1024;
    static constexpr std::size_t kExactBins = (kExactLimit - kMinBlock) / heap::kAlignment;
    static constexpr int kFirstOctave = 10;
    static constexpr int kOctaves = 28;
    static constexpr std::size_t kBins = kExactBins + kOctaves * 4;
    static constexpr int kWordBits = 64;
    static constexpr std::size_t kScanLimit = 32;

    static std::size_t block_size_for(std::size_t payload) {
        return std::max(kMinBlock, heap::align_up(payload + heap::kWord));
    }

    static std::size_t bin_index(std::size_t size) {
        if (size < kExactLimit) {
            return size / heap::kAlignment - kMinBlock / heap::kAlignment;
        }
        const int octave = std::bit_width(size) - 1;
        const std::size_t sub = (size >> (octave - 2)) & 3;
        return kExactBins + static_cast<std::size_t>(octave - kFirstOctave) * 4 + sub;
    }

    std::byte* epilogue_position() const { return arena_.base() + arena_.used() - heap::kWord; }
    bool is_epilogue(const std::byte* block) const { return block == epilogue_position(); }

    static bool previous_allocated(const std::byte* block) {
        return (heap::load(block) & heap::kPrevAllocated) != 0;
    }

    static void set_previous_allocated(std::byte* block, bool allocated) {
        const std::uint64_t word = heap::load(block);
        heap::store(block, allocated ? word | heap::kPrevAllocated : word & ~heap::kPrevAllocated);
    }

    static std::uint64_t epilogue_word(bool previous_is_allocated) {
        return heap::pack(0, heap::kAllocated | (previous_is_allocated ? heap::kPrevAllocated : 0));
    }

    static void write_epilogue(std::byte* block, bool previous_is_allocated) {
        heap::store(block, epilogue_word(previous_is_allocated));
    }

    static void write_allocated(std::byte* block, std::size_t size, bool previous_is_allocated) {
        heap::store(block, heap::pack(size, heap::kAllocated | (previous_is_allocated ? heap::kPrevAllocated : 0)));
    }

    static void write_free(std::byte* block, std::size_t size, bool previous_is_allocated) {
        const std::uint64_t word = heap::pack(size, previous_is_allocated ? heap::kPrevAllocated : 0);
        heap::store(block, word);
        heap::store(block + size - heap::kWord, word);
    }

    // The free-list links live in the first 16 payload bytes of a free block.
    static std::byte* next_of(const std::byte* block) { return load_link(block + heap::kWord); }
    static std::byte* prev_of(const std::byte* block) { return load_link(block + 2 * heap::kWord); }
    static void set_next(std::byte* block, std::byte* next) { store_link(block + heap::kWord, next); }
    static void set_prev(std::byte* block, std::byte* prev) { store_link(block + 2 * heap::kWord, prev); }

    static std::byte* load_link(const std::byte* at) {
        std::byte* link;
        std::memcpy(static_cast<void*>(&link), at, sizeof link);
        return link;
    }

    static void store_link(std::byte* at, std::byte* link) {
        std::memcpy(at, static_cast<const void*>(&link), sizeof link);
    }

    void link_free(std::byte* block) {
        const std::size_t index = bin_index(heap::size_of(block));
        std::byte* head = bins_[index];
        set_next(block, head);
        set_prev(block, nullptr);
        if (head != nullptr) {
            set_prev(head, block);
        }
        bins_[index] = block;
        mark_bin(index, true);
    }

    void unlink_free(std::byte* block) {
        const std::size_t index = bin_index(heap::size_of(block));
        std::byte* next = next_of(block);
        std::byte* prev = prev_of(block);
        if (next != nullptr) {
            set_prev(next, prev);
        }
        if (prev != nullptr) {
            set_next(prev, next);
        } else {
            bins_[index] = next;
            if (next == nullptr) {
                mark_bin(index, false);
            }
        }
    }

    void mark_bin(std::size_t index, bool non_empty) {
        const std::uint64_t bit = std::uint64_t{1} << (index % kWordBits);
        std::uint64_t& word = occupied_[index / kWordBits];
        word = non_empty ? word | bit : word & ~bit;
    }

    // Index of the first non-empty bin at or above `index`, or kBins.
    std::size_t next_occupied(std::size_t index) const {
        std::size_t word_index = index / kWordBits;
        if (word_index >= occupied_.size()) {
            return kBins;
        }
        std::uint64_t word = occupied_[word_index] & (~std::uint64_t{0} << (index % kWordBits));
        while (word == 0) {
            if (++word_index >= occupied_.size()) {
                return kBins;
            }
            word = occupied_[word_index];
        }
        return word_index * kWordBits + static_cast<std::size_t>(std::countr_zero(word));
    }

    std::byte* find_fit(std::size_t need) const {
        std::size_t index = bin_index(need);
        if (index >= kExactBins) {
            std::size_t scanned = 0;
            for (std::byte* block = bins_[index]; block != nullptr && scanned < kScanLimit;
                 block = next_of(block), ++scanned) {
                if (heap::size_of(block) >= need) {
                    return block;
                }
            }
            ++index;
        }
        const std::size_t found = next_occupied(index);
        return found == kBins ? nullptr : bins_[found];
    }

    void place(std::byte* block, std::size_t need) {
        unlink_free(block);
        const std::size_t have = heap::size_of(block);
        const bool previous = previous_allocated(block);
        if (have - need >= kMinBlock) {
            write_allocated(block, need, previous);
            write_free(block + need, have - need, true);
            link_free(block + need);
        } else {
            write_allocated(block, have, previous);
            set_previous_allocated(block + have, true);
        }
    }

    std::byte* extend_heap(std::size_t need) {
        std::byte* block = epilogue_position();
        bool previous = previous_allocated(block);
        std::size_t tail = 0;
        if (!previous) {
            tail = heap::size_of(block - heap::kWord);
            block -= tail;
            unlink_free(block);
            previous = previous_allocated(block);
        }
        if (arena_.grow(need - tail) == nullptr) {
            if (tail != 0) {
                link_free(block);
            }
            return nullptr;
        }
        write_allocated(block, need, previous);
        write_epilogue(block + need, true);
        return block;
    }

    void shrink(std::byte* block, std::size_t have, std::size_t need) {
        if (have - need < kMinBlock) {
            return;
        }
        write_allocated(block, need, previous_allocated(block));
        release(block + need, have - need, true);
    }

    // Frees [block, block + size) given whether the block before it is allocated.
    void release(std::byte* block, std::size_t size, bool previous) {
        std::byte* next = block + size;
        if (!heap::is_allocated(next)) {
            unlink_free(next);
            size += heap::size_of(next);
            next = block + size;
        }
        if (!previous) {
            const std::size_t back = heap::size_of(block - heap::kWord);
            block -= back;
            size += back;
            unlink_free(block);
            previous = previous_allocated(block);
        }
        write_free(block, size, previous);
        set_previous_allocated(next, false);
        link_free(block);
    }

    bool bins_are_consistent(std::size_t free_blocks) const {
        std::size_t seen = 0;
        for (std::size_t index = 0; index < kBins; ++index) {
            const std::byte* node = bins_[index];
            const bool occupied = ((occupied_[index / kWordBits] >> (index % kWordBits)) & 1) != 0;
            if (occupied != (node != nullptr)) {
                return false;
            }
            const std::byte* previous = nullptr;
            while (node != nullptr) {
                if (++seen > free_blocks || !is_valid_free_block(node, index) || prev_of(node) != previous) {
                    return false;
                }
                previous = node;
                node = next_of(node);
            }
        }
        return seen == free_blocks;
    }

    bool is_valid_free_block(const std::byte* node, std::size_t index) const {
        const std::byte* epilogue = epilogue_position();
        if (node < first_ || node >= epilogue) {
            return false;
        }
        if ((reinterpret_cast<std::uintptr_t>(node) + heap::kWord) % heap::kAlignment != 0) {
            return false;
        }
        return !heap::is_allocated(node) && heap::size_of(node) >= kMinBlock &&
               heap::size_of(node) <= static_cast<std::size_t>(epilogue - node) &&
               bin_index(heap::size_of(node)) == index;
    }

    Arena arena_;
    std::byte* first_ = nullptr;
    std::array<std::byte*, kBins> bins_{};
    std::array<std::uint64_t, kBins / kWordBits + 1> occupied_{};
};
