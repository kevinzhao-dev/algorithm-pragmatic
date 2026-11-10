#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include "01_bench.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

// A packed byte array with deliberately bit-at-a-time operations: the speed baseline.
class SlowBits {
public:
    explicit SlowBits(std::size_t bits) : bytes_((bits + 7) / 8, 0) {}
    bool get(std::size_t i) const { return (bytes_[i >> 3] >> (i & 7)) & 1; }
    void set(std::size_t i, bool v) {
        auto& byte = bytes_[i >> 3];
        const auto mask = static_cast<std::uint8_t>(1U << (i & 7));
        byte = static_cast<std::uint8_t>((byte & ~mask) | (v ? mask : 0U));
    }

    void reverse(std::size_t begin, std::size_t end) {
        while (begin + 1 < end) {
            --end;
            const bool left = get(begin);
            const bool right = get(end);
            set(begin, right);
            set(end, left);
            ++begin;
        }
    }

    void rotate_right(std::size_t offset, std::size_t length, std::size_t amount) {
        reverse(offset, offset + length);
        reverse(offset, offset + amount);
        reverse(offset + amount, offset + length);
    }

    const std::uint8_t* data() const { return bytes_.data(); }

private:
    std::vector<std::uint8_t> bytes_;
};

std::string rotated_model(std::string text, std::size_t offset, std::size_t length,
                          std::ptrdiff_t amount) {
    if (length == 0) {
        return text;
    }
    const auto l = static_cast<std::ptrdiff_t>(length);
    std::ptrdiff_t k = amount % l;
    if (k < 0) {
        k += l;
    }
    std::string segment = text.substr(offset, length);
    for (std::size_t i = 0; i < length; ++i) {
        text[offset + (i + static_cast<std::size_t>(k)) % length] = segment[i];
    }
    return text;
}

std::string random_bits(Rng& rng, std::size_t n) {
    std::string text(n, '0');
    for (char& ch : text) {
        ch = rng.coin() ? '1' : '0';
    }
    return text;
}

}  // namespace

TEST(1, "a new array is all zeros with the right size") {
    const BitArray a(100);
    CHECK_EQ(a.size(), 100);
    for (std::size_t i = 0; i < 100; ++i) {
        CHECK(!a.get(i));
    }
    CHECK_EQ(BitArray(0).size(), 0);
}

TEST(1, "set and get round trip across word boundaries") {
    BitArray a(200);
    for (const std::size_t i : {0, 1, 7, 8, 63, 64, 65, 127, 128, 199}) {
        a.set(i, true);
        CHECK(a.get(i));
        a.set(i, false);
        CHECK(!a.get(i));
    }
}

TEST(1, "indexing past the end throws") {
    BitArray a(8);
    CHECK_THROWS_AS(a.get(8), std::out_of_range);
    CHECK_THROWS_AS(a.set(8, true), std::out_of_range);
    CHECK_THROWS_AS(BitArray(0).get(0), std::out_of_range);
}

TEST(1, "from_string and to_string are inverses") {
    const std::string text = "0110100110010110101";
    const BitArray a = BitArray::from_string(text);
    CHECK_EQ(a.size(), text.size());
    CHECK(a.get(1));
    CHECK(!a.get(0));
    CHECK_EQ(a.to_string(), text);
    CHECK_EQ(BitArray::from_string("").size(), 0);
    CHECK_EQ(BitArray::from_string("").to_string(), "");
}

TEST(1, "from_string rejects other characters") {
    CHECK_THROWS_AS(BitArray::from_string("0102"), std::invalid_argument);
    CHECK_THROWS_AS(BitArray::from_string("01 1"), std::invalid_argument);
}

