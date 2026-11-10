#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Counts = std::map<std::string, std::int64_t>;

WordFrequencies frequencies_from(const Counts& counts) {
    WordFrequencies frequencies;
    for (const auto& [word, count] : counts) {
        std::string text;
        for (std::int64_t i = 0; i < count; ++i) {
            text += word;
            text += ' ';
        }
        frequencies.add_text(text);
    }
    return frequencies;
}

Counts random_vocabulary(Rng& rng, std::size_t size, std::size_t max_length) {
    Counts counts;
    for (std::size_t i = 0; i < size; ++i) {
        const auto length = static_cast<std::size_t>(rng.uniform(1, static_cast<std::int64_t>(max_length)));
        counts[rng.text(length, "abc")] = rng.uniform(1, 9);
    }
    return counts;
}

// (best score, fewest words among those) over every way to place breaks.
std::pair<std::int64_t, std::int64_t> exhaustive(const Counts& counts, std::string_view text) {
    if (text.empty()) {
        return {0, 0};
    }
    std::int64_t best_score = -1;
    std::int64_t best_words = std::numeric_limits<std::int64_t>::max();
    const std::size_t n = text.size();
    for (std::size_t mask = 0; mask < (std::size_t{1} << (n - 1)); ++mask) {
        std::int64_t score = 0;
        std::int64_t words = 0;
        std::size_t start = 0;
        for (std::size_t i = 0; i <= n; ++i) {
            if (i == n || ((mask >> i) & 1)) {
                const auto it = counts.find(std::string(text.substr(start, i + 1 - start)));
                if (it != counts.end()) {
                    score += it->second;
                }
                ++words;
                start = i + 1;
            }
        }
        if (score > best_score || (score == best_score && words < best_words)) {
            best_score = score;
            best_words = words;
        }
    }
    return {best_score, best_words};
}

std::int64_t score_of(const Counts& counts, const std::vector<std::string_view>& words) {
    std::int64_t score = 0;
    for (const std::string_view word : words) {
        const auto it = counts.find(std::string(word));
        if (it != counts.end()) {
            score += it->second;
        }
    }
    return score;
}

void check_restoration(std::string_view corrupted, const std::vector<std::string_view>& words) {
    std::string joined;
    for (const std::string_view word : words) {
        REQUIRE(!word.empty());
        joined += word;
    }
    REQUIRE_EQ(joined, corrupted);
}

}  // namespace

TEST(1, "words are counted across calls") {
    WordFrequencies frequencies;
    frequencies.add_text("the cat sat");
    frequencies.add_text("on the mat");
    CHECK_EQ(frequencies.count("the"), 2);
    CHECK_EQ(frequencies.count("cat"), 1);
    CHECK_EQ(frequencies.count("dog"), 0);
    CHECK_EQ(frequencies.distinct_words(), 5);
}

TEST(1, "extra spaces do not create words") {
    WordFrequencies frequencies;
    frequencies.add_text("   a  b   a   ");
    CHECK_EQ(frequencies.count("a"), 2);
    CHECK_EQ(frequencies.count("b"), 1);
    CHECK_EQ(frequencies.distinct_words(), 2);
    CHECK_EQ(frequencies.count(""), 0);
}

TEST(1, "an empty document changes nothing") {
    WordFrequencies frequencies;
    frequencies.add_text("");
    frequencies.add_text("     ");
    CHECK_EQ(frequencies.distinct_words(), 0);
}

TEST(1, "lookups accept string_view without copying the key type") {
    WordFrequencies frequencies;
    const std::string text = "zebra";
    frequencies.add_text(text);
    const std::string_view view = std::string_view(text).substr(0, 5);
    CHECK_EQ(frequencies.count(view), 1);
    CHECK_EQ(frequencies.count(view.substr(0, 3)), 0);
}

TEST(2, "an empty log has score zero") {
    const WordFrequencies frequencies;
    CHECK_EQ(best_restoration_score(frequencies, ""), 0);
}

TEST(2, "unknown text still has a restoration, worth zero") {
    const WordFrequencies frequencies;
    CHECK_EQ(best_restoration_score(frequencies, "abc"), 0);
}

TEST(2, "known words are preferred over unknown segments") {
    WordFrequencies frequencies;
    frequencies.add_text("cat cat dog");
    CHECK_EQ(best_restoration_score(frequencies, "catdog"), 3);
}

TEST(2, "the highest total can beat the longest words") {
    WordFrequencies frequencies;
    frequencies.add_text("in in in in side side side inside");
    CHECK_EQ(best_restoration_score(frequencies, "inside"), 4 + 3);
}

TEST(2, "random logs match trying every split") {
    Rng rng;
    for (int trial = 0; trial < 800; ++trial) {
        const Counts counts = random_vocabulary(rng, 15, 4);
        const WordFrequencies frequencies = frequencies_from(counts);
        const std::string text = rng.text(static_cast<std::size_t>(rng.uniform(0, 12)), "abc");
        INFO("text ", text);
        REQUIRE_EQ(best_restoration_score(frequencies, text), exhaustive(counts, text).first);
    }
}

