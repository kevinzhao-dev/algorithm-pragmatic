#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "07_heap_meter.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

// Dijkstra over every string reachable by single edits, so this oracle shares no reasoning with
// the table. Intermediate strings never need to exceed the longer input.
int search_oracle(const std::string& from, const std::string& to, EditCosts costs) {
    std::string alphabet = from + to;
    std::ranges::sort(alphabet);
    alphabet.erase(std::unique(alphabet.begin(), alphabet.end()), alphabet.end());
    const std::size_t max_length = std::max(from.size(), to.size());

    std::map<std::string, int> best{{from, 0}};
    using Item = std::pair<int, std::string>;
    std::priority_queue<Item, std::vector<Item>, std::greater<>> queue;
    queue.push({0, from});
    const auto relax = [&](std::string text, int cost) {
        const auto found = best.find(text);
        if (found == best.end() || cost < found->second) {
            best[text] = cost;
            queue.push({cost, std::move(text)});
        }
    };
    while (!queue.empty()) {
        auto [cost, text] = queue.top();
        queue.pop();
        if (cost != best[text]) {
            continue;
        }
        if (text == to) {
            return cost;
        }
        for (std::size_t at = 0; at < text.size(); ++at) {
            std::string erased = text;
            erased.erase(at, 1);
            relax(std::move(erased), cost + costs.erase);
            for (const char ch : alphabet) {
                if (ch != text[at]) {
                    std::string replaced = text;
                    replaced[at] = ch;
                    relax(std::move(replaced), cost + costs.replace);
                }
            }
        }
        if (text.size() < max_length) {
            for (std::size_t at = 0; at <= text.size(); ++at) {
                for (const char ch : alphabet) {
                    std::string inserted = text;
                    inserted.insert(at, 1, ch);
                    relax(std::move(inserted), cost + costs.insert);
                }
            }
        }
    }
    return -1;
}

// Applies a script step by step and returns the resulting text and its total cost.
std::pair<std::string, int> replay(std::string text, const std::vector<Edit>& script,
                                   EditCosts costs) {
    int total = 0;
    for (const Edit& edit : script) {
        switch (edit.op) {
            case EditOp::Insert:
                REQUIRE(edit.at <= text.size());
                text.insert(edit.at, 1, edit.ch);
                total += costs.insert;
                break;
            case EditOp::Erase:
                REQUIRE(edit.at < text.size());
                text.erase(edit.at, 1);
                total += costs.erase;
                break;
            case EditOp::Replace:
                REQUIRE(edit.at < text.size());
                text[edit.at] = edit.ch;
                total += costs.replace;
                break;
        }
    }
    return {std::move(text), total};
}

EditCosts random_costs(Rng& rng) {
    return {static_cast<int>(rng.uniform(1, 6)), static_cast<int>(rng.uniform(1, 6)),
            static_cast<int>(rng.uniform(1, 12))};
}

std::string random_word(Rng& rng, std::size_t max_length, std::string_view alphabet = "abc") {
    return rng.text(static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(max_length))),
                    alphabet);
}

}  // namespace

TEST(1, "empty strings need one edit per character") {
    CHECK_EQ(edit_distance("", ""), 0);
    CHECK_EQ(edit_distance("abc", ""), 3);
    CHECK_EQ(edit_distance("", "abcd"), 4);
}

TEST(1, "identical strings cost nothing") {
    CHECK_EQ(edit_distance("kitten", "kitten"), 0);
}

TEST(1, "a single substitution costs one") {
    CHECK_EQ(edit_distance("flour", "floor"), 1);
}

TEST(1, "unit costs on a classic pair of words") {
    CHECK_EQ(edit_distance("saturday", "sunday"), 3);
}

TEST(1, "insert and erase costs are not interchangeable") {
    const EditCosts costs{1, 5, 9};
    CHECK_EQ(edit_distance("", "aa", costs), 2);
    CHECK_EQ(edit_distance("aa", "", costs), 10);
}

TEST(1, "an expensive replace is routed through insert plus erase") {
    const EditCosts costs{1, 1, 5};
    CHECK_EQ(edit_distance("x", "y", costs), 2);
}

