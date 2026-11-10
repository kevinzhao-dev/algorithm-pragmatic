#include <algorithm>
#include <array>
#include <set>
#include <vector>

#include "01_bench.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

const std::array<LoopOrder, 6> kAllOrders = {LoopOrder::IJK, LoopOrder::IKJ, LoopOrder::JIK,
                                             LoopOrder::JKI, LoopOrder::KIJ, LoopOrder::KJI};

using Triple = std::array<std::size_t, 3>;

std::vector<Triple> expected_triples(std::size_t rows, std::size_t cols, std::size_t depth,
                                     LoopOrder order) {
    // (i, j, k) nested in the order the enum spells; index into `sizes` by axis.
    const std::array<std::size_t, 3> sizes = {rows, cols, depth};
    std::array<std::size_t, 3> axes{};  // position -> axis
    switch (order) {
        case LoopOrder::IJK: axes = {0, 1, 2}; break;
        case LoopOrder::IKJ: axes = {0, 2, 1}; break;
        case LoopOrder::JIK: axes = {1, 0, 2}; break;
        case LoopOrder::JKI: axes = {1, 2, 0}; break;
        case LoopOrder::KIJ: axes = {2, 0, 1}; break;
        case LoopOrder::KJI: axes = {2, 1, 0}; break;
    }
    std::vector<Triple> out;
    std::array<std::size_t, 3> index{};
    for (index[0] = 0; index[0] < sizes[axes[0]]; ++index[0]) {
        for (index[1] = 0; index[1] < sizes[axes[1]]; ++index[1]) {
            for (index[2] = 0; index[2] < sizes[axes[2]]; ++index[2]) {
                Triple triple{};
                for (int position = 0; position < 3; ++position) {
                    triple[axes[static_cast<std::size_t>(position)]] =
                        index[static_cast<std::size_t>(position)];
                }
                out.push_back(triple);
            }
        }
    }
    return out;
}

Matrix random_matrix(Rng& rng, std::size_t rows, std::size_t cols) {
    Matrix m(rows, cols);
    for (std::size_t r = 0; r < rows; ++r) {
        for (std::size_t c = 0; c < cols; ++c) {
            m(r, c) = static_cast<double>(rng.uniform(-5, 5));
        }
    }
    return m;
}

Matrix naive_product(const Matrix& a, const Matrix& b) {
    Matrix c(a.rows(), b.cols());
    for (std::size_t i = 0; i < a.rows(); ++i) {
        for (std::size_t j = 0; j < b.cols(); ++j) {
            double sum = 0;
            for (std::size_t k = 0; k < a.cols(); ++k) {
                sum += a(i, k) * b(k, j);
            }
            c(i, j) = sum;
        }
    }
    return c;
}

[[clang::optnone]] void multiply_unoptimized(const Matrix& a, const Matrix& b, Matrix& c) {
    for (std::size_t i = 0; i < a.rows(); ++i) {
        for (std::size_t k = 0; k < a.cols(); ++k) {
            for (std::size_t j = 0; j < b.cols(); ++j) {
                c(i, j) += a(i, k) * b(k, j);
            }
        }
    }
}

}  // namespace

TEST(1, "a new matrix is zero-filled with the right shape") {
    const Matrix m(2, 3);
    CHECK_EQ(m.rows(), 2);
    CHECK_EQ(m.cols(), 3);
    for (std::size_t r = 0; r < 2; ++r) {
        for (std::size_t c = 0; c < 3; ++c) {
            CHECK_EQ(m(r, c), 0.0);
        }
    }
}

TEST(1, "elements are stored row-major and row() exposes them") {
    Matrix m(2, 3);
    m(1, 2) = 7.5;
    m(0, 1) = 2.0;
    const auto row = m.row(1);
    CHECK_EQ(row.size(), 3);
    CHECK_EQ(row[2], 7.5);
    CHECK_EQ(&m(1, 0), m.row(1).data());
    CHECK_EQ(&m(1, 2) - &m(0, 0), 5);
    const Matrix& view = m;
    CHECK_EQ(view.row(0)[1], 2.0);
}

TEST(1, "equality compares shape and contents") {
    Matrix a(1, 2);
    Matrix b(2, 1);
    Matrix c(1, 2);
    CHECK(!(a == b));
    CHECK(a == c);
    c(0, 1) = 1;
    CHECK(!(a == c));
}

TEST(1, "multiply computes a known product") {
    Matrix a(2, 2);
    a(0, 0) = 1; a(0, 1) = 2;
    a(1, 0) = 3; a(1, 1) = 4;
    Matrix b(2, 2);
    b(0, 0) = 5; b(0, 1) = 6;
    b(1, 0) = 7; b(1, 1) = 8;
    Matrix expected(2, 2);
    expected(0, 0) = 19; expected(0, 1) = 22;
    expected(1, 0) = 43; expected(1, 1) = 50;
    CHECK(multiply(a, b) == expected);
}

TEST(1, "mismatched dimensions are rejected") {
    const Matrix a(2, 3);
    const Matrix b(2, 3);
    CHECK_THROWS_AS(multiply(a, b), std::invalid_argument);
}