PERF(2, "three hundred characters of log", 800) {
    Rng rng;
    const Counts counts = random_vocabulary(rng, 40, 5);
    const WordFrequencies frequencies = frequencies_from(counts);
    const std::string text = rng.text(300, "abc");
    CHECK_GE(best_restoration_score(frequencies, text), 0);
}

TEST(3, "an empty log restores to no words") {
    const WordFrequencies frequencies;
    const Restoration result = restore(frequencies, "");
    CHECK_EQ(result.score, 0);
    CHECK(result.words.empty());
}

TEST(3, "words point into the corrupted log") {
    WordFrequencies frequencies;
    frequencies.add_text("ab cd");
    const std::string corrupted = "abcd";
    const Restoration result = restore(frequencies, corrupted);
    REQUIRE_EQ(result.words.size(), 2);
    CHECK(result.words[0].data() == corrupted.data());
    CHECK(result.words[1].data() == corrupted.data() + 2);
}

TEST(3, "random restorations are real and optimal") {
    Rng rng;
    for (int trial = 0; trial < 800; ++trial) {
        const Counts counts = random_vocabulary(rng, 15, 4);
        const WordFrequencies frequencies = frequencies_from(counts);
        const std::string text = rng.text(static_cast<std::size_t>(rng.uniform(0, 12)), "abc");
        INFO("text ", text);
        const Restoration result = restore(frequencies, text);
        check_restoration(text, result.words);
        REQUIRE_EQ(result.score, exhaustive(counts, text).first);
        REQUIRE_EQ(score_of(counts, result.words), result.score);
    }
}

TEST(4, "for_each visits every distinct word exactly once") {
    WordFrequencies frequencies;
    frequencies.add_text("a b a c b a");
    Counts seen;
    frequencies.for_each([&](std::string_view word, std::int64_t count) {
        CHECK(!seen.contains(std::string(word)));
        seen[std::string(word)] = count;
    });
    CHECK_EQ(seen, Counts{{"a", 3}, {"b", 2}, {"c", 1}});
}

TEST(4, "unknown text collapses to a single word") {
    WordFrequencies frequencies;
    frequencies.add_text("qqq");
    const FastRestorer restorer(frequencies);
    const Restoration result = restorer.restore("xyz");
    CHECK_EQ(result.score, 0);
    REQUIRE_EQ(result.words.size(), 1);
    CHECK_EQ(result.words[0], "xyz");
}

TEST(4, "known words are kept while unknown runs merge") {
    WordFrequencies frequencies;
    frequencies.add_text("cat cat");
    const FastRestorer restorer(frequencies);
    const Restoration result = restorer.restore("xxcatzz");
    CHECK_EQ(result.score, 2);
    check_restoration("xxcatzz", result.words);
    CHECK_EQ(result.words.size(), 3);
}

TEST(4, "an empty log restores to no words") {
    const WordFrequencies frequencies;
    const FastRestorer restorer(frequencies);
    CHECK(restorer.restore("").words.empty());
}

TEST(4, "random logs match trying every split, including the tie-break") {
    Rng rng;
    for (int trial = 0; trial < 800; ++trial) {
        const Counts counts = random_vocabulary(rng, 15, 4);
        const WordFrequencies frequencies = frequencies_from(counts);
        const FastRestorer restorer(frequencies);
        const std::string text = rng.text(static_cast<std::size_t>(rng.uniform(0, 12)), "abcd");
        INFO("text ", text);
        const Restoration result = restorer.restore(text);
        check_restoration(text, result.words);
        const auto [best_score, fewest_words] = exhaustive(counts, text);
        REQUIRE_EQ(result.score, best_score);
        REQUIRE_EQ(score_of(counts, result.words), best_score);
        REQUIRE_EQ(static_cast<std::int64_t>(result.words.size()), fewest_words);
    }
}

PERF(4, "two million characters with short vocabulary words", 3000) {
    Rng rng;
    std::vector<std::string> vocabulary;
    WordFrequencies frequencies;
    std::string corpus;
    for (int i = 0; i < 3000; ++i) {
        const std::string word = rng.text(static_cast<std::size_t>(rng.uniform(3, 12)), "abcdefghij");
        vocabulary.push_back(word);
        corpus += word;
        corpus += ' ';
    }
    frequencies.add_text(corpus);
    const FastRestorer restorer(frequencies);
    std::string log;
    log.reserve(2'000'000);
    while (log.size() < 2'000'000) {
        log += vocabulary[static_cast<std::size_t>(rng.uniform(0, 2999))];
    }
    const Restoration result = restorer.restore(log);
    check_restoration(log, result.words);
    CHECK_GT(result.score, 100'000);
}
