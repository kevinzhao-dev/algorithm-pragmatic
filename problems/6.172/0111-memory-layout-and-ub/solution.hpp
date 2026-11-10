#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "practice.hpp"

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

inline StructLayout compute_layout(std::span<const FieldSpec> fields) { PRACTICE_TODO(); }
inline std::size_t padding_bytes(std::span<const FieldSpec> fields) { PRACTICE_TODO(); }
inline std::vector<std::size_t> packed_order(std::span<const FieldSpec> fields) { PRACTICE_TODO(); }

// Stage 2
class BumpArena {
public:
    explicit BumpArena(std::span<std::byte> buffer) { PRACTICE_TODO(); }

    void* allocate(std::size_t bytes, std::size_t alignment) { PRACTICE_TODO(); }
    void reset() { PRACTICE_TODO(); }
    std::size_t used() const { PRACTICE_TODO(); }
    std::size_t capacity() const { PRACTICE_TODO(); }
};

// Stage 3
template <typename T, std::size_t N>
class StaticVector {
public:
    StaticVector() { PRACTICE_TODO(); }
    ~StaticVector() {}

    StaticVector(const StaticVector& other) { PRACTICE_TODO(); }
    StaticVector& operator=(const StaticVector& other) { PRACTICE_TODO(); }
    StaticVector(StaticVector&& other) { PRACTICE_TODO(); }
    StaticVector& operator=(StaticVector&& other) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    static constexpr std::size_t capacity() { return N; }
    bool empty() const { PRACTICE_TODO(); }

    T* data() { PRACTICE_TODO(); }
    const T* data() const { PRACTICE_TODO(); }
    T* begin() { PRACTICE_TODO(); }
    T* end() { PRACTICE_TODO(); }
    const T* begin() const { PRACTICE_TODO(); }
    const T* end() const { PRACTICE_TODO(); }

    T& operator[](std::size_t index) { PRACTICE_TODO(); }
    const T& operator[](std::size_t index) const { PRACTICE_TODO(); }

    T& push_back(const T& value) { PRACTICE_TODO(); }
    T& push_back(T&& value) { PRACTICE_TODO(); }
    template <typename... Args>
    T& emplace_back(Args&&... args) { PRACTICE_TODO(); }
    void pop_back() { PRACTICE_TODO(); }
    void clear() { PRACTICE_TODO(); }
};

// Stage 4
inline std::optional<int> checked_add(int a, int b) { PRACTICE_TODO(); }
inline std::optional<int> checked_mul(int a, int b) { PRACTICE_TODO(); }
inline int midpoint_floor(int a, int b) { PRACTICE_TODO(); }
inline std::uint32_t rotl32(std::uint32_t x, int k) { PRACTICE_TODO(); }
inline std::int32_t sign_extend(std::uint32_t value, int bits) { PRACTICE_TODO(); }
inline std::uint32_t load_le32(const std::byte* p) { PRACTICE_TODO(); }
inline void store_le32(std::byte* p, std::uint32_t value) { PRACTICE_TODO(); }
inline void no_temp_swap(int& a, int& b) { PRACTICE_TODO(); }
