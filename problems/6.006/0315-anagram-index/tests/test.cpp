#include <algorithm>
#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

std::size_t brute_count(std::string_view text, std::string_view query) {
    std::string sorted_query(query);
    std::sort(sorted_query.begin(), sorted_query.end());
    std::size_t count = 0;
    for (std::size_t i = 0; i + query.size() <= text.size(); ++i) {
        std::string window(text.substr(i, query.size()));
        std::sort(window.begin(), window.end());
        count += window == sorted_query ? 1 : 0;
    }
    return count;
}

}  // namespace

TEST(1, "counts windows that are anagrams of the query") {
    const AnagramIndex index("abcbacab", 3);
    // Windows are abc, bcb, cba, bac, aca, cab; four of them are anagrams of abc.
    CHECK_EQ(index.count("abc"), 4);
    CHECK_EQ(index.count("aca"), 1);
    CHECK_EQ(index.count("bbb"), 0);
}

TEST(1, "distinguishes different letter multisets of equal length") {
    const AnagramIndex index("aabbaacc", 2);
    CHECK_EQ(index.count("ab"), 2);
    CHECK_EQ(index.count("aa"), 2);
    CHECK_EQ(index.count("bb"), 1);
    CHECK_EQ(index.count("cc"), 1);
    CHECK_EQ(index.count("zz"), 0);
}

TEST(1, "k equal to the text length has a single window") {
    const AnagramIndex index("listen", 6);
    CHECK_EQ(index.count("silent"), 1);
    CHECK_EQ(index.count("tinsel"), 1);
    CHECK_EQ(index.count("banana"), 0);
}

TEST(1, "k larger than the text gives zero everywhere") {
    const AnagramIndex index("abc", 5);
    CHECK_EQ(index.count("abcab"), 0);
}

TEST(1, "single letter windows") {
    const AnagramIndex index("abca", 1);
    CHECK_EQ(index.count("a"), 2);
    CHECK_EQ(index.count("b"), 1);
    CHECK_EQ(index.count("z"), 0);
}

TEST(1, "empty text has no windows") {
    const AnagramIndex index("", 2);
    CHECK_EQ(index.count("ab"), 0);
}

TEST(1, "rejects k of zero and queries of the wrong length") {
    CHECK_THROWS_AS(AnagramIndex("abc", 0), std::invalid_argument);
    const AnagramIndex index("abcd", 2);
    CHECK_THROWS_AS(index.count("abc"), std::invalid_argument);
    CHECK_THROWS_AS(index.count("a"), std::invalid_argument);
}

TEST(1, "random text and queries match brute force") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(1, 120));
        const auto k = static_cast<std::size_t>(rng.uniform(1, 12));
        const std::string text = rng.text(n, "abcde");
        const AnagramIndex index(text, k);
        for (int q = 0; q < 20; ++q) {
            std::string query;
            if (rng.coin() && n >= k) {
                // Take a real window and scramble it so positive answers show up.
                query = text.substr(static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n - k))), k);
                rng.shuffle(query);
            } else {
                query = rng.text(k, "abcde");
            }
            REQUIRE_EQ(index.count(query), brute_count(text, query));
        }
    }
}

PERF(1, "two million characters, large windows, many queries", 4000) {
    Rng rng;
    const std::string text = rng.text(2'000'000, "abcdefghijklmnopqrstuvwxyz");
    const std::size_t k = 50'000;
    const AnagramIndex index(text, k);
    std::size_t total = 0;
    for (int i = 0; i < 2000; ++i) {
        total += index.count(std::string_view(text).substr(static_cast<std::size_t>(i) * 100, k));
    }
    CHECK_GE(total, 2000);
}

TEST(2, "answers every query in order") {
    const std::vector<std::string> queries{"ab", "ba", "bb", "cc"};
    CHECK_EQ(count_anagram_substrings("abab", std::span<const std::string>(queries)),
             std::vector<std::size_t>{3, 3, 0, 0});
}

TEST(2, "no queries returns an empty answer") {
    const std::vector<std::string> queries;
    CHECK(count_anagram_substrings("abc", std::span<const std::string>(queries)).empty());
}

TEST(2, "random batches match brute force") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(1, 100));
        const auto k = static_cast<std::size_t>(rng.uniform(1, 10));
        const std::string text = rng.text(n, "abc");
        std::vector<std::string> queries;
        std::vector<std::size_t> expected;
        const auto count = static_cast<std::size_t>(rng.uniform(0, 8));
        for (std::size_t i = 0; i < count; ++i) {
            queries.push_back(rng.text(k, "abc"));
            expected.push_back(brute_count(text, queries.back()));
        }
        CHECK_EQ(count_anagram_substrings(text, std::span<const std::string>(queries)), expected);
    }
}

PERF(2, "a large batch keeps the O(|T| + nk) bound", 3000) {
    Rng rng;
    const std::string text = rng.text(1'000'000, "abcd");
    const std::size_t k = 1000;
    std::vector<std::string> queries;
    for (int i = 0; i < 20'000; ++i) {
        queries.push_back(text.substr(static_cast<std::size_t>(i) * 40, k));
        rng.shuffle(queries.back());
    }
    const auto answers = count_anagram_substrings(text, std::span<const std::string>(queries));
    CHECK_EQ(answers.size(), 20'000);
    CHECK_GE(answers[0], 1);
}

TEST(3, "appended characters extend the text") {
    AnagramIndex index("ab", 2);
    CHECK_EQ(index.count("ab"), 1);
    CHECK_EQ(index.length(), 2);
    index.append('a');
    CHECK_EQ(index.length(), 3);
    CHECK_EQ(index.count("ab"), 2);
    index.append('c');
    CHECK_EQ(index.count("ac"), 1);
    CHECK_EQ(index.count("ab"), 2);
}

TEST(3, "starting empty and appending matches a one-shot build") {
    Rng rng;
    for (int round = 0; round < 100; ++round) {
        INFO("round ", round);
        const auto k = static_cast<std::size_t>(rng.uniform(1, 8));
        const std::string text = rng.text(static_cast<std::size_t>(rng.uniform(1, 150)), "abcd");
        AnagramIndex incremental("", k);
        for (std::size_t i = 0; i < text.size(); ++i) {
            incremental.append(text[i]);
            if (i % 7 == 0) {
                const std::string query = rng.text(k, "abcd");
                REQUIRE_EQ(incremental.count(query), brute_count(std::string_view(text).substr(0, i + 1), query));
            }
        }
        CHECK_EQ(incremental.length(), text.size());
    }
}

TEST(3, "appends and queries interleave against brute force") {
    Rng rng;
    std::string text = rng.text(30, "ab");
    AnagramIndex index(text, 4);
    for (int step = 0; step < 2000; ++step) {
        INFO("step ", step);
        if (rng.coin(0.7)) {
            const char ch = static_cast<char>('a' + rng.uniform(0, 1));
            index.append(ch);
            text.push_back(ch);
        } else {
            const std::string query = rng.text(4, "ab");
            REQUIRE_EQ(index.count(query), brute_count(text, query));
        }
    }
}

PERF(3, "ten million appends stay constant time", 3000) {
    AnagramIndex index("", 16);
    Rng rng(7);
    for (int i = 0; i < 10'000'000; ++i) {
        index.append(static_cast<char>('a' + (rng.engine()() % 4)));
    }
    CHECK_EQ(index.length(), 10'000'000);
    CHECK_GE(index.count("aaaabbbbccccdddd"), 1);
}