TEST(1, "random single-bit updates match a model") {
    Rng rng;
    const std::size_t n = 2000;
    BitArray a(n);
    std::vector<char> model(n, 0);
    for (int step = 0; step < 50'000; ++step) {
        const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 1));
        if (rng.coin(0.7)) {
            const bool value = rng.coin();
            a.set(i, value);
            model[i] = value;
        }
        REQUIRE_EQ(a.get(i), model[i] != 0);
    }
}

TEST(2, "read_bits assembles a field that spans two words") {
    BitArray a(200);
    a.write_bits(60, 8, 0xFF);
    CHECK_EQ(a.read_bits(60, 8), 0xFFU);
    CHECK_EQ(a.read_bits(59, 10), 0b0111111110U);
    CHECK(!a.get(59));
    CHECK(a.get(60));
    CHECK(a.get(67));
    CHECK(!a.get(68));
}

TEST(2, "bit order is least significant first") {
    BitArray a = BitArray::from_string("10000000");
    CHECK_EQ(a.read_bits(0, 8), 1U);
    BitArray b(16);
    b.write_bits(3, 5, 0b00001);
    CHECK(b.get(3));
    CHECK(!b.get(4));
    CHECK_EQ(b.to_string(), "0001000000000000");
}

TEST(2, "a full 64-bit field at an unaligned offset round trips") {
    BitArray a(200);
    const std::uint64_t value = 0xFEDCBA9876543210ULL;
    a.write_bits(13, 64, value);
    CHECK_EQ(a.read_bits(13, 64), value);
    CHECK_EQ(a.read_bits(0, 13), 0U);
    CHECK_EQ(a.read_bits(77, 64), 0U);
}

TEST(2, "write_bits leaves neighbouring bits alone") {
    BitArray a(128);
    for (std::size_t i = 0; i < 128; ++i) {
        a.set(i, true);
    }
    a.write_bits(50, 20, 0);
    CHECK_EQ(a.read_bits(0, 50), (std::uint64_t{1} << 50) - 1);
    CHECK_EQ(a.read_bits(70, 58), (std::uint64_t{1} << 58) - 1);
    CHECK_EQ(a.read_bits(50, 20), 0U);
}

TEST(2, "write_bits ignores bits of value beyond count") {
    BitArray a(16);
    a.write_bits(4, 4, 0xFF);
    CHECK_EQ(a.read_bits(0, 16), 0xF0U);
}

TEST(2, "zero-width fields are harmless") {
    BitArray a(8);
    CHECK_EQ(a.read_bits(8, 0), 0U);
    a.write_bits(8, 0, 123);
    CHECK_EQ(a.to_string(), "00000000");
}

TEST(2, "fields past the end throw") {
    BitArray a(64);
    CHECK_THROWS_AS(a.read_bits(60, 5), std::out_of_range);
    CHECK_THROWS_AS(a.read_bits(65, 0), std::out_of_range);
    CHECK_THROWS_AS(a.read_bits(0, 65), std::out_of_range);
    CHECK_THROWS_AS(a.write_bits(60, 5, 1), std::out_of_range);
    CHECK_THROWS_AS(a.read_bits(std::numeric_limits<std::size_t>::max(), 64), std::out_of_range);
}

TEST(2, "random fields agree with a bitwise model") {
    Rng rng;
    const std::size_t n = 4096;
    BitArray a(n);
    std::string model(n, '0');
    for (int step = 0; step < 40'000; ++step) {
        const auto count = static_cast<unsigned>(rng.uniform(0, 64));
        const auto offset =
            static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n - count)));
        const auto value = static_cast<std::uint64_t>(rng.uniform(0, 0x7FFFFFFFFFFFFFFFLL)) * 2 +
                           static_cast<std::uint64_t>(rng.coin());
        INFO("step ", step, " offset ", offset, " count ", count);
        if (rng.coin()) {
            a.write_bits(offset, count, value);
            for (unsigned j = 0; j < count; ++j) {
                model[offset + j] = ((value >> j) & 1) != 0 ? '1' : '0';
            }
        }
        std::uint64_t expected = 0;
        for (unsigned j = 0; j < count; ++j) {
            if (model[offset + j] == '1') {
                expected |= std::uint64_t{1} << j;
            }
        }
        REQUIRE_EQ(a.read_bits(offset, count), expected);
    }
    CHECK_EQ(a.to_string(), model);
}