TEST(1, "empty dimensions produce empty results") {
    const Matrix a(0, 3);
    const Matrix b(3, 2);
    const Matrix c = multiply(a, b);
    CHECK_EQ(c.rows(), 0);
    CHECK_EQ(c.cols(), 2);
    const Matrix d(2, 0);
    const Matrix e(0, 5);
    const Matrix f = multiply(d, e);
    CHECK_EQ(f.rows(), 2);
    CHECK_EQ(f.cols(), 5);
    CHECK(f == Matrix(2, 5));
}

TEST(1, "the result of multiply does not depend on prior contents") {
    Rng rng;
    const Matrix a = random_matrix(rng, 5, 7);
    const Matrix b = random_matrix(rng, 7, 3);
    CHECK(multiply(a, b) == naive_product(a, b));
}

TEST(2, "every order visits exactly the expected triples, in order") {
    for (const LoopOrder order : kAllOrders) {
        const auto expected = expected_triples(2, 3, 2, order);
        std::vector<Triple> seen;
        for_each_index_triple(2, 3, 2, order, [&](std::size_t i, std::size_t j, std::size_t k) {
            seen.push_back({i, j, k});
        });
        INFO("order ", static_cast<int>(order));
        REQUIRE_EQ(seen.size(), expected.size());
        CHECK(seen == expected);
    }
}

TEST(2, "orders that disagree on the inner loop produce different triples") {
    std::vector<Triple> ijk;
    std::vector<Triple> ikj;
    for_each_index_triple(2, 2, 2, LoopOrder::IJK,
                          [&](std::size_t i, std::size_t j, std::size_t k) { ijk.push_back({i, j, k}); });
    for_each_index_triple(2, 2, 2, LoopOrder::IKJ,
                          [&](std::size_t i, std::size_t j, std::size_t k) { ikj.push_back({i, j, k}); });
    CHECK(ijk != ikj);
}

TEST(2, "all six orders agree on random rectangular products") {
    Rng rng;
    for (int round = 0; round < 80; ++round) {
        const auto rows = static_cast<std::size_t>(rng.uniform(1, 12));
        const auto depth = static_cast<std::size_t>(rng.uniform(1, 12));
        const auto cols = static_cast<std::size_t>(rng.uniform(1, 12));
        const Matrix a = random_matrix(rng, rows, depth);
        const Matrix b = random_matrix(rng, depth, cols);
        const Matrix expected = naive_product(a, b);
        for (const LoopOrder order : kAllOrders) {
            INFO("round ", round, " order ", static_cast<int>(order));
            CHECK(multiply_ordered(a, b, order) == expected);
        }
    }
}

TEST(2, "multiply_ordered rejects mismatched dimensions") {
    const Matrix a(2, 3);
    const Matrix b(4, 2);
    CHECK_THROWS_AS(multiply_ordered(a, b, LoopOrder::IKJ), std::invalid_argument);
}

TEST(3, "the ranking is a permutation of all six orders") {
    const std::vector<LoopOrder> ranking = rank_by_cache_misses(64, 64, 64, 8);
    REQUIRE_EQ(ranking.size(), 6);
    CHECK_EQ(std::set<LoopOrder>(ranking.begin(), ranking.end()).size(), 6);
}

TEST(3, "orders fall into the three classes the model predicts") {
    const std::vector<LoopOrder> ranking = rank_by_cache_misses(128, 64, 64, 8);
    REQUIRE_EQ(ranking.size(), 6);
    const std::set<LoopOrder> best(ranking.begin(), ranking.begin() + 2);
    const std::set<LoopOrder> middle(ranking.begin() + 2, ranking.begin() + 4);
    const std::set<LoopOrder> worst(ranking.begin() + 4, ranking.end());
    CHECK(best == std::set<LoopOrder>{LoopOrder::IKJ, LoopOrder::KIJ});
    CHECK(middle == std::set<LoopOrder>{LoopOrder::IJK, LoopOrder::JIK});
    CHECK(worst == std::set<LoopOrder>{LoopOrder::JKI, LoopOrder::KJI});
}

PERF(4, "IKJ beats IJK by a wide margin in a real run", 10000) {
    Rng rng;
    const Matrix a = random_matrix(rng, 512, 512);
    const Matrix b = random_matrix(rng, 512, 512);
    const double ratio = bench::speedup(
        3,
        [&] {
            auto c = multiply_ordered(a, b, LoopOrder::IJK);
            bench::do_not_optimize(c(1, 2));
        },
        [&] {
            auto c = multiply_ordered(a, b, LoopOrder::IKJ);
            bench::do_not_optimize(c(1, 2));
        });
    INFO("IJK/IKJ speedup ", ratio);
    CHECK_GE(ratio, 4.0);
}

PERF(4, "compiler optimization is worth an order of magnitude", 10000) {
    Rng rng;
    const Matrix a = random_matrix(rng, 512, 512);
    const Matrix b = random_matrix(rng, 512, 512);
    const double ratio = bench::speedup(
        3,
        [&] {
            Matrix c(512, 512);
            multiply_unoptimized(a, b, c);
            bench::do_not_optimize(c(1, 2));
        },
        [&] {
            auto c = multiply_ordered(a, b, LoopOrder::IKJ);
            bench::do_not_optimize(c(1, 2));
        });
    INFO("optnone/optimized speedup ", ratio);
    CHECK_GE(ratio, 4.0);
}