TEST(1, "unit costs match graph search over all strings") {
    Rng rng;
    for (int trial = 0; trial < 600; ++trial) {
        const std::string from = random_word(rng, 5);
        const std::string to = random_word(rng, 5);
        INFO("from=", from, " to=", to);
        REQUIRE_EQ(edit_distance(from, to), search_oracle(from, to, {}));
    }
}

TEST(1, "random costs match graph search over all strings") {
    Rng rng;
    for (int trial = 0; trial < 400; ++trial) {
        const std::string from = random_word(rng, 4);
        const std::string to = random_word(rng, 5);
        const EditCosts costs = random_costs(rng);
        INFO("from=", from, " to=", to, " costs ", costs.insert, " ", costs.erase, " ",
             costs.replace);
        REQUIRE_EQ(edit_distance(from, to, costs), search_oracle(from, to, costs));
    }
}

PERF(1, "five thousand by five thousand", 500) {
    Rng rng;
    const std::string a = rng.text(5000, "acgt");
    const std::string b = rng.text(5000, "acgt");
    CHECK_GT(edit_distance(a, b), 0);
}

TEST(2, "scripts for trivial cases") {
    CHECK(edit_script("", "").empty());
    CHECK(edit_script("same", "same").empty());
    CHECK_EQ(edit_script("abc", "").size(), 3);
    CHECK_EQ(edit_script("", "abc").size(), 3);
}

TEST(2, "positions refer to the string as it stands when applied") {
    const auto [text, total] = replay("axb", edit_script("axb", "ab"), {});
    CHECK_EQ(text, "ab");
    CHECK_EQ(total, 1);

    const auto [grown, grown_total] = replay("ac", edit_script("ac", "abcd"), {});
    CHECK_EQ(grown, "abcd");
    CHECK_EQ(grown_total, 2);
}

TEST(2, "random scripts replay exactly and cost the optimum") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const std::string from = random_word(rng, 14, "abcd");
        const std::string to = random_word(rng, 14, "abcd");
        const EditCosts costs = random_costs(rng);
        INFO("from=", from, " to=", to, " costs ", costs.insert, " ", costs.erase, " ",
             costs.replace);
        const auto [text, total] = replay(from, edit_script(from, to, costs), costs);
        REQUIRE_EQ(text, to);
        REQUIRE_EQ(total, edit_distance(from, to, costs));
    }
}

TEST(3, "edit_distance keeps memory linear in the shorter string") {
    Rng rng;
    const std::string a = rng.text(2000, "acgt");
    const std::string b = rng.text(1200, "acgt");
    heap_meter::Scope scope;
    const int distance = edit_distance(a, b);
    CHECK_LE(scope.peak_bytes(), 32 * b.size());
    CHECK_GT(distance, 0);
}

TEST(3, "edit_distance rolls over the shorter string in both argument orders") {
    Rng rng;
    const std::string a = rng.text(4000, "acgt");
    const std::string b = rng.text(16, "acgt");
    {
        heap_meter::Scope scope;
        CHECK_GT(edit_distance(a, b), 0);
        CHECK_LE(scope.peak_bytes(), 8192);
    }
    {
        heap_meter::Scope scope;
        CHECK_GT(edit_distance(b, a), 0);
        CHECK_LE(scope.peak_bytes(), 8192);
    }
}

TEST(3, "rolling keeps asymmetric costs correct when strings are swapped") {
    Rng rng;
    for (int trial = 0; trial < 400; ++trial) {
        const std::string from = random_word(rng, 4);
        const std::string to = random_word(rng, 5);
        const EditCosts costs = random_costs(rng);
        INFO("from=", from, " to=", to, " costs ", costs.insert, " ", costs.erase, " ",
             costs.replace);
        REQUIRE_EQ(edit_distance(from, to, costs), search_oracle(from, to, costs));
        REQUIRE_EQ(edit_distance(from, to, costs),
                   replay(from, edit_script(from, to, costs), costs).second);
    }
}

