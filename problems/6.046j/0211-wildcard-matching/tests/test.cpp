#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Positions = std::vector<std::size_t>;

namespace {

Positions oracle(std::string_view text, std::string_view pattern) {
    Positions positions;
    if (pattern.size() > text.size()) {
        return positions;
    }
    for (std::size_t start = 0; start + pattern.size() <= text.size(); ++start) {
        bool matched = true;
        for (std::size_t i = 0; i < pattern.size(); ++i) {
            matched = matched && (pattern[i] == '*' || pattern[i] == text[start + i]);
        }
        if (matched) {
            positions.push_back(start);
        }
    }
    return positions;
}

std::string random_pattern(Rng& rng, std::size_t length, std::string_view symbols) {
    std::string pattern(length, '*');
    for (char& c : pattern) {
        if (rng.coin(0.6)) {
            c = symbols[static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(symbols.size()) - 1))];
        }
    }
    return pattern;
}

// Every window agrees until the very last symbol, the worst case for an early-exit scan.
std::pair<std::string, std::string> adversarial(std::size_t n, std::size_t m, char common, char last) {
    std::string pattern(m, common);
    pattern.back() = last;
    return {std::string(n, common), pattern};
}

}  // namespace

TEST(1, "a plain symbol matches where it occurs") {
    CHECK_EQ(match_naive("abba", "b"), (Positions{1, 2}));
}

TEST(1, "the wildcard matches exactly one symbol of either kind") {
    CHECK_EQ(match_naive("abab", "a*a"), (Positions{0}));
    CHECK_EQ(match_naive("aab", "a*b"), (Positions{0}));
    CHECK_EQ(match_naive("abb", "a*b"), (Positions{0}));
}

TEST(1, "a pattern longer than the text has no matches") {
    CHECK(match_naive("ab", "aba").empty());
}

TEST(1, "an all-wildcard pattern matches every window") {
    CHECK_EQ(match_naive("abba", "**"), (Positions{0, 1, 2}));
}

TEST(1, "the empty pattern matches at every position including the end") {
    CHECK_EQ(match_naive("ab", ""), (Positions{0, 1, 2}));
    CHECK_EQ(match_naive("", ""), (Positions{0}));
}

TEST(1, "unknown symbols are rejected") {
    CHECK_THROWS_AS(match_naive("abc", "a"), std::invalid_argument);
    CHECK_THROWS_AS(match_naive("ab", "c"), std::invalid_argument);
    CHECK_THROWS_AS(match_naive("a*", "a"), std::invalid_argument);
}

TEST(1, "random inputs match the definition") {
    Rng rng;
    for (int trial = 0; trial < 2000; ++trial) {
        INFO("trial ", trial);
        const std::string text = rng.text(static_cast<std::size_t>(rng.uniform(0, 30)), "ab");
        const std::string pattern = random_pattern(rng, static_cast<std::size_t>(rng.uniform(0, 8)), "ab");
        REQUIRE_EQ(match_naive(text, pattern), oracle(text, pattern));
    }
}

TEST(2, "basic patterns agree with the definition") {
    for (const auto& [text, pattern] :
         std::vector<std::pair<std::string, std::string>>{
             {"baabba", "b*a"}, {"aaaa", "a"}, {"bbbb", "a"}, {"ab", "ab"}, {"b", "*"}}) {
        INFO("text ", text, " pattern ", pattern);
        CHECK_EQ(match_fft(text, pattern), oracle(text, pattern));
    }
}

TEST(2, "edge sizes") {
    CHECK(match_fft("ab", "aba").empty());
    CHECK_EQ(match_fft("ab", ""), (Positions{0, 1, 2}));
    CHECK_EQ(match_fft("", ""), (Positions{0}));
    CHECK(match_fft("", "a").empty());
}

TEST(2, "a single-symbol text and pattern") {
    CHECK_EQ(match_fft("a", "a"), (Positions{0}));
    CHECK(match_fft("b", "a").empty());
    CHECK_EQ(match_fft("b", "*"), (Positions{0}));
}

TEST(2, "matches on the last window are reported") {
    CHECK_EQ(match_fft("aaab", "ab"), (Positions{2}));
}

TEST(2, "all-wildcard patterns need no fixed symbols") {
    CHECK_EQ(match_fft("abba", "***"), (Positions{0, 1}));
}