TEST(3, "right rotation moves bits toward higher indices") {
    BitArray a = BitArray::from_string("10000000");
    a.rotate(0, 8, 1);
    CHECK_EQ(a.to_string(), "01000000");
    a.rotate(0, 8, 6);
    CHECK_EQ(a.to_string(), "00000001");
    a.rotate(0, 8, 1);
    CHECK_EQ(a.to_string(), "10000000");
}

TEST(3, "negative amounts rotate toward lower indices") {
    BitArray a = BitArray::from_string("01000000");
    a.rotate(0, 8, -1);
    CHECK_EQ(a.to_string(), "10000000");
    a.rotate(0, 8, -1);
    CHECK_EQ(a.to_string(), "00000001");
}

TEST(3, "only the specified sub-range moves") {
    BitArray a = BitArray::from_string("11000111");
    a.rotate(2, 3, 1);  // middle "000" is unchanged by rotation of equal bits
    CHECK_EQ(a.to_string(), "11000111");
    BitArray b = BitArray::from_string("10110010");
    b.rotate(1, 5, 2);  // positions 1..5 hold 01100; right by 2 gives 00011
    CHECK_EQ(b.to_string(), "10001110");
}

TEST(3, "amounts larger than the length wrap around") {
    BitArray a = BitArray::from_string("10000000");
    a.rotate(0, 8, 8);
    CHECK_EQ(a.to_string(), "10000000");
    a.rotate(0, 8, 19);  // 19 mod 8 == 3
    CHECK_EQ(a.to_string(), "00010000");
    a.rotate(0, 8, -21);  // -21 mod 8 == 3 in the rightward sense
    CHECK_EQ(a.to_string(), "00000010");
}

TEST(3, "empty and single-bit ranges are no-ops") {
    BitArray a = BitArray::from_string("1010");
    a.rotate(1, 0, 5);
    a.rotate(1, 1, 3);
    a.rotate(4, 0, 1);
    CHECK_EQ(a.to_string(), "1010");
}

TEST(3, "the most negative amount does not overflow") {
    BitArray a = BitArray::from_string("10010110");
    a.rotate(0, 8, std::numeric_limits<std::ptrdiff_t>::min());
    // PTRDIFF_MIN mod 8 == 0 in the rightward sense
    CHECK_EQ(a.to_string(), "10010110");
    BitArray b = BitArray::from_string("1000000");
    b.rotate(0, 7, std::numeric_limits<std::ptrdiff_t>::min());
    CHECK_EQ(b.to_string(), rotated_model("1000000", 0, 7, std::numeric_limits<std::ptrdiff_t>::min()));
}

TEST(3, "invalid ranges throw") {
    BitArray a(8);
    CHECK_THROWS_AS(a.rotate(5, 4, 1), std::out_of_range);
    CHECK_THROWS_AS(a.rotate(9, 0, 1), std::out_of_range);
    CHECK_THROWS_AS(a.rotate(std::numeric_limits<std::size_t>::max(), 2, 1), std::out_of_range);
}

TEST(3, "exhaustive offsets lengths and amounts on a small array") {
    Rng rng;
    const std::size_t n = 22;
    const std::string text = random_bits(rng, n);
    for (std::size_t offset = 0; offset <= n; ++offset) {
        for (std::size_t length = 0; length <= n - offset; ++length) {
            for (std::ptrdiff_t amount = -30; amount <= 30; ++amount) {
                BitArray a = BitArray::from_string(text);
                a.rotate(offset, length, amount);
                INFO("offset ", offset, " length ", length, " amount ", amount);
                REQUIRE_EQ(a.to_string(), rotated_model(text, offset, length, amount));
            }
        }
    }
}