TEST(4, "the limit is inclusive") {
    CHECK_EQ(edit_distance_within("abc", "abd", 1), std::optional<int>(1));
    CHECK_EQ(edit_distance_within("abc", "abd", 0), std::optional<int>());
}

TEST(4, "a length gap beyond the limit is rejected") {
    CHECK_EQ(edit_distance_within("abcd", "", 3), std::optional<int>());
    CHECK_EQ(edit_distance_within("", "abcd", 4), std::optional<int>(4));
}

TEST(4, "identical strings have distance zero at any limit") {
    CHECK_EQ(edit_distance_within("kitten", "kitten", 0), std::optional<int>(0));
    CHECK_EQ(edit_distance_within("", "", 0), std::optional<int>(0));
}

TEST(4, "random pairs match the full table at every limit") {
    Rng rng;
    for (int trial = 0; trial < 3000; ++trial) {
        const std::string from = random_word(rng, 20);
        const std::string to = random_word(rng, 20);
        const int limit = static_cast<int>(rng.uniform(0, 22));
        INFO("from=", from, " to=", to, " limit=", limit);
        const int distance = edit_distance(from, to);
        const std::optional<int> expected =
            distance <= limit ? std::optional<int>(distance) : std::nullopt;
        REQUIRE_EQ(edit_distance_within(from, to, limit), expected);
    }
}

TEST(4, "a huge limit does not allocate a huge band") {
    heap_meter::Scope scope;
    CHECK_EQ(edit_distance_within("short", "word", 1'000'000'000),
             std::optional<int>(edit_distance("short", "word")));
    CHECK_LE(scope.peak_bytes(), 4096);
}

PERF(4, "nearly identical long strings with a small limit", 300) {
    Rng rng;
    const std::string a = rng.text(300'000, "acgt");
    std::string b = a;
    for (int edits = 0; edits < 6; ++edits) {
        b[static_cast<std::size_t>(rng.uniform(0, 299'999))] = 'z';
    }
    const auto distance = edit_distance_within(a, b, 8);
    REQUIRE(distance.has_value());
    CHECK_LE(*distance, 6);
    CHECK_GT(*distance, 0);
}

TEST(5, "an empty dictionary suggests nothing") {
    CHECK(suggest("anything", {}, 2).empty());
}

TEST(5, "suggestions are ordered by distance then alphabetically") {
    const std::vector<std::string> dictionary{"stack", "slack", "black", "stick", "track", "smack"};
    const auto result = suggest("stack", dictionary, 1);
    REQUIRE_EQ(result.size(), 4);  // "black" and "track" are two edits away
    CHECK_EQ(result[0].word, "stack");
    CHECK_EQ(result[0].distance, 0);
    std::vector<std::string> words;
    for (std::size_t i = 1; i < result.size(); ++i) {
        CHECK_EQ(result[i].distance, 1);
        words.emplace_back(result[i].word);
    }
    CHECK_EQ(words, std::vector<std::string>{"slack", "smack", "stick"});
}

TEST(5, "suggestions point into the dictionary") {
    const std::vector<std::string> dictionary{"alpha"};
    const auto result = suggest("alpha", dictionary, 0);
    REQUIRE_EQ(result.size(), 1);
    CHECK(result[0].word.data() == dictionary[0].data());
}

TEST(5, "random dictionaries match a brute-force model") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        std::set<std::string> unique;
        for (int i = 0; i < 40; ++i) {
            unique.insert(random_word(rng, 7));
        }
        const std::vector<std::string> dictionary(unique.begin(), unique.end());
        const std::string query = random_word(rng, 7);
        const int limit = static_cast<int>(rng.uniform(0, 4));

        std::vector<std::pair<int, std::string>> expected;
        for (const std::string& word : dictionary) {
            const int distance = edit_distance(query, word);
            if (distance <= limit) {
                expected.emplace_back(distance, word);
            }
        }
        std::ranges::sort(expected);

        std::vector<std::pair<int, std::string>> actual;
        for (const Suggestion& s : suggest(query, dictionary, limit)) {
            actual.emplace_back(s.distance, s.word);
        }
        INFO("query=", query, " limit=", limit);
        REQUIRE_EQ(actual, expected);
    }
}
