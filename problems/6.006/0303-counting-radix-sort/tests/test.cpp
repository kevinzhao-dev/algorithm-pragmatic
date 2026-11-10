#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <tuple>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

// Move-only and not default-constructible: the sort must really move elements around.
class Row {
public:
    Row(std::uint64_t key_value, int id_value)
        : key_(key_value), id_(id_value), payload_(std::make_unique<int>(id_value)) {}
    std::uint64_t key() const { return key_; }
    int id() const { return id_; }
    int payload() const { return *payload_; }

private:
    std::uint64_t key_;
    int id_;
    std::unique_ptr<int> payload_;
};

std::vector<Row> make_rows(const std::vector<std::uint64_t>& keys) {
    std::vector<Row> rows;
    rows.reserve(keys.size());
    for (std::size_t i = 0; i < keys.size(); ++i) {
        rows.emplace_back(keys[i], static_cast<int>(i));
    }
    return rows;
}

// Sorted by key, equal keys keep their original relative order, and nothing got lost.
void check_stable_sorted(std::span<const Row> rows) {
    for (std::size_t i = 0; i < rows.size(); ++i) {
        REQUIRE_EQ(rows[i].id(), rows[i].payload());
        if (i != 0) {
            REQUIRE(rows[i - 1].key() <= rows[i].key());
            if (rows[i - 1].key() == rows[i].key()) {
                REQUIRE(rows[i - 1].id() < rows[i].id());
            }
        }
    }
}

template <typename Sorter>
void sort_and_check_counting(const std::vector<std::uint64_t>& keys, Sorter sorter) {
    auto rows = make_rows(keys);
    sorter(std::span<Row>(rows));
    REQUIRE_EQ(rows.size(), keys.size());
    check_stable_sorted(rows);
    std::vector<std::uint64_t> got;
    for (const Row& row : rows) {
        got.push_back(row.key());
    }
    auto expected = keys;
    std::sort(expected.begin(), expected.end());
    CHECK_EQ(got, expected);
}

std::uint64_t ceil_digits_base_n(std::uint64_t max_key, std::uint64_t n) {
    std::uint64_t digits = 1;
    while (max_key >= n) {
        max_key /= n;
        ++digits;
    }
    return digits;
}

}  // namespace

TEST(1, "sorts small keys") {
    sort_and_check_counting({3, 1, 2, 1, 0}, [](std::span<Row> rows) {
        counting_sort(rows, 4, [](const Row& row) { return static_cast<std::size_t>(row.key()); });
    });
}

TEST(1, "empty and single-element input") {
    std::vector<Row> none;
    counting_sort(std::span<Row>(none), 10, [](const Row& row) { return static_cast<std::size_t>(row.key()); });
    CHECK(none.empty());

    auto one = make_rows({7});
    counting_sort(std::span<Row>(one), 8, [](const Row& row) { return static_cast<std::size_t>(row.key()); });
    CHECK_EQ(one[0].key(), std::uint64_t{7});
}

TEST(1, "equal keys keep their input order") {
    std::vector<std::uint64_t> keys(1000, 2);
    for (std::size_t i = 0; i < keys.size(); i += 3) {
        keys[i] = 1;
    }
    sort_and_check_counting(keys, [](std::span<Row> rows) {
        counting_sort(rows, 3, [](const Row& row) { return static_cast<std::size_t>(row.key()); });
    });
}

TEST(1, "every key can be zero or u - 1") {
    sort_and_check_counting({9, 0, 9, 0, 0, 9}, [](std::span<Row> rows) {
        counting_sort(rows, 10, [](const Row& row) { return static_cast<std::size_t>(row.key()); });
    });
}

TEST(1, "random inputs sort correctly and stably") {
    Rng rng;
    for (int round = 0; round < 50; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(0, 2000));
        const auto u = static_cast<std::size_t>(rng.uniform(1, 500));
        std::vector<std::uint64_t> keys;
        for (std::size_t i = 0; i < n; ++i) {
            keys.push_back(static_cast<std::uint64_t>(rng.uniform(0, static_cast<std::int64_t>(u) - 1)));
        }
        sort_and_check_counting(keys, [u](std::span<Row> rows) {
            counting_sort(rows, u, [](const Row& row) { return static_cast<std::size_t>(row.key()); });
        });
    }
}

