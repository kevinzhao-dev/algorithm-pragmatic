#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

// Four-by-four matrices over the integers mod 2^64: exact, associative and not commutative.
struct Mat4 {
    std::array<std::uint64_t, 16> cells{};

    static Mat4 identity() {
        Mat4 result;
        for (std::size_t i = 0; i < 4; ++i) {
            result.cells[i * 5] = 1;
        }
        return result;
    }

    friend Mat4 operator*(const Mat4& a, const Mat4& b) {
        Mat4 result;
        for (std::size_t i = 0; i < 4; ++i) {
            for (std::size_t j = 0; j < 4; ++j) {
                std::uint64_t sum = 0;
                for (std::size_t k = 0; k < 4; ++k) {
                    sum += a.cells[i * 4 + k] * b.cells[k * 4 + j];
                }
                result.cells[i * 4 + j] = sum;
            }
        }
        return result;
    }

    friend bool operator==(const Mat4&, const Mat4&) = default;
};

Mat4 random_matrix(Rng& rng) {
    Mat4 result;
    for (auto& cell : result.cells) {
        cell = rng.engine()();
    }
    return result;
}

// Wraps Mat4 to count how often the chain multiplies, so bounds are deterministic.
struct CountedMat {
    Mat4 value = Mat4::identity();
    static inline std::uint64_t products = 0;

    static CountedMat identity() { return {Mat4::identity()}; }
    friend CountedMat operator*(const CountedMat& a, const CountedMat& b) {
        ++products;
        return {a.value * b.value};
    }
    friend bool operator==(const CountedMat&, const CountedMat&) = default;
};

Mat4 brute_product(const std::vector<Mat4>& joints, std::size_t left, std::size_t right) {
    Mat4 result = Mat4::identity();
    for (std::size_t i = left; i < right; ++i) {
        result = result * joints[i];
    }
    return result;
}

int ceil_lg(std::size_t n) {
    int bits = 0;
    while ((std::size_t{1} << bits) < n) {
        ++bits;
    }
    return bits;
}

using Chain = TransformChain<Mat4>;

}  // namespace

TEST(1, "builds the full product in order") {
    Rng rng;
    std::vector<Mat4> joints;
    for (int i = 0; i < 40; ++i) {
        joints.push_back(random_matrix(rng));
    }
    const Chain chain(joints);
    CHECK_EQ(chain.size(), 40);
    CHECK(chain.full_transformation() == brute_product(joints, 0, joints.size()));
}

TEST(1, "an empty chain is the identity") {
    const Chain chain(std::vector<Mat4>{});
    CHECK_EQ(chain.size(), 0);
    CHECK(chain.full_transformation() == Mat4::identity());
}

TEST(1, "a single joint is itself") {
    Rng rng;
    const Mat4 only = random_matrix(rng);
    const Chain chain(std::vector<Mat4>{only});
    CHECK(chain.full_transformation() == only);
}

TEST(1, "non-commutative multiplication keeps the left-to-right order") {
    Rng rng;
    const Mat4 a = random_matrix(rng);
    const Mat4 b = random_matrix(rng);
    const Chain chain(std::vector<Mat4>{a, b});
    CHECK(chain.full_transformation() == a * b);
    CHECK(!(chain.full_transformation() == b * a));
}

TEST(1, "update replaces one joint") {
    Rng rng;
    std::vector<Mat4> joints{random_matrix(rng), random_matrix(rng), random_matrix(rng)};
    Chain chain(joints);
    const Mat4 replacement = random_matrix(rng);
    chain.update_joint(1, replacement);
    joints[1] = replacement;
    CHECK(chain.full_transformation() == brute_product(joints, 0, 3));
}

TEST(1, "out-of-range updates throw") {
    Rng rng;
    Chain chain(std::vector<Mat4>{random_matrix(rng)});
    CHECK_THROWS_AS(chain.update_joint(1, Mat4::identity()), std::out_of_range);
}

