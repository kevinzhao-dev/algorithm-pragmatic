#include <limits>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;

namespace {

// McIlroy's adaptive adversary: elements stay undecided ("gas") until a comparison forces one
// of them to become the next smallest value. Its answers are consistent with a total order, so
// any algorithm with a worst-case bound must honour that bound here, but a pivot rule that only
// looks good on fixed inputs does not.
class Adversary {
public:
    static void reset(int n) {
        values().assign(static_cast<std::size_t>(n), kGas);
        frozen() = 0;
        candidate() = 0;
        comparisons() = 0;
    }

    explicit Adversary(int id) : id_(id) {}

    friend bool operator<(const Adversary& a, const Adversary& b) {
        ++comparisons();
        auto& val = values();
        const auto x = static_cast<std::size_t>(a.id_);
        const auto y = static_cast<std::size_t>(b.id_);
        if (val[x] == kGas && val[y] == kGas) {
            if (a.id_ == candidate()) {
                val[x] = frozen()++;
            } else {
                val[y] = frozen()++;
            }
        }
        if (val[x] == kGas) {
            candidate() = a.id_;
        } else if (val[y] == kGas) {
            candidate() = b.id_;
        }
        return val[x] < val[y];
    }

    int value() const { return values()[static_cast<std::size_t>(id_)]; }
    static bool is_gas(int value) { return value == kGas; }
    static int frozen_count() { return frozen(); }
    static std::uint64_t comparison_count() { return comparisons(); }

private:
    static constexpr int kGas = std::numeric_limits<int>::max();

    static std::vector<int>& values() {
        static std::vector<int> storage;
        return storage;
    }
    static int& frozen() {
        static int count = 0;
        return count;
    }
    static int& candidate() {
        static int id = 0;
        return id;
    }
    static std::uint64_t& comparisons() {
        static std::uint64_t count = 0;
        return count;
    }

    int id_;
};

std::vector<Counted<int>> to_counted(const std::vector<int>& values) {
    std::vector<Counted<int>> out;
    out.reserve(values.size());
    for (int value : values) {
        out.emplace_back(value);
    }
    return out;
}

std::vector<int> sorted_copy(std::vector<int> values) {
    std::ranges::sort(values);
    return values;
}

std::vector<int> shape(const std::string& name, std::size_t n, Rng& rng) {
    std::vector<int> values(n);
    for (std::size_t i = 0; i < n; ++i) {
        const int position = static_cast<int>(i);
        if (name == "sorted") {
            values[i] = position;
        } else if (name == "reversed") {
            values[i] = static_cast<int>(n) - position;
        } else if (name == "organ pipe") {
            values[i] = std::min(position, static_cast<int>(n) - position);
        } else if (name == "equal") {
            values[i] = 42;
        } else if (name == "few distinct") {
            values[i] = position % 3;
        } else {
            values[i] = static_cast<int>(rng.uniform(0, 1'000'000));
        }
    }
    return values;
}

std::uint64_t comparisons_per_element_scaled(std::size_t n) {
    Rng rng;
    auto values = to_counted(shape("random", n, rng));
    Counted<int>::comparisons() = 0;
    select_kth<Counted<int>>(values, n / 2);
    return Counted<int>::comparisons();
}

}  // namespace

TEST(1, "partition3 separates less, equal and greater") {
    std::vector<int> values{5, 1, 7, 3, 3, 9, 3, 0};
    const auto [less_end, greater_begin] = partition3<int>(values, 3);
    CHECK_EQ(less_end, 2);
    CHECK_EQ(greater_begin, 5);
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i < less_end) {
            CHECK_LT(values[i], 3);
        } else if (i < greater_begin) {
            CHECK_EQ(values[i], 3);
        } else {
            CHECK_GT(values[i], 3);
        }
    }
}

