#include <algorithm>
#include <cctype>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "01_bench.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

#define FORCE_SCALAR _Pragma("clang loop vectorize(disable) interleave(disable)")

namespace {

[[gnu::noinline]] std::size_t scalar_find(const std::uint8_t* p, std::size_t n, std::uint8_t x) {
    FORCE_SCALAR for (std::size_t i = 0; i < n; ++i) {
        if (p[i] == x) {
            return i;
        }
    }
    return n;
}

[[gnu::noinline]] std::size_t scalar_count(const std::uint8_t* p, std::size_t n, std::uint8_t x) {
    std::size_t total = 0;
    FORCE_SCALAR for (std::size_t i = 0; i < n; ++i) {
        total += p[i] == x ? 1 : 0;
    }
    return total;
}

[[gnu::noinline]] void scalar_lower(char* p, std::size_t n) {
    FORCE_SCALAR for (std::size_t i = 0; i < n; ++i) {
        if (p[i] >= 'A' && p[i] <= 'Z') {
            p[i] = static_cast<char>(p[i] + 32);
        }
    }
}

[[gnu::noinline]] std::size_t scalar_find3(const std::uint8_t* p, std::size_t n, std::uint8_t a,
                                           std::uint8_t b, std::uint8_t c) {
    FORCE_SCALAR for (std::size_t i = 0; i < n; ++i) {
        if (p[i] == a || p[i] == b || p[i] == c) {
            return i;
        }
    }
    return n;
}

std::vector<std::uint8_t> random_buffer(Rng& rng, std::size_t n, int low, int high) {
    std::vector<std::uint8_t> buffer(n);
    for (auto& byte : buffer) {
        byte = static_cast<std::uint8_t>(rng.uniform(low, high));
    }
    return buffer;
}

}  // namespace

TEST(1, "find_byte finds matches at every position in short buffers") {
    for (std::size_t n = 0; n <= 64; ++n) {
        for (std::size_t at = 0; at < n; ++at) {
            std::vector<std::uint8_t> buffer(n, 0x41);
            buffer[at] = 0x7A;
            INFO("n ", n, " at ", at);
            REQUIRE_EQ(find_byte(buffer, 0x7A), at);
        }
    }
}

TEST(1, "find_byte returns the size when absent and handles empty input") {
    std::vector<std::uint8_t> buffer(100, 0x20);
    CHECK_EQ(find_byte(buffer, 0x7A), 100);
    CHECK_EQ(find_byte(std::span<const std::uint8_t>(), 0x7A), 0);
    CHECK_EQ(find_byte(buffer, 0x20), 0);
}

TEST(1, "find_byte returns the first of several matches") {
    std::vector<std::uint8_t> buffer(50, 0);
    buffer[20] = 0x55;
    buffer[21] = 0x55;
    buffer[40] = 0x55;
    CHECK_EQ(find_byte(buffer, 0x55), 20);
}

TEST(1, "find_byte matches a model on random buffers of exact allocation size") {
    Rng rng;
    for (int round = 0; round < 2000; ++round) {
        // Exactly sized allocation: reading past the end is a heap-buffer-overflow.
        const auto n = static_cast<std::size_t>(rng.uniform(0, 200));
        const std::vector<std::uint8_t> buffer = random_buffer(rng, n, 0, 7);
        const auto needle = static_cast<std::uint8_t>(rng.uniform(0, 8));
        const auto found = std::find(buffer.begin(), buffer.end(), needle);
        INFO("round ", round);
        REQUIRE_EQ(find_byte(buffer, needle), static_cast<std::size_t>(found - buffer.begin()));
    }
}

PERF(1, "find_byte beats a scalar scan on long misses", 5000) {
    const std::size_t n = 1 << 20;
    std::vector<std::uint8_t> buffer(n, 0x20);
    const double ratio = bench::speedup(
        5,
        [&] { for (int r = 0; r < 300; ++r) bench::do_not_optimize(scalar_find(buffer.data(), n, 0x7A)); },
        [&] { for (int r = 0; r < 300; ++r) bench::do_not_optimize(find_byte(buffer, 0x7A)); });
    INFO("speedup ", ratio);
    CHECK_GE(ratio, 4.0);
}

TEST(2, "count_byte counts matches, including the tail") {
    std::vector<std::uint8_t> buffer(40, 1);
    CHECK_EQ(count_byte(buffer, 1), 40);
    CHECK_EQ(count_byte(buffer, 2), 0);
    buffer[39] = 2;
    buffer[0] = 2;
    CHECK_EQ(count_byte(buffer, 2), 2);
    CHECK_EQ(count_byte(std::span<const std::uint8_t>(), 2), 0);
}

TEST(2, "count_byte matches a model on random buffers") {
    Rng rng;
    for (int round = 0; round < 2000; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(0, 300));
        const std::vector<std::uint8_t> buffer = random_buffer(rng, n, 0, 5);
        const auto needle = static_cast<std::uint8_t>(rng.uniform(0, 5));
        const auto expected = std::count(buffer.begin(), buffer.end(), needle);
        INFO("round ", round);
        REQUIRE_EQ(count_byte(buffer, needle), static_cast<std::size_t>(expected));
    }
}

PERF(2, "count_byte beats a scalar loop", 5000) {
    const std::size_t n = 1 << 20;
    Rng rng;
    const std::vector<std::uint8_t> buffer = random_buffer(rng, n, 0, 3);
    const double ratio = bench::speedup(
        5,
        [&] { for (int r = 0; r < 300; ++r) bench::do_not_optimize(scalar_count(buffer.data(), n, 2)); },
        [&] { for (int r = 0; r < 300; ++r) bench::do_not_optimize(count_byte(buffer, 2)); });
    INFO("speedup ", ratio);
    CHECK_GE(ratio, 4.0);
}