TEST(1, "random updates match a brute-force product") {
    Rng rng;
    const std::size_t n = 200;
    std::vector<Mat4> joints;
    for (std::size_t i = 0; i < n; ++i) {
        joints.push_back(random_matrix(rng));
    }
    Chain chain(joints);
    for (int step = 0; step < 4000; ++step) {
        INFO("step ", step);
        const auto k = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 1));
        joints[k] = random_matrix(rng);
        chain.update_joint(k, joints[k]);
        REQUIRE(chain.full_transformation() == brute_product(joints, 0, n));
    }
}

TEST(1, "build and update cost O(n) and O(log n) multiplications") {
    Rng rng;
    const std::size_t n = 1 << 16;
    std::vector<CountedMat> joints(n);
    CountedMat::products = 0;
    TransformChain<CountedMat> chain(joints);
    CHECK_LE(CountedMat::products, static_cast<std::uint64_t>(3 * n));

    CountedMat::products = 0;
    const int updates = 20'000;
    for (int i = 0; i < updates; ++i) {
        chain.update_joint(static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 1)), CountedMat{});
    }
    CHECK_LE(CountedMat::products, static_cast<std::uint64_t>(updates * (4 * ceil_lg(n) + 8)));
}

TEST(1, "reading the full transformation performs no multiplications") {
    std::vector<CountedMat> joints(1000);
    TransformChain<CountedMat> chain(joints);
    CountedMat::products = 0;
    for (int i = 0; i < 100'000; ++i) {
        (void)chain.full_transformation();
    }
    CHECK_EQ(CountedMat::products, 0);
}

PERF(1, "200k updates on a long arm", 2500) {
    Rng rng;
    const std::size_t n = 200'000;
    std::vector<Mat4> joints;
    joints.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        joints.push_back(random_matrix(rng));
    }
    Chain chain(joints);
    for (int i = 0; i < 200'000; ++i) {
        chain.update_joint(static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 1)), random_matrix(rng));
    }
    CHECK_EQ(chain.size(), n);
}

TEST(2, "segments multiply exactly their joints, left to right") {
    Rng rng;
    std::vector<Mat4> joints;
    for (int i = 0; i < 30; ++i) {
        joints.push_back(random_matrix(rng));
    }
    const Chain chain(joints);
    CHECK(chain.segment(0, 30) == brute_product(joints, 0, 30));
    CHECK(chain.segment(5, 6) == joints[5]);
    CHECK(chain.segment(10, 25) == brute_product(joints, 10, 25));
    CHECK(chain.segment(7, 7) == Mat4::identity());
    CHECK(chain.segment(30, 30) == Mat4::identity());
}

TEST(2, "invalid ranges throw") {
    Rng rng;
    const Chain chain(std::vector<Mat4>{random_matrix(rng), random_matrix(rng)});
    CHECK_THROWS_AS(chain.segment(0, 3), std::out_of_range);
    CHECK_THROWS_AS(chain.segment(2, 1), std::out_of_range);
}

TEST(2, "random segments match brute force under updates") {
    Rng rng;
    const std::size_t n = 300;
    std::vector<Mat4> joints;
    for (std::size_t i = 0; i < n; ++i) {
        joints.push_back(random_matrix(rng));
    }
    Chain chain(joints);
    for (int step = 0; step < 3000; ++step) {
        INFO("step ", step);
        if (rng.coin(0.3)) {
            const auto k = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 1));
            joints[k] = random_matrix(rng);
            chain.update_joint(k, joints[k]);
        }
        const auto l = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n)));
        const auto r = static_cast<std::size_t>(rng.uniform(static_cast<std::int64_t>(l), static_cast<std::int64_t>(n)));
        REQUIRE(chain.segment(l, r) == brute_product(joints, l, r));
    }
}

TEST(2, "segment queries use O(log n) multiplications") {
    Rng rng;
    const std::size_t n = 1 << 16;
    std::vector<CountedMat> joints(n);
    TransformChain<CountedMat> chain(joints);
    CountedMat::products = 0;
    const int queries = 5000;
    for (int i = 0; i < queries; ++i) {
        const auto l = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) / 2));
        const auto r = static_cast<std::size_t>(rng.uniform(static_cast<std::int64_t>(n) / 2, static_cast<std::int64_t>(n)));
        (void)chain.segment(l, r);
    }
    CHECK_LE(CountedMat::products, static_cast<std::uint64_t>(queries * (8 * ceil_lg(n) + 16)));
}