TEST(1, "partition3 keeps every element") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        INFO("trial ", trial);
        auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 40)), 0, 6);
        const auto expected = sorted_copy(values);
        const int pivot = static_cast<int>(rng.uniform(0, 6));
        const auto [less_end, greater_begin] = partition3<int>(values, pivot);
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i < less_end) {
                CHECK_LT(values[i], pivot);
            } else if (i < greater_begin) {
                CHECK_EQ(values[i], pivot);
            } else {
                CHECK_GT(values[i], pivot);
            }
        }
        CHECK_EQ(sorted_copy(values), expected);
    }
}

TEST(1, "partition3 handles an empty range and a pivot outside the values") {
    std::vector<int> empty;
    const auto bounds = partition3<int>(empty, 3);
    CHECK_EQ(bounds.first, 0);
    CHECK_EQ(bounds.second, 0);
    std::vector<int> values{1, 2, 3};
    CHECK_EQ(partition3<int>(values, 10).first, 3);
    CHECK_EQ(partition3<int>(values, -1).second, 0);
}

TEST(1, "partition3 uses at most two comparisons per element") {
    Rng rng;
    auto values = to_counted(rng.ints(10'000, 0, 50));
    Counted<int>::comparisons() = 0;
    partition3<Counted<int>>(values, Counted<int>(25));
    CHECK_LE(Counted<int>::comparisons(), 20'000);
}

TEST(2, "out of range k throws") {
    std::vector<int> values{1, 2, 3};
    CHECK_THROWS_AS(select_kth<int>(values, 3), std::out_of_range);
    std::vector<int> empty;
    CHECK_THROWS_AS(select_kth<int>(empty, 0), std::out_of_range);
}

TEST(2, "a single element is its own median") {
    std::vector<int> values{7};
    CHECK_EQ(select_kth<int>(values, 0), 7);
}

TEST(2, "every rank of a small array is found") {
    const std::vector<int> original{9, 1, 8, 2, 7, 3, 6, 4, 5, 0, 11, 10, 13, 12};
    for (std::size_t k = 0; k < original.size(); ++k) {
        auto values = original;
        CHECK_EQ(select_kth<int>(values, k), static_cast<int>(k));
    }
}

TEST(2, "duplicates count by rank") {
    for (std::size_t k = 0; k < 7; ++k) {
        std::vector<int> values{2, 2, 1, 2, 3, 2, 2};
        const std::vector<int> expected{1, 2, 2, 2, 2, 2, 3};
        CHECK_EQ(select_kth<int>(values, k), expected[k]);
    }
}

TEST(2, "non-trivial element types survive the shuffling") {
    Rng rng;
    std::vector<std::string> words;
    for (int i = 0; i < 2000; ++i) {
        words.push_back(rng.text(static_cast<std::size_t>(rng.uniform(1, 30)), "abc"));
    }
    auto expected = words;
    std::ranges::sort(expected);
    auto copy = words;
    CHECK_EQ(select_kth<std::string>(copy, 1234), expected[1234]);
    std::ranges::sort(copy);
    CHECK_EQ(copy, expected);
}

TEST(2, "random arrays match sorting") {
    Rng rng;
    for (int trial = 0; trial < 3000; ++trial) {
        INFO("trial ", trial);
        auto values = rng.ints(static_cast<std::size_t>(rng.uniform(1, 80)), 0, rng.coin() ? 5 : 1000);
        const auto expected = sorted_copy(values);
        const auto k = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(values.size()) - 1));
        CHECK_EQ(select_kth<int>(values, k), expected[k]);
    }
}

TEST(2, "comparisons stay linear on structured inputs") {
    Rng rng;
    const std::size_t n = 1 << 16;
    for (const std::string name : {"sorted", "reversed", "organ pipe", "equal", "few distinct", "random"}) {
        INFO("shape ", name);
        for (std::size_t k : {std::size_t{0}, n / 2, n - 1}) {
            auto values = to_counted(shape(name, n, rng));
            Counted<int>::comparisons() = 0;
            select_kth<Counted<int>>(values, k);
            CHECK_LE(Counted<int>::comparisons(), 60 * n);
        }
    }
}

TEST(2, "comparisons per element do not grow with n") {
    const double small = static_cast<double>(comparisons_per_element_scaled(1 << 10)) / (1 << 10);
    const double large = static_cast<double>(comparisons_per_element_scaled(1 << 20)) / (1 << 20);
    INFO("small ", small, " large ", large);
    CHECK_LE(large, 1.5 * small);
}

TEST(2, "the bound survives an adaptive adversary") {
    const int n = 20'000;
    for (int k : {0, n / 4, n / 2, n - 1}) {
        INFO("k ", k);
        Adversary::reset(n);
        std::vector<Adversary> values;
        values.reserve(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            values.emplace_back(i);
        }
        const Adversary answer = select_kth<Adversary>(values, static_cast<std::size_t>(k));
        CHECK_LE(Adversary::comparison_count(), 60ULL * static_cast<std::uint64_t>(n));
        if (Adversary::is_gas(answer.value())) {
            CHECK_LE(Adversary::frozen_count(), k);
        } else {
            CHECK_EQ(answer.value(), k);
        }
    }
}

PERF(2, "selecting the median of five million values", 3000) {
    Rng rng;
    auto values = rng.ints(5'000'000, 0, 1'000'000'000);
    const std::size_t k = values.size() / 2;
    const int answer = select_kth<int>(values, k);
    std::ranges::nth_element(values, values.begin() + static_cast<std::ptrdiff_t>(k));
    CHECK_EQ(answer, values[k]);
}

TEST(3, "invalid inputs are rejected") {
    CHECK_THROWS_AS(weighted_median({}), std::invalid_argument);
    CHECK_THROWS_AS(weighted_median(std::vector<Sample>{{1, 0}}), std::invalid_argument);
    CHECK_THROWS_AS(weighted_median(std::vector<Sample>{{1, 1}, {2, -3}}), std::invalid_argument);
}

TEST(3, "a single sample is the median") {
    CHECK_EQ(weighted_median(std::vector<Sample>{{7, 100}}), 7);
}

TEST(3, "a heavy sample pulls the median toward it") {
    CHECK_EQ(weighted_median(std::vector<Sample>{{1, 1}, {2, 1}, {3, 1}, {100, 10}}), 100);
}

TEST(3, "an exact tie picks the smaller value") {
    CHECK_EQ(weighted_median(std::vector<Sample>{{5, 3}, {9, 3}}), 5);
}

TEST(3, "repeated values pool their weight") {
    CHECK_EQ(weighted_median(std::vector<Sample>{{1, 5}, {2, 2}, {2, 2}, {2, 2}, {3, 1}}), 2);
}

TEST(3, "weights near the int64 limit sum without overflow") {
    const std::int64_t big = 4'000'000'000'000'000'000;
    CHECK_EQ(weighted_median(std::vector<Sample>{{1, big}, {2, big}}), 1);
    CHECK_EQ(weighted_median(std::vector<Sample>{{-5, 1}, {2, big}}), 2);
}

TEST(3, "random samples match sorting") {
    Rng rng;
    for (int trial = 0; trial < 2000; ++trial) {
        INFO("trial ", trial);
        std::vector<Sample> samples(static_cast<std::size_t>(rng.uniform(1, 60)));
        for (Sample& s : samples) {
            s = {rng.uniform(-20, 20), rng.uniform(1, 9)};
        }
        auto sorted = samples;
        std::ranges::sort(sorted, {}, &Sample::value);
        std::int64_t total = 0;
        for (const Sample& s : samples) {
            total += s.weight;
        }
        std::int64_t cumulative = 0;
        std::int64_t expected = 0;
        for (const Sample& s : sorted) {
            cumulative += s.weight;
            if (2 * cumulative >= total) {
                expected = s.value;
                break;
            }
        }
        CHECK_EQ(weighted_median(samples), expected);
    }
}

PERF(3, "weighted median of five million samples", 3000) {
    Rng rng;
    std::vector<Sample> samples(5'000'000);
    for (Sample& s : samples) {
        s = {rng.uniform(0, 1'000'000'000), rng.uniform(1, 1000)};
    }
    CHECK_GE(weighted_median(samples), 0);
}
