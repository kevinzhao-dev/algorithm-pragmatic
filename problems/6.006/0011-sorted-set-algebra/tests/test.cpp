#include <algorithm>
#include <cmath>
#include <iterator>
#include <numeric>
#include <set>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;

namespace {

std::vector<int> random_set(Rng& rng, std::size_t size, int universe) {
    std::set<int> values;
    while (values.size() < size) {
        values.insert(static_cast<int>(rng.uniform(0, universe)));
    }
    return {values.begin(), values.end()};
}

std::vector<Counted<int>> counted(const std::vector<int>& values) {
    std::vector<Counted<int>> out;
    out.reserve(values.size());
    for (int v : values) {
        out.emplace_back(v);
    }
    return out;
}

std::vector<int> standard_intersection(const std::vector<int>& a, const std::vector<int>& b) {
    std::vector<int> out;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(out));
    return out;
}

// A "large" set of the even numbers below 2n, and a "small" subset of m of them, spread evenly.
void spread_sets(std::size_t n, std::size_t m, std::vector<int>& large, std::vector<int>& small) {
    large.resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        large[i] = static_cast<int>(2 * i);
    }
    small.resize(m);
    for (std::size_t i = 0; i < m; ++i) {
        small[i] = large[i * (n / m)];
    }
}

std::uint64_t comparisons_of_gallop(const std::vector<int>& a, const std::vector<int>& b) {
    const auto left = counted(a);
    const auto right = counted(b);
    Counted<int>::comparisons() = 0;
    const auto result = gallop_intersect<Counted<int>>(left, right);
    CHECK_EQ(result.size(), standard_intersection(a, b).size());
    return Counted<int>::comparisons();
}

}  // namespace

