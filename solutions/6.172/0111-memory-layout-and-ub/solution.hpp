#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <numeric>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

// Stage 1
struct FieldSpec {
    std::size_t size;
    std::size_t align;
};

struct StructLayout {
    std::vector<std::size_t> offsets;
    std::size_t size = 0;
    std::size_t align = 1;
};

namespace detail_0111 {

inline std::size_t round_up(std::size_t value, std::size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

}  // namespace detail_0111

inline StructLayout compute_layout(std::span<const FieldSpec> fields) {
    StructLayout layout;
    layout.offsets.reserve(fields.size());
    std::size_t cursor = 0;
    for (const FieldSpec& field : fields) {
        if (field.align == 0 || !std::has_single_bit(field.align) ||
            field.size % field.align != 0) {
            throw std::invalid_argument("field size must be a multiple of its alignment");
        }
        cursor = detail_0111::round_up(cursor, field.align);
        layout.offsets.push_back(cursor);
        cursor += field.size;
        layout.align = std::max(layout.align, field.align);
    }
    if (fields.empty()) {
        layout.size = 1;
        layout.align = 1;
        return layout;
    }
    layout.size = detail_0111::round_up(cursor, layout.align);
    return layout;
}

inline std::size_t padding_bytes(std::span<const FieldSpec> fields) {
    const StructLayout layout = compute_layout(fields);
    std::size_t sum = 0;
    for (const FieldSpec& field : fields) {
        sum += field.size;
    }
    return layout.size - sum;
}

inline std::vector<std::size_t> packed_order(std::span<const FieldSpec> fields) {
    std::vector<std::size_t> order(fields.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    // Alignments are powers of two, so larger-aligned fields leave cursors that already suit
    // every smaller alignment: sorting by alignment descending removes all interior padding.
    std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        return fields[a].align > fields[b].align;
    });
    return order;
}

// Stage 2
class BumpArena {
public:
    explicit BumpArena(std::span<std::byte> buffer) : buffer_(buffer) {}

    void* allocate(std::size_t bytes, std::size_t alignment) {
        if (alignment == 0 || !std::has_single_bit(alignment)) {
            throw std::invalid_argument("alignment must be a power of two");
        }
        // Stay in integers: forming a pointer past the buffer would itself be UB.
        const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(buffer_.data());
        const std::uintptr_t cursor = base + used_;
        const std::uintptr_t aligned = (cursor + (alignment - 1)) & ~std::uintptr_t{alignment - 1};
        const std::size_t padding = static_cast<std::size_t>(aligned - cursor);
        const std::size_t remaining = buffer_.size() - used_;
        if (padding > remaining || bytes > remaining - padding) {
            return nullptr;
        }
        std::byte* const result = buffer_.data() + used_ + padding;
        used_ += padding + bytes;
        return result;
    }

    void reset() { used_ = 0; }
    std::size_t used() const { return used_; }
    std::size_t capacity() const { return buffer_.size(); }

private:
    std::span<std::byte> buffer_;
    std::size_t used_ = 0;
};

// Stage 3
template <typename T, std::size_t N>
class StaticVector {
public:
    StaticVector() = default;
    ~StaticVector() { clear(); }

    StaticVector(const StaticVector& other) {
        // A constructor that exits by exception never runs its own destructor, so the guard
        // destroys whatever was built before the throw.
        Guard guard{this};
        for (const T& value : other) {
            emplace_back(value);
        }
        guard.owner = nullptr;
    }

    StaticVector& operator=(const StaticVector& other) {
        if (this != &other) {
            StaticVector copy(other);  // strong guarantee: build first, then commit
            *this = std::move(copy);
        }
        return *this;
    }

    StaticVector(StaticVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        Guard guard{this};
        for (T& value : other) {
            emplace_back(std::move(value));
        }
        guard.owner = nullptr;
        other.clear();
    }

    StaticVector& operator=(StaticVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if (this != &other) {
            clear();
            for (T& value : other) {
                emplace_back(std::move(value));
            }
            other.clear();
        }
        return *this;
    }

