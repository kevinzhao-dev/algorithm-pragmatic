// sha256.hpp — a small SHA-256 implementation (FIPS 180-4) for the Merkle tree exercise.
// sha256::hash(data) returns the digest; sha256::calls() counts invocations so tests can check
// how many hashes an operation performs.
#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace sha256 {

using Digest = std::array<unsigned char, 32>;

inline std::int64_t& calls() {
    static std::int64_t count = 0;
    return count;
}

namespace detail {

constexpr std::uint32_t kRound[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
    0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
    0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
    0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
    0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
    0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
    0xc67178f2};

constexpr std::uint32_t rotr(std::uint32_t x, int k) { return (x >> k) | (x << (32 - k)); }

inline void compress(std::array<std::uint32_t, 8>& state, const unsigned char* block) {
    std::array<std::uint32_t, 64> w{};
    for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<std::uint32_t>(block[4 * i]) << 24) |
               (static_cast<std::uint32_t>(block[4 * i + 1]) << 16) |
               (static_cast<std::uint32_t>(block[4 * i + 2]) << 8) |
               static_cast<std::uint32_t>(block[4 * i + 3]);
    }
    for (int i = 16; i < 64; ++i) {
        const std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    auto [a, b, c, d, e, f, g, h] = state;
    for (int i = 0; i < 64; ++i) {
        const std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        const std::uint32_t ch = (e & f) ^ (~e & g);
        const std::uint32_t t1 = h + s1 + ch + kRound[i] + w[i];
        const std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t t2 = s0 + maj;
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

}  // namespace detail

inline Digest hash(std::string_view message) {
    ++calls();
    std::array<std::uint32_t, 8> state = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                          0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    const auto* data = reinterpret_cast<const unsigned char*>(message.data());
    std::size_t remaining = message.size();
    while (remaining >= 64) {
        detail::compress(state, data);
        data += 64;
        remaining -= 64;
    }
    std::array<unsigned char, 128> tail{};
    std::memcpy(tail.data(), data, remaining);
    tail[remaining] = 0x80;
    const std::size_t pad_end = remaining < 56 ? 64 : 128;
    const std::uint64_t bit_length = static_cast<std::uint64_t>(message.size()) * 8;
    for (int i = 0; i < 8; ++i) {
        tail[pad_end - 1 - i] = static_cast<unsigned char>(bit_length >> (8 * i));
    }
    detail::compress(state, tail.data());
    if (pad_end == 128) {
        detail::compress(state, tail.data() + 64);
    }
    Digest digest{};
    for (int i = 0; i < 8; ++i) {
        digest[4 * i] = static_cast<unsigned char>(state[i] >> 24);
        digest[4 * i + 1] = static_cast<unsigned char>(state[i] >> 16);
        digest[4 * i + 2] = static_cast<unsigned char>(state[i] >> 8);
        digest[4 * i + 3] = static_cast<unsigned char>(state[i]);
    }
    return digest;
}

}  // namespace sha256
