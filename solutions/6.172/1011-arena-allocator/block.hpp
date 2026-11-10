// block.hpp — the raw word layout shared by the implicit-list and binned allocators.
//
// A block is [header][payload...]; the header word sits right before the payload and holds the
// whole block size (a multiple of 16) in its high bits. Blocks are contiguous in address order.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace heap {

inline constexpr std::size_t kAlignment = 16;
inline constexpr std::size_t kWord = sizeof(std::uint64_t);
inline constexpr std::uint64_t kSizeMask = ~std::uint64_t{15};
inline constexpr std::uint64_t kAllocated = 1;
inline constexpr std::uint64_t kPrevAllocated = 2;

constexpr std::size_t align_up(std::size_t n) { return (n + kAlignment - 1) & ~(kAlignment - 1); }

inline std::uint64_t load(const std::byte* at) {
    std::uint64_t word;
    std::memcpy(&word, at, kWord);
    return word;
}

inline void store(std::byte* at, std::uint64_t word) { std::memcpy(at, &word, kWord); }

inline std::uint64_t pack(std::size_t size, std::uint64_t flags) { return size | flags; }
inline std::size_t size_of(const std::byte* block) { return load(block) & kSizeMask; }
inline bool is_allocated(const std::byte* block) { return (load(block) & kAllocated) != 0; }
inline std::byte* block_of(void* payload) { return static_cast<std::byte*>(payload) - kWord; }
inline void* payload_of(std::byte* block) { return block + kWord; }

}  // namespace heap