TEST(3, "to_lower_ascii changes only uppercase ASCII") {
    std::string text = "Hello, WORLD! 123 [\\]^_`{|}~";
    std::string expected = "hello, world! 123 [\\]^_`{|}~";
    to_lower_ascii(text);
    CHECK_EQ(text, expected);
}

TEST(3, "bytes around the ASCII letter range are untouched") {
    std::string text;
    for (int c = 0; c < 256; ++c) {
        text.push_back(static_cast<char>(c));
    }
    std::string expected = text;
    for (char& c : expected) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    to_lower_ascii(text);
    CHECK_EQ(text, expected);
}

TEST(3, "to_lower_ascii matches a model across all short lengths") {
    Rng rng;
    for (std::size_t n = 0; n <= 80; ++n) {
        for (int round = 0; round < 20; ++round) {
            std::string text = rng.text(n, "AZazMNaz09[{@`");
            std::string expected = text;
            for (char& c : expected) {
                if (c >= 'A' && c <= 'Z') {
                    c = static_cast<char>(c + 32);
                }
            }
            to_lower_ascii(text);
            INFO("n ", n);
            REQUIRE_EQ(text, expected);
        }
    }
}

PERF(3, "to_lower_ascii beats a branchy scalar loop", 5000) {
    const std::size_t n = 1 << 20;
    Rng rng;
    const std::string source = rng.text(n, "abcXYZ 0123.,;");
    std::string a = source;
    std::string b = source;
    const double ratio = bench::speedup(
        5,
        [&] { for (int r = 0; r < 60; ++r) { a = source; scalar_lower(a.data(), n); bench::do_not_optimize(a.data()); } },
        [&] { for (int r = 0; r < 60; ++r) { b = source; to_lower_ascii(b); bench::do_not_optimize(b.data()); } });
    INFO("speedup ", ratio);
    CHECK_GE(ratio, 4.0);
}

TEST(4, "find_any_of3 finds whichever of the three comes first") {
    std::vector<std::uint8_t> buffer(50, 'x');
    buffer[30] = 'c';
    buffer[10] = 'b';
    buffer[40] = 'a';
    CHECK_EQ(find_any_of3(buffer, 'a', 'b', 'c'), 10);
    CHECK_EQ(find_any_of3(buffer, 'q', 'r', 's'), 50);
    CHECK_EQ(find_any_of3(std::span<const std::uint8_t>(), 'a', 'b', 'c'), 0);
}

TEST(4, "find_any_of3 matches a model on random exact-size buffers") {
    Rng rng;
    for (int round = 0; round < 2000; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(0, 200));
        const std::vector<std::uint8_t> buffer = random_buffer(rng, n, 0, 9);
        const std::size_t expected = static_cast<std::size_t>(
            std::find_if(buffer.begin(), buffer.end(),
                         [](std::uint8_t b) { return b == 3 || b == 5 || b == 9; }) -
            buffer.begin());
        INFO("round ", round);
        REQUIRE_EQ(find_any_of3(buffer, 3, 5, 9), expected);
    }
}

PERF(4, "find_any_of3 beats a scalar scan", 5000) {
    const std::size_t n = 1 << 20;
    std::vector<std::uint8_t> buffer(n, 'x');
    const double ratio = bench::speedup(
        5,
        [&] { for (int r = 0; r < 300; ++r) bench::do_not_optimize(scalar_find3(buffer.data(), n, ',', '\n', '"')); },
        [&] { for (int r = 0; r < 300; ++r) bench::do_not_optimize(find_any_of3(buffer, ',', '\n', '"')); });
    INFO("speedup ", ratio);
    CHECK_GE(ratio, 3.0);
}

TEST(5, "split_fields splits on the separator") {
    const auto fields = split_fields("a,bb,,ccc", ',');
    REQUIRE_EQ(fields.size(), 4);
    CHECK_EQ(fields[0], "a");
    CHECK_EQ(fields[1], "bb");
    CHECK_EQ(fields[2], "");
    CHECK_EQ(fields[3], "ccc");
}

TEST(5, "leading trailing and empty inputs keep empty fields") {
    CHECK_EQ(split_fields("", ',').size(), 1);
    CHECK_EQ(split_fields(",", ',').size(), 2);
    const auto fields = split_fields(",x,", ',');
    REQUIRE_EQ(fields.size(), 3);
    CHECK_EQ(fields[0], "");
    CHECK_EQ(fields[1], "x");
    CHECK_EQ(fields[2], "");
}

TEST(5, "split_fields views the original string") {
    const std::string line = "key=value";
    const auto fields = split_fields(line, '=');
    REQUIRE_EQ(fields.size(), 2);
    CHECK_EQ(fields[0].data(), line.data());
    CHECK_EQ(fields[1].data(), line.data() + 4);
}

TEST(5, "split_fields agrees with a naive splitter on random lines") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const std::string line = rng.text(static_cast<std::size_t>(rng.uniform(0, 200)), "ab,c,,");
        std::vector<std::string_view> expected;
        std::size_t begin = 0;
        for (std::size_t i = 0; i <= line.size(); ++i) {
            if (i == line.size() || line[i] == ',') {
                expected.push_back(std::string_view(line).substr(begin, i - begin));
                begin = i + 1;
            }
        }
        INFO("line ", line);
        REQUIRE_EQ(split_fields(line, ','), expected);
    }
}
