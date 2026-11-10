#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <new>
#include <utility>

// Where raw chunks come from. The default forwards to operator new.
struct NewDeleteUpstream {
    void* allocate(std::size_t bytes) { return ::operator new(bytes); }
    void deallocate(void* p, std::size_t /*bytes*/) noexcept { ::operator delete(p); }
};

template <typename Upstream = NewDeleteUpstream>
class BinnedAllocator {
public:
    static constexpr std::size_t kBinCount = 9;
    static constexpr std::size_t kMaxBinSize = 4096;

    explicit BinnedAllocator(std::size_t chunk_bytes = 1 << 16, Upstream upstream = Upstream{})
        : chunk_bytes_(chunk_bytes), upstream_(std::move(upstream)) {}

    ~BinnedAllocator() {
        while (chunks_ != nullptr) {
            ChunkHeader* next = chunks_->next;
            upstream_.deallocate(chunks_, chunk_bytes_);
            chunks_ = next;
        }
    }

    BinnedAllocator(const BinnedAllocator&) = delete;
    BinnedAllocator& operator=(const BinnedAllocator&) = delete;

    static std::size_t bin_index(std::size_t size) {
        return size <= 16 ? 0 : static_cast<std::size_t>(std::bit_width(size - 1)) - 4;
    }
    static std::size_t bin_size(std::size_t index) { return std::size_t{16} << index; }

    void* allocate(std::size_t size) {
        live_bytes_ += size;
        if (size > kMaxBinSize) {
            ++large_allocations_;
            return ::operator new(size);
        }
        Bin& bin = bins_[bin_index(size)];
        if (bin.free != nullptr) {
            FreeBlock* block = bin.free;
            bin.free = block->next;
            return block;
        }
        const std::size_t block_bytes = bin_size(bin_index(size));
        if (bin.bump_left < block_bytes) {
            grab_chunk(bin);
        }
        char* block = bin.bump;
        bin.bump += block_bytes;
        bin.bump_left -= block_bytes;
        return block;
    }

    void deallocate(void* p, std::size_t size) {
        live_bytes_ -= size;
        if (size > kMaxBinSize) {
            ::operator delete(p);
            return;
        }
        Bin& bin = bins_[bin_index(size)];
        bin.free = ::new (p) FreeBlock{bin.free};
    }

    std::size_t large_allocations() const { return large_allocations_; }
    std::size_t live_bytes() const { return live_bytes_; }

private:
    struct FreeBlock {
        FreeBlock* next;
    };
    struct ChunkHeader {
        ChunkHeader* next;
        std::size_t padding;  // keeps blocks after the header aligned to 16
    };
    struct Bin {
        FreeBlock* free = nullptr;
        char* bump = nullptr;
        std::size_t bump_left = 0;
    };

    void grab_chunk(Bin& bin) {
        void* memory = upstream_.allocate(chunk_bytes_);
        chunks_ = ::new (memory) ChunkHeader{chunks_, 0};
        bin.bump = static_cast<char*>(memory) + sizeof(ChunkHeader);
        bin.bump_left = chunk_bytes_ - sizeof(ChunkHeader);
    }

    std::size_t chunk_bytes_;
    Upstream upstream_;
    ChunkHeader* chunks_ = nullptr;
    std::array<Bin, kBinCount> bins_{};
    std::size_t large_allocations_ = 0;
    std::size_t live_bytes_ = 0;
};