    std::size_t size() const { return size_; }
    static constexpr std::size_t capacity() { return N; }
    bool empty() const { return size_ == 0; }

    T* data() { return ptr(0); }
    const T* data() const { return ptr(0); }
    T* begin() { return ptr(0); }
    T* end() { return ptr(size_); }
    const T* begin() const { return ptr(0); }
    const T* end() const { return ptr(size_); }

    T& operator[](std::size_t index) {
        if (index >= size_) {
            throw std::out_of_range("StaticVector index out of range");
        }
        return *ptr(index);
    }
    const T& operator[](std::size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("StaticVector index out of range");
        }
        return *ptr(index);
    }

    T& push_back(const T& value) { return emplace_back(value); }
    T& push_back(T&& value) { return emplace_back(std::move(value)); }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (size_ == N) {
            throw std::length_error("StaticVector is full");
        }
        T* const slot = std::construct_at(raw(size_), std::forward<Args>(args)...);
        ++size_;  // only after the constructor returned
        return *slot;
    }

    void pop_back() {
        if (size_ == 0) {
            throw std::out_of_range("pop_back on empty StaticVector");
        }
        --size_;
        std::destroy_at(ptr(size_));
    }

    void clear() {
        while (size_ > 0) {
            --size_;
            std::destroy_at(ptr(size_));
        }
    }

private:
    struct Guard {
        StaticVector* owner;
        ~Guard() {
            if (owner != nullptr) {
                owner->clear();
            }
        }
    };

    T* raw(std::size_t index) { return reinterpret_cast<T*>(storage_ + index * sizeof(T)); }
    T* ptr(std::size_t index) { return std::launder(raw(index)); }
    const T* ptr(std::size_t index) const {
        return std::launder(reinterpret_cast<const T*>(storage_ + index * sizeof(T)));
    }

    alignas(T) std::byte storage_[sizeof(T) * N];
    std::size_t size_ = 0;
};

// Stage 4
inline std::optional<int> checked_add(int a, int b) {
    int result = 0;
    if (__builtin_add_overflow(a, b, &result)) {
        return std::nullopt;
    }
    return result;
}

inline std::optional<int> checked_mul(int a, int b) {
    int result = 0;
    if (__builtin_mul_overflow(a, b, &result)) {
        return std::nullopt;
    }
    return result;
}

inline int midpoint_floor(int a, int b) {
    // Common bits plus half the differing bits; arithmetic shift floors in two's complement.
    return (a & b) + ((a ^ b) >> 1);
}

inline std::uint32_t rotl32(std::uint32_t x, int k) {
    const unsigned amount = static_cast<unsigned>(k) & 31U;
    return (x << amount) | (x >> ((32U - amount) & 31U));
}

inline std::int32_t sign_extend(std::uint32_t value, int bits) {
    const std::uint32_t mask = bits == 32 ? 0xFFFFFFFFU : (std::uint32_t{1} << bits) - 1U;
    const std::uint32_t sign = std::uint32_t{1} << (bits - 1);
    const std::uint32_t low = value & mask;
    return static_cast<std::int32_t>((low ^ sign) - sign);
}

inline std::uint32_t load_le32(const std::byte* p) {
    return std::to_integer<std::uint32_t>(p[0]) | (std::to_integer<std::uint32_t>(p[1]) << 8) |
           (std::to_integer<std::uint32_t>(p[2]) << 16) |
           (std::to_integer<std::uint32_t>(p[3]) << 24);
}

inline void store_le32(std::byte* p, std::uint32_t value) {
    p[0] = static_cast<std::byte>(value & 0xFFU);
    p[1] = static_cast<std::byte>((value >> 8) & 0xFFU);
    p[2] = static_cast<std::byte>((value >> 16) & 0xFFU);
    p[3] = static_cast<std::byte>((value >> 24) & 0xFFU);
}

inline void no_temp_swap(int& a, int& b) {
    if (&a == &b) {
        return;
    }
    a ^= b;
    b ^= a;
    a ^= b;
}
