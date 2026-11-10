#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// Stage 1
inline std::uint64_t set_bit(std::uint64_t x, unsigned k) { return x | (std::uint64_t{1} << k); }
inline std::uint64_t clear_bit(std::uint64_t x, unsigned k) { return x & ~(std::uint64_t{1} << k); }
inline std::uint64_t toggle_bit(std::uint64_t x, unsigned k) { return x ^ (std::uint64_t{1} << k); }

namespace detail_0301 {
inline std::uint64_t low_mask(unsigned len) {
    return len >= 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << len) - 1;
}
}  // namespace detail_0301

inline std::uint64_t extract_field(std::uint64_t x, unsigned lo, unsigned len) {
    if (len == 0) {
        return 0;
    }
    return (x >> lo) & detail_0301::low_mask(len);
}

inline std::uint64_t insert_field(std::uint64_t x, unsigned lo, unsigned len, std::uint64_t value) {
    if (len == 0) {
        return x;
    }
    const std::uint64_t mask = detail_0301::low_mask(len) << lo;
    return (x & ~mask) | ((value << lo) & mask);
}

// Stage 2
inline std::int64_t min_no_branch(std::int64_t x, std::int64_t y) {
    return y ^ ((x ^ y) & -static_cast<std::int64_t>(x < y));
}

inline std::int64_t abs_no_branch(std::int64_t x) {
    const std::int64_t mask = x >> 63;
    return (x ^ mask) - mask;
}

inline std::uint64_t mod_add_no_branch(std::uint64_t a, std::uint64_t b, std::uint64_t n) {
    const std::uint64_t sum = a + b;
    return sum - (n & -static_cast<std::uint64_t>(sum >= n));
}

inline void merge_no_branch(const int* a, std::size_t na, const int* b, std::size_t nb, int* out) {
    while (na > 0 && nb > 0) {
        const auto take_a = static_cast<std::size_t>(*a <= *b);
        const int value = *b ^ ((*b ^ *a) & -static_cast<int>(take_a));
        *out++ = value;
        a += take_a;
        na -= take_a;
        b += take_a ^ 1U;
        nb -= take_a ^ 1U;
    }
    while (na > 0) {
        *out++ = *a++;
        --na;
    }
    while (nb > 0) {
        *out++ = *b++;
        --nb;
    }
}

// Stage 3
inline std::uint64_t ceil_pow2(std::uint64_t x) {
    if (x == 0) {
        return 1;
    }
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}

inline std::uint64_t lowest_set_bit(std::uint64_t x) { return x & -x; }

inline int count_trailing_zeros(std::uint64_t x) {
    // Multiplying by a de Bruijn constant maps the isolated bit's position to a unique top-6-bit
    // pattern, which indexes this permutation table.
    constexpr std::uint64_t kDeBruijn = 0x03F79D71B4CB0A89ULL;
    constexpr std::array<int, 64> kIndex = {
        0,  47, 1,  56, 48, 27, 2,  60, 57, 49, 41, 37, 28, 16, 3,  61,
        54, 58, 35, 52, 50, 42, 21, 44, 38, 32, 29, 23, 17, 11, 4,  62,
        46, 55, 26, 59, 40, 36, 15, 53, 34, 51, 20, 43, 31, 22, 10, 45,
        25, 39, 14, 33, 19, 30, 9,  24, 13, 18, 8,  12, 7,  6,  5,  63,
    };
    return kIndex[((x ^ (x - 1)) * kDeBruijn) >> 58];
}

inline int popcount_table(std::uint64_t x) {
    static constexpr std::array<unsigned char, 256> kCounts = [] {
        std::array<unsigned char, 256> counts{};
        for (std::size_t value = 1; value < counts.size(); ++value) {
            counts[value] = static_cast<unsigned char>(counts[value >> 1] + (value & 1));
        }
        return counts;
    }();
    int total = 0;
    for (; x != 0; x >>= 8) {
        total += kCounts[x & 0xFF];
    }
    return total;
}

inline int popcount_parallel(std::uint64_t x) {
    constexpr std::uint64_t m1 = 0x5555555555555555ULL;
    constexpr std::uint64_t m2 = 0x3333333333333333ULL;
    constexpr std::uint64_t m4 = 0x0F0F0F0F0F0F0F0FULL;
    x = x - ((x >> 1) & m1);
    x = (x & m2) + ((x >> 2) & m2);
    x = (x + (x >> 4)) & m4;
    return static_cast<int>((x * 0x0101010101010101ULL) >> 56);
}

inline std::uint64_t reverse_bits(std::uint64_t x) {
    x = ((x >> 1) & 0x5555555555555555ULL) | ((x & 0x5555555555555555ULL) << 1);
    x = ((x >> 2) & 0x3333333333333333ULL) | ((x & 0x3333333333333333ULL) << 2);
    x = ((x >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((x & 0x0F0F0F0F0F0F0F0FULL) << 4);
    x = ((x >> 8) & 0x00FF00FF00FF00FFULL) | ((x & 0x00FF00FF00FF00FFULL) << 8);
    x = ((x >> 16) & 0x0000FFFF0000FFFFULL) | ((x & 0x0000FFFF0000FFFFULL) << 16);
    return (x >> 32) | (x << 32);
}

// Stage 4
class SetBits {
public:
    struct Iterator {
        std::uint64_t word;
        unsigned operator*() const { return static_cast<unsigned>(count_trailing_zeros(word)); }
        Iterator& operator++() {
            word &= word - 1;
            return *this;
        }
        friend bool operator==(const Iterator& a, const Iterator& b) { return a.word == b.word; }
    };

    explicit SetBits(std::uint64_t word) : word_(word) {}

    Iterator begin() const { return Iterator{word_}; }
    Iterator end() const { return Iterator{0}; }

private:
    std::uint64_t word_;
};

inline std::uint64_t next_with_same_popcount(std::uint64_t x) {
    if (x == 0) {
        return 0;
    }
    const std::uint64_t low = x & -x;
    const std::uint64_t ripple = x + low;
    if (ripple == 0) {
        return 0;  // the block of ones already reaches the top bit
    }
    return (((x ^ ripple) >> 2) / low) | ripple;
}
