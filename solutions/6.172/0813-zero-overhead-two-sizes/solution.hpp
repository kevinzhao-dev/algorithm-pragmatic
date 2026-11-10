#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <span>

class FixedAligned64 {
public:
    explicit FixedAligned64(std::span<std::byte> heap) : heap_(heap) {}

    void* allocate() {
        if (free_ != nullptr) {
            Node* node = free_;
            free_ = node->next;
            return node;
        }
        if (used_ + 64 > heap_.size()) {
            return nullptr;
        }
        void* block = heap_.data() + used_;
        used_ += 64;
        return block;
    }

    void deallocate(void* p) {
        if (p != nullptr) {
            free_ = ::new (p) Node{free_};
        }
    }

    std::size_t used_bytes() const { return used_; }

private:
    struct Node {
        Node* next;
    };

    std::span<std::byte> heap_;
    std::size_t used_ = 0;
    Node* free_ = nullptr;
};

class SmartPtr {
public:
    SmartPtr() = default;

    static SmartPtr make(void* address, bool is_small) {
        SmartPtr p;
        p.bits_ = reinterpret_cast<std::uintptr_t>(address) | (is_small ? 1 : 0);
        return p;
    }

    void* get() const { return reinterpret_cast<void*>(bits_ & ~std::uintptr_t{31}); }
    bool is_small() const { return (bits_ & 1) != 0; }
    explicit operator bool() const { return bits_ != 0; }

    friend bool operator==(const SmartPtr& a, const SmartPtr& b) { return a.bits_ == b.bits_; }

private:
    std::uintptr_t bits_ = 0;
};

class SmartAllocator {
public:
    explicit SmartAllocator(std::span<std::byte> heap) : heap_(heap) {}

    SmartPtr allocate(std::size_t bytes) {
        if (bytes == 0 || bytes > 64) {
            return {};
        }
        return bytes <= 32 ? allocate_small() : allocate_large();
    }

    void deallocate(SmartPtr p) {
        if (p) {
            (p.is_small() ? small_ : large_) = ::new (p.get()) Node{p.is_small() ? small_ : large_};
        }
    }

    std::size_t used_bytes() const { return used_; }

private:
    struct Node {
        Node* next;
    };

    SmartPtr allocate_small() {
        if (small_ != nullptr) {
            Node* node = small_;
            small_ = node->next;
            return SmartPtr::make(node, true);
        }
        if (used_ + 32 > heap_.size()) {
            return {};
        }
        void* block = heap_.data() + used_;
        used_ += 32;
        return SmartPtr::make(block, true);
    }

    SmartPtr allocate_large() {
        if (large_ != nullptr) {
            Node* node = large_;
            large_ = node->next;
            return SmartPtr::make(node, false);
        }
        std::size_t start = used_;
        if (start % 64 != 0) {
            start += 32;
        }
        if (start + 64 > heap_.size()) {
            return {};
        }
        if (start != used_) {
            small_ = ::new (heap_.data() + used_) Node{small_};  // the half line is not wasted
        }
        used_ = start + 64;
        return SmartPtr::make(heap_.data() + start, false);
    }

    std::span<std::byte> heap_;
    std::size_t used_ = 0;
    Node* small_ = nullptr;
    Node* large_ = nullptr;
};
