#pragma once

#include <arm_neon.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace detail_0412 {

// Compresses a byte mask (each lane 0x00 or 0xFF) into 4 bits per byte, in lane order.
inline std::uint64_t mask_bits(uint8x16_t mask) {
    return vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(mask), 4)), 0);
}

}  // namespace detail_0412

// Stage 1
[[gnu::noinline]] inline std::size_t find_byte(std::span<const std::uint8_t> haystack,
                                               std::uint8_t needle) {
    const std::uint8_t* const data = haystack.data();
    const std::size_t n = haystack.size();
    std::size_t i = 0;
    const uint8x16_t target = vdupq_n_u8(needle);
    for (; i + 16 <= n; i += 16) {
        const uint8x16_t eq = vceqq_u8(vld1q_u8(data + i), target);
        const std::uint64_t bits = detail_0412::mask_bits(eq);
        if (bits != 0) {
            return i + static_cast<std::size_t>(std::countr_zero(bits) / 4);
        }
    }
    for (; i < n; ++i) {
        if (data[i] == needle) {
            return i;
        }
    }
    return n;
}

// Stage 2
[[gnu::noinline]] inline std::size_t count_byte(std::span<const std::uint8_t> haystack,
                                                std::uint8_t needle) {
    const std::uint8_t* const data = haystack.data();
    const std::size_t n = haystack.size();
    std::size_t i = 0;
    std::size_t total = 0;
    const uint8x16_t target = vdupq_n_u8(needle);
    for (; i + 16 <= n; i += 16) {
        const uint8x16_t eq = vceqq_u8(vld1q_u8(data + i), target);
        total += vaddvq_u8(vshrq_n_u8(eq, 7));
    }
    for (; i < n; ++i) {
        total += data[i] == needle ? 1 : 0;
    }
    return total;
}

// Stage 3
[[gnu::noinline]] inline void to_lower_ascii(std::span<char> text) {
    auto* const data = reinterpret_cast<std::uint8_t*>(text.data());
    const std::size_t n = text.size();
    std::size_t i = 0;
    const uint8x16_t low = vdupq_n_u8('A');
    const uint8x16_t high = vdupq_n_u8('Z');
    const uint8x16_t delta = vdupq_n_u8(0x20);
    for (; i + 16 <= n; i += 16) {
        const uint8x16_t chars = vld1q_u8(data + i);
        const uint8x16_t is_upper = vandq_u8(vcgeq_u8(chars, low), vcleq_u8(chars, high));
        vst1q_u8(data + i, vorrq_u8(chars, vandq_u8(is_upper, delta)));
    }
    for (; i < n; ++i) {
        if (data[i] >= 'A' && data[i] <= 'Z') {
            data[i] = static_cast<std::uint8_t>(data[i] + 0x20);
        }
    }
}

// Stage 4
[[gnu::noinline]] inline std::size_t find_any_of3(std::span<const std::uint8_t> haystack,
                                                  std::uint8_t a, std::uint8_t b, std::uint8_t c) {
    const std::uint8_t* const data = haystack.data();
    const std::size_t n = haystack.size();
    std::size_t i = 0;
    const uint8x16_t va = vdupq_n_u8(a);
    const uint8x16_t vb = vdupq_n_u8(b);
    const uint8x16_t vc = vdupq_n_u8(c);
    for (; i + 16 <= n; i += 16) {
        const uint8x16_t chars = vld1q_u8(data + i);
        const uint8x16_t any =
            vorrq_u8(vorrq_u8(vceqq_u8(chars, va), vceqq_u8(chars, vb)), vceqq_u8(chars, vc));
        const std::uint64_t bits = detail_0412::mask_bits(any);
        if (bits != 0) {
            return i + static_cast<std::size_t>(std::countr_zero(bits) / 4);
        }
    }
    for (; i < n; ++i) {
        if (data[i] == a || data[i] == b || data[i] == c) {
            return i;
        }
    }
    return n;
}

// Stage 5
inline std::vector<std::string_view> split_fields(std::string_view line, char separator) {
    std::vector<std::string_view> fields;
    std::size_t begin = 0;
    for (;;) {
        const std::size_t relative = find_byte(
            std::span(reinterpret_cast<const std::uint8_t*>(line.data() + begin), line.size() - begin),
            static_cast<std::uint8_t>(separator));
        if (begin + relative >= line.size()) {
            fields.push_back(line.substr(begin));
            return fields;
        }
        fields.push_back(line.substr(begin, relative));
        begin += relative + 1;
    }
}