TEST(1, "binomial handles the edges") {
    CHECK_EQ(binomial(0, 0), 1);
    CHECK_EQ(binomial(7, 0), 1);
    CHECK_EQ(binomial(7, 7), 1);
    CHECK_EQ(binomial(7, 1), 7);
    CHECK_EQ(binomial(7, 2), 21);
    CHECK_EQ(binomial(20, 10), 184'756);
}

TEST(1, "binomial is zero when k exceeds n") {
    CHECK_EQ(binomial(3, 4), 0);
    CHECK_EQ(binomial(0, 1), 0);
    CHECK_EQ(binomial(100, 101), 0);
}

TEST(1, "binomial matches Pascal's triangle up to the 64-bit limit") {
    std::vector<std::vector<std::uint64_t>> pascal(68);
    for (std::size_t n = 0; n < pascal.size(); ++n) {
        pascal[n].assign(n + 1, 1);
        for (std::size_t k = 1; k < n; ++k) {
            pascal[n][k] = pascal[n - 1][k - 1] + pascal[n - 1][k];
        }
    }
    for (std::size_t n = 0; n < pascal.size(); ++n) {
        for (std::size_t k = 0; k <= n; ++k) {
            INFO("n=", n, " k=", k);
            REQUIRE_EQ(binomial(n, k), pascal[n][k]);
        }
    }
}

TEST(1, "binomial survives intermediate values beyond 64 bits") {
    CHECK_EQ(binomial(67, 33), 14'226'520'737'620'288'370ULL);
    CHECK_EQ(binomial(67, 34), 14'226'520'737'620'288'370ULL);
    CHECK_EQ(binomial(2'000'000, 2), 1'999'999'000'000ULL);
}

TEST(1, "set_of sorts and removes duplicates") {
    const std::vector<int> domain = {4, 1, 3, 2, 1, 6};
    const auto squares_mod_seven = set_of(domain, [](int x) { return x * x % 7; });
    CHECK_EQ(squares_mod_seven, std::vector<int>{1, 2, 4});
}

TEST(1, "set_of works over an empty domain") {
    const std::vector<int> domain;
    CHECK(set_of(domain, [](int x) { return x; }).empty());
}

TEST(1, "set_of may produce a different element type") {
    const std::vector<int> domain = {12, 3, 12, 7};
    const auto names = set_of(domain, [](int x) { return "n" + std::to_string(x % 10); });
    CHECK_EQ(names, std::vector<std::string>{"n2", "n3", "n7"});
}

TEST(1, "set_of over a binomial expression") {
    const std::vector<std::uint64_t> domain = {0, 1, 2, 3};
    const auto values = set_of(domain, [](std::uint64_t i) { return i + binomial(4, i); });
    CHECK_EQ(values, std::vector<std::uint64_t>{1, 5, 7, 8});
}

TEST(2, "intersect keeps only common elements") {
    const std::vector<int> a = {1, 3, 5, 7};
    const std::vector<int> b = {3, 4, 5, 6};
    CHECK_EQ(intersect<int>(a, b), std::vector<int>{3, 5});
    CHECK_EQ(union_size<int>(a, b), 6);
    CHECK_EQ(difference_size<int>(a, b), 2);
    CHECK_EQ(difference_size<int>(b, a), 2);
}

TEST(2, "empty sets on either side") {
    const std::vector<int> empty;
    const std::vector<int> a = {2, 4};
    CHECK(intersect<int>(empty, a).empty());
    CHECK(intersect<int>(a, empty).empty());
    CHECK(intersect<int>(empty, empty).empty());
    CHECK_EQ(union_size<int>(empty, a), 2);
    CHECK_EQ(union_size<int>(a, empty), 2);
    CHECK_EQ(union_size<int>(empty, empty), 0);
    CHECK_EQ(difference_size<int>(a, empty), 2);
    CHECK_EQ(difference_size<int>(empty, a), 0);
}

TEST(2, "identical sets") {
    const std::vector<int> a = {1, 2, 3};
    CHECK_EQ(intersect<int>(a, a), a);
    CHECK_EQ(union_size<int>(a, a), 3);
    CHECK_EQ(difference_size<int>(a, a), 0);
}

TEST(2, "disjoint sets, interleaved and separated") {
    const std::vector<int> odd = {1, 3, 5};
    const std::vector<int> even = {2, 4, 6};
    const std::vector<int> far = {10, 11};
    CHECK(intersect<int>(odd, even).empty());
    CHECK_EQ(union_size<int>(odd, even), 6);
    CHECK_EQ(difference_size<int>(odd, even), 3);
    CHECK(intersect<int>(odd, far).empty());
    CHECK_EQ(union_size<int>(far, odd), 5);
}

TEST(2, "one set strictly inside the other") {
    const std::vector<int> outer = {1, 2, 3, 4, 5, 6};
    const std::vector<int> inner = {2, 5};
    CHECK_EQ(intersect<int>(outer, inner), inner);
    CHECK_EQ(union_size<int>(outer, inner), 6);
    CHECK_EQ(difference_size<int>(outer, inner), 4);
    CHECK_EQ(difference_size<int>(inner, outer), 0);
}

TEST(2, "operations work with any ordered element type") {
    const std::vector<std::string> a = {"ada", "bob", "cleo"};
    const std::vector<std::string> b = {"bob", "dan"};
    CHECK_EQ(intersect<std::string>(a, b), std::vector<std::string>{"bob"});
    CHECK_EQ(union_size<std::string>(a, b), 4);
    CHECK_EQ(difference_size<std::string>(a, b), 2);
}

TEST(2, "random sets match the standard library") {
    Rng rng;
    for (int round = 0; round < 2'000; ++round) {
        INFO("round ", round);
        const auto a = random_set(rng, static_cast<std::size_t>(rng.uniform(0, 40)), 80);
        const auto b = random_set(rng, static_cast<std::size_t>(rng.uniform(0, 40)), 80);
        const auto expected = standard_intersection(a, b);
        REQUIRE_EQ(intersect<int>(a, b), expected);
        REQUIRE_EQ(union_size<int>(a, b), a.size() + b.size() - expected.size());
        REQUIRE_EQ(difference_size<int>(a, b), a.size() - expected.size());
    }
}

TEST(2, "linear scan: comparisons stay proportional to |a| + |b|") {
    Rng rng;
    const auto a = counted(random_set(rng, 10'000, 200'000));
    const auto b = counted(random_set(rng, 10'000, 200'000));
    const std::size_t limit = 3 * (a.size() + b.size());

    Counted<int>::comparisons() = 0;
    intersect<Counted<int>>(a, b);
    CHECK_LE(Counted<int>::comparisons(), limit);

    Counted<int>::comparisons() = 0;
    union_size<Counted<int>>(a, b);
    CHECK_LE(Counted<int>::comparisons(), limit);

    Counted<int>::comparisons() = 0;
    difference_size<Counted<int>>(a, b);
    CHECK_LE(Counted<int>::comparisons(), limit);
}

TEST(3, "gallop_intersect matches the standard library on random sets") {
    Rng rng;
    for (int round = 0; round < 2'000; ++round) {
        INFO("round ", round);
        const auto a = random_set(rng, static_cast<std::size_t>(rng.uniform(0, 40)), 80);
        const auto b = random_set(rng, static_cast<std::size_t>(rng.uniform(0, 200)), 800);
        REQUIRE_EQ(gallop_intersect<int>(a, b), standard_intersection(a, b));
        REQUIRE_EQ(gallop_intersect<int>(b, a), standard_intersection(a, b));
    }
}

TEST(3, "gallop_intersect edge cases") {
    const std::vector<int> empty;
    const std::vector<int> one = {5};
    const std::vector<int> many = {1, 3, 5, 7, 9};
    CHECK(gallop_intersect<int>(empty, many).empty());
    CHECK(gallop_intersect<int>(many, empty).empty());
    CHECK_EQ(gallop_intersect<int>(one, many), one);
    CHECK_EQ(gallop_intersect<int>(many, one), one);
    CHECK_EQ(gallop_intersect<int>(many, many), many);
    CHECK_EQ(gallop_intersect<int>(std::vector<int>{9}, many), std::vector<int>{9});
    CHECK(gallop_intersect<int>(std::vector<int>{10}, many).empty());
    CHECK(gallop_intersect<int>(std::vector<int>{0}, many).empty());
}

TEST(3, "gallop_intersect costs O(m log(n/m)), not O(n) and not O(m log n)") {
    std::vector<int> large;
    std::vector<int> small;
    // The constant is the slack granted over 2 lg(n/m) + O(1) comparisons per element of the
    // small set. A linear merge fails at high skew; a binary search over the whole large set
    // for each element fails at low skew.
    constexpr double kFactor = 7.0;
    const std::vector<std::pair<std::size_t, std::size_t>> shapes = {
        {1 << 20, 1 << 19},  // n/m = 2
        {1 << 20, 1 << 16},  // n/m = 16
        {1 << 20, 1 << 10},  // n/m = 1024
    };
    for (const auto& [n, m] : shapes) {
        spread_sets(n, m, large, small);
        const double budget =
            kFactor * static_cast<double>(m) * (1.0 + std::log2(static_cast<double>(n) / m));
        INFO("n=", n, " m=", m);
        CHECK_LE(static_cast<double>(comparisons_of_gallop(small, large)), budget);
        CHECK_LE(static_cast<double>(comparisons_of_gallop(large, small)), budget);
    }
}

TEST(4, "query intersects every required term") {
    PostingIndex index;
    index.add("cat", {1, 2, 3, 8});
    index.add("dog", {2, 3, 4, 8, 9});
    index.add("fish", {3, 8, 100});
    const std::vector<std::string> must = {"cat", "dog", "fish"};
    CHECK_EQ(index.query(must), std::vector<std::int64_t>{3, 8});
}

TEST(4, "must_not removes documents") {
    PostingIndex index;
    index.add("cat", {1, 2, 3, 4});
    index.add("dog", {2, 4});
    const std::vector<std::string> must = {"cat"};
    const std::vector<std::string> must_not = {"dog"};
    CHECK_EQ(index.query(must, must_not), std::vector<std::int64_t>{1, 3});
}

TEST(4, "unknown terms and empty queries") {
    PostingIndex index;
    index.add("cat", {1, 2});
    const std::vector<std::string> unknown = {"cat", "zebra"};
    CHECK(index.query(unknown).empty());
    const std::vector<std::string> none;
    CHECK(index.query(none).empty());
    const std::vector<std::string> cat = {"cat"};
    const std::vector<std::string> ignored = {"zebra"};
    CHECK_EQ(index.query(cat, ignored), std::vector<std::int64_t>{1, 2});
}

TEST(4, "adding a term twice replaces its list") {
    PostingIndex index;
    index.add("cat", {1, 2, 3});
    index.add("cat", {7});
    const std::vector<std::string> cat = {"cat"};
    CHECK_EQ(index.query(cat), std::vector<std::int64_t>{7});
}

TEST(4, "random queries match a brute-force model") {
    Rng rng;
    for (int round = 0; round < 800; ++round) {
        INFO("round ", round);
        PostingIndex index;
        std::vector<std::set<std::int64_t>> lists;
        for (int term = 0; term < 6; ++term) {
            const auto size = static_cast<std::size_t>(rng.uniform(0, 40));
            std::set<std::int64_t> values;
            while (values.size() < size) {
                values.insert(rng.uniform(0, 60));
            }
            index.add("t" + std::to_string(term),
                      std::vector<std::int64_t>(values.begin(), values.end()));
            lists.push_back(std::move(values));
        }
        std::vector<std::string> must;
        std::vector<std::string> must_not;
        std::vector<int> must_ids;
        std::vector<int> must_not_ids;
        for (int term = 0; term < 6; ++term) {
            const auto role = rng.uniform(0, 2);
            if (role == 0) {
                must.push_back("t" + std::to_string(term));
                must_ids.push_back(term);
            } else if (role == 1) {
                must_not.push_back("t" + std::to_string(term));
                must_not_ids.push_back(term);
            }
        }
        std::vector<std::int64_t> expected;
        if (!must_ids.empty()) {
            for (std::int64_t doc = 0; doc <= 60; ++doc) {
                const auto in = [&](int term) { return lists[static_cast<std::size_t>(term)].contains(doc); };
                if (std::all_of(must_ids.begin(), must_ids.end(), in) &&
                    std::none_of(must_not_ids.begin(), must_not_ids.end(), in)) {
                    expected.push_back(doc);
                }
            }
        }
        REQUIRE_EQ(index.query(must, must_not), expected);
    }
}

// Linear scans cost 16 billion steps here; galloping the shortest list costs almost nothing.
PERF(4, "AND over huge lists only walks the shortest list", 800) {
    std::vector<std::int64_t> huge(1'000'000);
    std::iota(huge.begin(), huge.end(), 0);

    PostingIndex index;
    std::vector<std::string> must = {"rare"};
    index.add("rare", {5, 500'000, 999'999});
    for (int term = 0; term < 8; ++term) {
        must.push_back("huge" + std::to_string(term));
        index.add(must.back(), huge);
    }
    std::size_t total = 0;
    for (int round = 0; round < 2'000; ++round) {
        total += index.query(must).size();
    }
    CHECK_EQ(total, 6'000);
}