TEST(2, "unknown symbols are rejected") {
    CHECK_THROWS_AS(match_fft("abc", "a"), std::invalid_argument);
    CHECK_THROWS_AS(match_fft("ab", "c"), std::invalid_argument);
}

TEST(2, "random inputs match the definition") {
    Rng rng;
    for (int trial = 0; trial < 3000; ++trial) {
        INFO("trial ", trial);
        const std::string text = rng.text(static_cast<std::size_t>(rng.uniform(0, 200)), "ab");
        const std::string pattern = random_pattern(rng, static_cast<std::size_t>(rng.uniform(0, 24)), "ab");
        REQUIRE_EQ(match_fft(text, pattern), oracle(text, pattern));
    }
}

TEST(2, "long adversarial strings match the definition") {
    const auto [text, pattern] = adversarial(4096, 512, 'a', 'b');
    CHECK(match_fft(text, pattern).empty());
    const auto [uniform_text, uniform_pattern] = adversarial(4096, 512, 'a', 'a');
    CHECK_EQ(match_fft(uniform_text, uniform_pattern).size(), 4096 - 512 + 1);
}

PERF(2, "half a million symbols with a long pattern", 2000) {
    const auto [text, pattern] = adversarial(std::size_t{1} << 19, std::size_t{1} << 18, 'a', 'b');
    CHECK(match_fft(text, pattern).empty());
}

TEST(3, "a four-letter alphabet with wildcards") {
    CHECK_EQ(match_alphabet("ACGTACG", "A*G", "ACGT"), (Positions{0, 4}));
}

TEST(3, "a match at an odd bit offset is not a symbol match") {
    // In a 2-bit code the pattern can line up across a symbol boundary.
    CHECK(match_alphabet("AG", "C", "ACGT").empty());
    CHECK(match_alphabet("CCCC", "G", "ACGT").empty());
}

TEST(3, "wildcards expand across the whole symbol") {
    CHECK_EQ(match_alphabet("TTTT", "T*T", "ACGT"), (Positions{0, 1}));
}

TEST(3, "an alphabet size that is not a power of two") {
    CHECK_EQ(match_alphabet("xyzzy", "z*", "xyz"), (Positions{2, 3}));
    CHECK_EQ(match_alphabet("pqrst", "t", "pqrst"), (Positions{4}));
}

TEST(3, "a single-symbol alphabet") {
    CHECK_EQ(match_alphabet("qqq", "q", "q"), (Positions{0, 1, 2}));
    CHECK(match_alphabet("qq", "qqq", "q").empty());
}

TEST(3, "edge sizes") {
    CHECK_EQ(match_alphabet("AC", "", "ACGT"), (Positions{0, 1, 2}));
    CHECK(match_alphabet("AC", "ACG", "ACGT").empty());
}

TEST(3, "bad alphabets and symbols are rejected") {
    CHECK_THROWS_AS(match_alphabet("A", "A", ""), std::invalid_argument);
    CHECK_THROWS_AS(match_alphabet("A", "A", "AA"), std::invalid_argument);
    CHECK_THROWS_AS(match_alphabet("A", "A", "A*"), std::invalid_argument);
    CHECK_THROWS_AS(match_alphabet("AX", "A", "ACGT"), std::invalid_argument);
    CHECK_THROWS_AS(match_alphabet("AC", "Q", "ACGT"), std::invalid_argument);
}

TEST(3, "random strings over many alphabets match the definition") {
    Rng rng;
    const std::string alphabets[] = {"ab", "abc", "ACGT", "01234", "abcdefghij", "pqrstuvwxyz"};
    for (int trial = 0; trial < 3000; ++trial) {
        INFO("trial ", trial);
        const std::string& alphabet = alphabets[static_cast<std::size_t>(rng.uniform(0, 5))];
        const std::string text = rng.text(static_cast<std::size_t>(rng.uniform(0, 40)), alphabet);
        const std::string pattern = random_pattern(rng, static_cast<std::size_t>(rng.uniform(0, 6)), alphabet);
        REQUIRE_EQ(match_alphabet(text, pattern, alphabet), oracle(text, pattern));
    }
}

PERF(3, "a quarter-million DNA symbols", 2500) {
    const auto [text, pattern] = adversarial(std::size_t{1} << 18, std::size_t{1} << 17, 'A', 'C');
    CHECK(match_alphabet(text, pattern, "ACGT").empty());
}