TEST(1, "the key function is called a linear number of times") {
    Rng rng;
    const std::size_t n = 50'000;
    std::vector<int> values = rng.ints(n, 0, 99);
    long calls = 0;
    counting_sort(std::span<int>(values), 100, [&](int value) {
        ++calls;
        return static_cast<std::size_t>(value);
    });
    CHECK(std::is_sorted(values.begin(), values.end()));
    CHECK_LE(calls, static_cast<long>(4 * n));
}

PERF(1, "ten million items in a small range", 800) {
    Rng rng;
    std::vector<std::uint32_t> values(10'000'000);
    for (auto& value : values) {
        value = static_cast<std::uint32_t>(rng.engine()() % 1000);
    }
    counting_sort(std::span<std::uint32_t>(values), 1000,
                  [](std::uint32_t value) { return static_cast<std::size_t>(value); });
    CHECK(std::is_sorted(values.begin(), values.end()));
}

TEST(2, "sorts pairs lexicographically") {
    using Point = std::pair<int, int>;
    std::vector<Point> points{{3, 2}, {1, 9}, {3, 1}, {0, 0}, {1, 1}};
    tuple_sort(std::span<Point>(points), 2, 10,
               [](const Point& point, std::size_t column) {
                   return static_cast<std::size_t>(column == 0 ? point.second : point.first);
               });
    CHECK_EQ(points, std::vector<Point>{{0, 0}, {1, 1}, {1, 9}, {3, 1}, {3, 2}});
}

TEST(2, "zero columns leaves the order untouched") {
    auto rows = make_rows({5, 1, 3});
    tuple_sort(std::span<Row>(rows), 0, 4, [](const Row&, std::size_t) { return std::size_t{0}; });
    CHECK_EQ(rows[0].key(), std::uint64_t{5});
    CHECK_EQ(rows[2].key(), std::uint64_t{3});
}

TEST(2, "random tuples match std::stable_sort and stay stable") {
    Rng rng;
    struct Rec {
        int a, b, c, id;
    };
    for (int round = 0; round < 30; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(1, 1500));
        std::vector<Rec> recs;
        for (std::size_t i = 0; i < n; ++i) {
            recs.push_back({static_cast<int>(rng.uniform(0, 9)), static_cast<int>(rng.uniform(0, 3)),
                            static_cast<int>(rng.uniform(0, 5)), static_cast<int>(i)});
        }
        auto expected = recs;
        std::stable_sort(expected.begin(), expected.end(), [](const Rec& x, const Rec& y) {
            return std::tie(x.a, x.b, x.c) < std::tie(y.a, y.b, y.c);
        });
        // Column 0 is c (least significant), then b, then a.
        tuple_sort(std::span<Rec>(recs), 3, 10, [](const Rec& rec, std::size_t column) {
            return static_cast<std::size_t>(column == 0 ? rec.c : column == 1 ? rec.b : rec.a);
        });
        for (std::size_t i = 0; i < n; ++i) {
            REQUIRE_EQ(recs[i].id, expected[i].id);
        }
    }
}

TEST(3, "empty, single and all-equal inputs") {
    std::vector<Row> none;
    radix_sort(std::span<Row>(none), [](const Row& row) { return row.key(); });
    CHECK(none.empty());

    auto one = make_rows({3});
    radix_sort(std::span<Row>(one), [](const Row& row) { return row.key(); });
    CHECK_EQ(one[0].key(), std::uint64_t{3});

    sort_and_check_counting(std::vector<std::uint64_t>(500, 42), [](std::span<Row> rows) {
        radix_sort(rows, [](const Row& row) { return row.key(); });
    });
}

TEST(3, "keys near the top of the 64-bit range") {
    const std::uint64_t top = std::numeric_limits<std::uint64_t>::max();
    sort_and_check_counting({top, 0, top - 1, 1, top, top - 2, 2}, [](std::span<Row> rows) {
        radix_sort(rows, [](const Row& row) { return row.key(); });
    });
}

TEST(3, "two elements still work") {
    sort_and_check_counting({9, 3}, [](std::span<Row> rows) {
        radix_sort(rows, [](const Row& row) { return row.key(); });
    });
}