TEST(3, "random rotations across word boundaries match the model") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 3000));
        std::string text = random_bits(rng, n);
        BitArray a = BitArray::from_string(text);
        for (int step = 0; step < 6; ++step) {
            const auto length = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n)));
            const auto offset =
                static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n - length)));
            const std::ptrdiff_t amount = rng.uniform(-5000, 5000);
            a.rotate(offset, length, amount);
            text = rotated_model(std::move(text), offset, length, amount);
            INFO("round ", round, " step ", step);
            REQUIRE_EQ(a.to_string(), text);
        }
    }
}

PERF(4, "word-level rotation beats bit-by-bit by a wide margin", 8000) {
    const std::size_t n = 1 << 24;
    Rng rng;
    SlowBits slow(n);
    BitArray fast(n);
    for (std::size_t i = 0; i < n; i += static_cast<std::size_t>(rng.uniform(1, 3))) {
        slow.set(i, true);
        fast.set(i, true);
    }
    const std::size_t offset = 13;
    const std::size_t length = n - 100;
    const std::size_t amount = 7'777'777;
    const double ratio = bench::speedup(
        5, [&] {
            slow.rotate_right(offset, length, amount);
            bench::do_not_optimize(slow.data());
        },
        [&] {
            fast.rotate(offset, length, static_cast<std::ptrdiff_t>(amount));
            bench::do_not_optimize(fast);
        });
    INFO("bitwise/word-level speedup ", ratio);
    CHECK_GE(ratio, 16.0);
}

PERF(4, "many large rotations stay near-linear", 3000) {
    Rng rng;
    const std::size_t n = 1 << 25;
    BitArray a(n);
    for (int round = 0; round < 100; ++round) {
        const auto length = static_cast<std::size_t>(rng.uniform(n / 2, n - 1));
        const auto offset =
            static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n - length)));
        a.rotate(offset, length, rng.uniform(-100'000'000, 100'000'000));
    }
    CHECK_EQ(a.size(), n);
}

TEST(5, "invalid widths and values are rejected") {
    CHECK_THROWS_AS(BitPackedVector(0, 4), std::invalid_argument);
    CHECK_THROWS_AS(BitPackedVector(65, 4), std::invalid_argument);
    BitPackedVector v(4, 2);
    CHECK_THROWS_AS(v.push_back(16), std::invalid_argument);
    v.push_back(15);
    CHECK_THROWS_AS(v.set(0, 99), std::invalid_argument);
    CHECK_EQ(v.get(0), 15U);
}

TEST(5, "push_back stores values and respects capacity") {
    BitPackedVector v(5, 3);
    CHECK_EQ(v.capacity(), 3);
    CHECK_EQ(v.width(), 5);
    v.push_back(1);
    v.push_back(31);
    v.push_back(16);
    CHECK_EQ(v.size(), 3);
    CHECK_EQ(v.get(0), 1U);
    CHECK_EQ(v.get(1), 31U);
    CHECK_EQ(v.get(2), 16U);
    CHECK_THROWS_AS(v.push_back(0), std::length_error);
    CHECK_THROWS_AS(v.get(3), std::out_of_range);
}

TEST(5, "insert shifts later elements") {
    BitPackedVector v(5, 8);
    for (std::uint64_t x : {10, 20, 30}) {
        v.push_back(x);
    }
    v.insert(1, 7);
    CHECK_EQ(v.size(), 4);
    CHECK_EQ(v.get(0), 10U);
    CHECK_EQ(v.get(1), 7U);
    CHECK_EQ(v.get(2), 20U);
    CHECK_EQ(v.get(3), 30U);
    v.insert(0, 1);
    CHECK_EQ(v.get(0), 1U);
    v.insert(v.size(), 2);
    CHECK_EQ(v.get(v.size() - 1), 2U);
    CHECK_THROWS_AS(v.insert(100, 1), std::out_of_range);
}