TEST(3, "inserting and removing joints changes the arm") {
    Rng rng;
    const Mat4 a = random_matrix(rng);
    const Mat4 b = random_matrix(rng);
    const Mat4 c = random_matrix(rng);
    Chain chain(std::vector<Mat4>{a, c});
    chain.insert_joint(1, b);
    CHECK_EQ(chain.size(), 3);
    CHECK(chain.full_transformation() == a * b * c);
    CHECK(chain.remove_joint(0) == a);
    CHECK(chain.full_transformation() == b * c);
    chain.insert_joint(2, a);
    CHECK(chain.full_transformation() == b * c * a);
    CHECK_THROWS_AS(chain.insert_joint(4, a), std::out_of_range);
    CHECK_THROWS_AS(chain.remove_joint(3), std::out_of_range);
}

TEST(3, "removing every joint leaves the identity") {
    Rng rng;
    Chain chain(std::vector<Mat4>{random_matrix(rng), random_matrix(rng)});
    chain.remove_joint(1);
    chain.remove_joint(0);
    CHECK_EQ(chain.size(), 0);
    CHECK(chain.full_transformation() == Mat4::identity());
    chain.insert_joint(0, Mat4::identity());
    CHECK_EQ(chain.size(), 1);
}

TEST(3, "random structural edits match a model arm") {
    Rng rng;
    std::vector<Mat4> joints;
    Chain chain(joints);
    for (int step = 0; step < 6000; ++step) {
        INFO("step ", step);
        const auto action = rng.uniform(0, 3);
        const auto n = static_cast<std::int64_t>(joints.size());
        if (joints.empty() || action == 0) {
            const auto k = static_cast<std::size_t>(rng.uniform(0, n));
            const Mat4 m = random_matrix(rng);
            chain.insert_joint(k, m);
            joints.insert(joints.begin() + static_cast<std::ptrdiff_t>(k), m);
        } else if (action == 1) {
            const auto k = static_cast<std::size_t>(rng.uniform(0, n - 1));
            REQUIRE(chain.remove_joint(k) == joints[k]);
            joints.erase(joints.begin() + static_cast<std::ptrdiff_t>(k));
        } else if (action == 2) {
            const auto k = static_cast<std::size_t>(rng.uniform(0, n - 1));
            joints[k] = random_matrix(rng);
            chain.update_joint(k, joints[k]);
        } else {
            const auto l = static_cast<std::size_t>(rng.uniform(0, n));
            const auto r = static_cast<std::size_t>(rng.uniform(static_cast<std::int64_t>(l), n));
            REQUIRE(chain.segment(l, r) == brute_product(joints, l, r));
        }
        REQUIRE_EQ(chain.size(), joints.size());
        REQUIRE(chain.full_transformation() == brute_product(joints, 0, joints.size()));
    }
}

TEST(3, "structural edits use O(log n) multiplications") {
    Rng rng;
    const std::size_t n = 1 << 16;
    std::vector<CountedMat> joints(n);
    TransformChain<CountedMat> chain(joints);
    CountedMat::products = 0;
    const int edits = 20'000;
    for (int i = 0; i < edits; ++i) {
        const auto k = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n)));
        chain.insert_joint(k, CountedMat{});
        chain.remove_joint(k);
    }
    CHECK_LE(CountedMat::products, static_cast<std::uint64_t>(edits * (24 * ceil_lg(n) + 48)));
    CHECK_EQ(chain.size(), n);
}

PERF(3, "structural edits on a long arm", 2500) {
    Rng rng;
    const std::size_t n = 200'000;
    std::vector<Mat4> joints;
    joints.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        joints.push_back(random_matrix(rng));
    }
    Chain chain(joints);
    for (int i = 0; i < 150'000; ++i) {
        const auto size = static_cast<std::int64_t>(chain.size());
        chain.insert_joint(static_cast<std::size_t>(rng.uniform(0, size)), random_matrix(rng));
        chain.remove_joint(static_cast<std::size_t>(rng.uniform(0, size)));
    }
    CHECK_EQ(chain.size(), n);
}
