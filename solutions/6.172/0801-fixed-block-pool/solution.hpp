#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <new>
#include <utility>

// Where raw chunks come from. The default forwards to operator new.
struct NewDeleteUpstream {
    void* allocate(std::size_t bytes) { return ::operator new(bytes); }
    void deallocate(void* p, std::size_t /*bytes*/) noexcept { ::operator delete(p); }
};

template <typename Upstream = NewDeleteUpstream>
class FixedBlockPool {
public:
    FixedBlockPool(std::size_t block_size, std::size_t blocks_per_chunk,
                   Upstream upstream = Upstream{})
        : block_size_(round_up(block_size)),
          blocks_per_chunk_(blocks_per_chunk),
          upstream_(std::move(upstream)) {}

    ~FixedBlockPool() { release_chunks(); }

    FixedBlockPool(const FixedBlockPool&) = delete;
    FixedBlockPool& operator=(const FixedBlockPool&) = delete;

    FixedBlockPool(FixedBlockPool&& other) noexcept
        : block_size_(other.block_size_),
          blocks_per_chunk_(other.blocks_per_chunk_),
          upstream_(std::move(other.upstream_)),
          free_list_(std::exchange(other.free_list_, nullptr)),
          chunks_(std::exchange(other.chunks_, nullptr)),
          bump_(std::exchange(other.bump_, nullptr)),
          bump_left_(std::exchange(other.bump_left_, 0)),
          chunk_count_(std::exchange(other.chunk_count_, 0)),
          live_(std::exchange(other.live_, 0)) {}

    FixedBlockPool& operator=(FixedBlockPool&& other) noexcept {
        if (this != &other) {
            release_chunks();
            block_size_ = other.block_size_;
            blocks_per_chunk_ = other.blocks_per_chunk_;
            upstream_ = std::move(other.upstream_);
            free_list_ = std::exchange(other.free_list_, nullptr);
            chunks_ = std::exchange(other.chunks_, nullptr);
            bump_ = std::exchange(other.bump_, nullptr);
            bump_left_ = std::exchange(other.bump_left_, 0);
            chunk_count_ = std::exchange(other.chunk_count_, 0);
            live_ = std::exchange(other.live_, 0);
        }
        return *this;
    }

    std::size_t block_size() const { return block_size_; }
    std::size_t allocated_blocks() const { return live_; }
    std::size_t chunk_count() const { return chunk_count_; }

    void* allocate() {
        if (free_list_ != nullptr) {
            FreeBlock* block = std::exchange(free_list_, free_list_->next);
            ++live_;
            return block;
        }
        if (bump_left_ == 0) {
            grow();
        }
        void* block = std::exchange(bump_, bump_ + block_size_);
        --bump_left_;
        ++live_;
        return block;
    }

    void deallocate(void* block) {
        if (block != nullptr) {
            free_list_ = ::new (block) FreeBlock{free_list_};
            --live_;
        }
    }

private:
    struct FreeBlock {
        FreeBlock* next;
    };
    struct ChunkHeader {
        ChunkHeader* next;
        std::size_t bytes;
    };

    static std::size_t round_up(std::size_t bytes) {
        return bytes <= 16 ? 16 : (bytes + 15) & ~std::size_t{15};
    }

    void grow() {
        const std::size_t bytes = sizeof(ChunkHeader) + blocks_per_chunk_ * block_size_;
        void* memory = upstream_.allocate(bytes);
        chunks_ = ::new (memory) ChunkHeader{chunks_, bytes};
        bump_ = reinterpret_cast<char*>(chunks_) + sizeof(ChunkHeader);
        bump_left_ = blocks_per_chunk_;
        ++chunk_count_;
    }

    void release_chunks() noexcept {
        while (chunks_ != nullptr) {
            ChunkHeader* next = chunks_->next;
            const std::size_t bytes = chunks_->bytes;
            upstream_.deallocate(chunks_, bytes);
            chunks_ = next;
        }
        free_list_ = nullptr;
        bump_ = nullptr;
        bump_left_ = 0;
        chunk_count_ = 0;
        live_ = 0;
    }

    std::size_t block_size_;
    std::size_t blocks_per_chunk_;
    Upstream upstream_;
    FreeBlock* free_list_ = nullptr;
    ChunkHeader* chunks_ = nullptr;
    char* bump_ = nullptr;
    std::size_t bump_left_ = 0;
    std::size_t chunk_count_ = 0;
    std::size_t live_ = 0;
};

// One pool per block size, shared by every allocator of one container family.
class PoolRegistry {
public:
    void* allocate(std::size_t bytes) { return pool_for(bytes).allocate(); }
    void deallocate(void* block, std::size_t bytes) { pool_for(bytes).deallocate(block); }

private:
    FixedBlockPool<>& pool_for(std::size_t bytes) {
        const auto found = pools_.find(bytes);
        if (found != pools_.end()) {
            return found->second;
        }
        return pools_.emplace(bytes, FixedBlockPool<>(bytes, 4096)).first->second;
    }

    std::map<std::size_t, FixedBlockPool<>> pools_;
};

template <typename T>
class PoolAllocator {
public:
    using value_type = T;

    PoolAllocator() : registry_(std::make_shared<PoolRegistry>()) {}
    PoolAllocator(const PoolAllocator& other) noexcept = default;
    template <typename U>
    PoolAllocator(const PoolAllocator<U>& other) noexcept : registry_(other.registry_) {}

    T* allocate(std::size_t n) {
        if (n == 1) {
            return static_cast<T*>(registry_->allocate(sizeof(T)));
        }
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t n) noexcept {
        if (n == 1) {
            registry_->deallocate(p, sizeof(T));
        } else {
            ::operator delete(p);
        }
    }

    friend bool operator==(const PoolAllocator& a, const PoolAllocator& b) noexcept {
        return a.registry_ == b.registry_;
    }

private:
    template <typename U>
    friend class PoolAllocator;

    std::shared_ptr<PoolRegistry> registry_;
};