TEST(5, "erase closes the gap") {
    BitPackedVector v(5, 8);
    for (std::uint64_t x : {1, 2, 3, 4}) {
        v.push_back(x);
    }
    v.erase(1);
    CHECK_EQ(v.size(), 3);
    CHECK_EQ(v.get(0), 1U);
    CHECK_EQ(v.get(1), 3U);
    CHECK_EQ(v.get(2), 4U);
    v.erase(2);
    v.erase(0);
    CHECK_EQ(v.size(), 1);
    CHECK_EQ(v.get(0), 3U);
    CHECK_THROWS_AS(v.erase(5), std::out_of_range);
}

TEST(5, "the full 64-bit width works") {
    BitPackedVector v(64, 4);
    v.push_back(~std::uint64_t{0});
    v.insert(0, 0x123456789ABCDEF0ULL);
    v.push_back(0);
    CHECK_EQ(v.get(0), 0x123456789ABCDEF0ULL);
    CHECK_EQ(v.get(1), ~std::uint64_t{0});
    v.erase(1);
    CHECK_EQ(v.get(1), 0U);
}

TEST(5, "a column full to capacity rejects inserts") {
    BitPackedVector v(3, 2);
    v.push_back(1);
    v.push_back(2);
    CHECK_THROWS_AS(v.insert(0, 3), std::length_error);
    v.erase(0);
    v.insert(0, 3);
    CHECK_EQ(v.get(0), 3U);
    CHECK_EQ(v.get(1), 2U);
}

TEST(5, "random operations match std::vector for many widths") {
    Rng rng;
    for (const unsigned width : {1U, 3U, 7U, 8U, 13U, 31U, 63U, 64U}) {
        const std::size_t capacity = 600;
        BitPackedVector column(width, capacity);
        std::vector<std::uint64_t> model;
        const auto random_value = [&] {
            const auto wide = (static_cast<std::uint64_t>(rng.uniform(0, 0xFFFFFFFFLL)) << 32) |
                              static_cast<std::uint64_t>(rng.uniform(0, 0xFFFFFFFFLL));
            return width == 64 ? wide : wide & ((std::uint64_t{1} << width) - 1);
        };
        for (int step = 0; step < 5000; ++step) {
            INFO("width ", width, " step ", step, " size ", model.size());
            const auto op = rng.uniform(0, 3);
            if (op == 0 && model.size() < capacity) {
                const std::uint64_t value = random_value();
                column.push_back(value);
                model.push_back(value);
            } else if (op == 1 && model.size() < capacity) {
                const auto at = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size())));
                const std::uint64_t value = random_value();
                column.insert(at, value);
                model.insert(model.begin() + static_cast<std::ptrdiff_t>(at), value);
            } else if (op == 2 && !model.empty()) {
                const auto at = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
                column.erase(at);
                model.erase(model.begin() + static_cast<std::ptrdiff_t>(at));
            } else if (!model.empty()) {
                const auto at = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
                const std::uint64_t value = random_value();
                column.set(at, value);
                model[at] = value;
            }
            REQUIRE_EQ(column.size(), model.size());
            if (!model.empty()) {
                const auto at = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
                REQUIRE_EQ(column.get(at), model[at]);
            }
        }
    }
}

PERF(5, "front inserts move data at word level", 4000) {
    const std::size_t count = 2000;
    const std::size_t base = 400'000;
    BitPackedVector column(13, base + count);
    for (std::size_t i = 0; i < base; ++i) {
        column.push_back(i & 0x1FFF);
    }
    for (std::size_t i = 0; i < count; ++i) {
        column.insert(0, i & 0x1FFF);
    }
    CHECK_EQ(column.size(), base + count);
    CHECK_EQ(column.get(count + 5), 5U);
}