TEST(3, "random 64-bit keys match std::sort and stay stable") {
    Rng rng;
    for (int round = 0; round < 40; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(0, 3000));
        const int mode = static_cast<int>(rng.uniform(0, 2));
        std::vector<std::uint64_t> keys;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint64_t key = rng.engine()();
            if (mode == 1) {
                key %= 8;  // lots of duplicates
            } else if (mode == 2) {
                key %= static_cast<std::uint64_t>(n) * n + 1;  // polynomial in n
            }
            keys.push_back(key);
        }
        sort_and_check_counting(keys, [](std::span<Row> rows) {
            radix_sort(rows, [](const Row& row) { return row.key(); });
        });
    }
}

// A comparison sort would call key() about 2 n lg n times; radix needs O(n * digits).
TEST(3, "the key function is not called per comparison") {
    Rng rng;
    const std::size_t n = 200'000;
    const std::uint64_t bound = static_cast<std::uint64_t>(n) * n * n;
    std::vector<std::uint64_t> keys(n);
    std::uint64_t max_key = 0;
    for (auto& key : keys) {
        key = rng.engine()() % bound;
        max_key = std::max(max_key, key);
    }
    long calls = 0;
    radix_sort(std::span<std::uint64_t>(keys), [&](std::uint64_t key) {
        ++calls;
        return key;
    });
    CHECK(std::is_sorted(keys.begin(), keys.end()));
    const auto digits = ceil_digits_base_n(max_key, n);
    CHECK_LE(calls, static_cast<long>(n * (5 * digits + 2)));
}

PERF(3, "five million 64-bit keys", 3000) {
    Rng rng;
    std::vector<std::uint64_t> keys(5'000'000);
    for (auto& key : keys) {
        key = rng.engine()();
    }
    radix_sort(std::span<std::uint64_t>(keys), [](std::uint64_t key) { return key; });
    CHECK(std::is_sorted(keys.begin(), keys.end()));
}

TEST(4, "signed sort handles the extremes") {
    std::vector<std::int64_t> values{5, -1, std::numeric_limits<std::int64_t>::min(), 0,
                                     std::numeric_limits<std::int64_t>::max(), -5};
    sort_signed(std::span<std::int64_t>(values));
    CHECK_EQ(values, std::vector<std::int64_t>{std::numeric_limits<std::int64_t>::min(), -5, -1, 0,
                                               5, std::numeric_limits<std::int64_t>::max()});
}

TEST(4, "random signed values match std::sort") {
    Rng rng;
    for (int round = 0; round < 30; ++round) {
        INFO("round ", round);
        std::vector<std::int64_t> values;
        const auto n = static_cast<std::size_t>(rng.uniform(0, 2000));
        for (std::size_t i = 0; i < n; ++i) {
            values.push_back(static_cast<std::int64_t>(rng.engine()()));
        }
        auto expected = values;
        std::sort(expected.begin(), expected.end());
        sort_signed(std::span<std::int64_t>(values));
        CHECK_EQ(values, expected);
    }
}

TEST(4, "equal-length strings sort by byte value") {
    std::vector<std::string> strings{"cab", "abc", "bca", "abb", "~zz", "aaa"};
    sort_equal_length(std::span<std::string>(strings));
    CHECK_EQ(strings, std::vector<std::string>{"aaa", "abb", "abc", "bca", "cab", "~zz"});
}

TEST(4, "empty and zero-length string inputs") {
    std::vector<std::string> none;
    sort_equal_length(std::span<std::string>(none));
    CHECK(none.empty());
    std::vector<std::string> empties{"", "", ""};
    sort_equal_length(std::span<std::string>(empties));
    CHECK_EQ(empties.size(), 3);
}

TEST(4, "bytes above 127 sort after ASCII") {
    std::vector<std::string> strings{"\xff", "a", "\x80"};
    sort_equal_length(std::span<std::string>(strings));
    CHECK_EQ(strings, std::vector<std::string>{"a", "\x80", "\xff"});
}

TEST(4, "random strings match std::sort") {
    Rng rng;
    for (int round = 0; round < 20; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(1, 800));
        const auto length = static_cast<std::size_t>(rng.uniform(1, 12));
        std::vector<std::string> strings;
        for (std::size_t i = 0; i < n; ++i) {
            strings.push_back(rng.text(length, "abcxyz"));
        }
        auto expected = strings;
        std::sort(expected.begin(), expected.end());
        sort_equal_length(std::span<std::string>(strings));
        CHECK_EQ(strings, expected);
    }
}
