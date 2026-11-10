#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <queue>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr double kRelative = 1e-6;
const double kInfinity = std::numeric_limits<double>::infinity();

void check_close(double got, double want, const char* name) {
    INFO(name, " got ", got, " want ", want);
    if (std::isinf(want)) {
        CHECK(std::isinf(got));
        return;
    }
    CHECK_LE(std::abs(got - want), kRelative * std::max(1.0, std::abs(want)));
}

Measurement m(int p, double t) { return Measurement{p, t}; }

// A greedy simulator over a random unit-time DAG, to produce measurements a real run could give.
struct Dag {
    std::size_t n = 0;
    std::vector<std::vector<std::size_t>> successors;
    std::vector<std::size_t> in_degree;

    int span() const {
        std::vector<int> longest(n, 1);
        std::vector<std::size_t> pending = in_degree;
        std::vector<std::size_t> stack;
        for (std::size_t v = 0; v < n; ++v) {
            if (pending[v] == 0) {
                stack.push_back(v);
            }
        }
        int best = 0;
        while (!stack.empty()) {
            const std::size_t v = stack.back();
            stack.pop_back();
            best = std::max(best, longest[v]);
            for (const std::size_t w : successors[v]) {
                longest[w] = std::max(longest[w], longest[v] + 1);
                if (--pending[w] == 0) {
                    stack.push_back(w);
                }
            }
        }
        return best;
    }

    int run(int processors) const {
        std::vector<std::size_t> pending = in_degree;
        std::vector<std::size_t> ready;
        for (std::size_t v = 0; v < n; ++v) {
            if (pending[v] == 0) {
                ready.push_back(v);
            }
        }
        int steps = 0;
        while (!ready.empty()) {
            ++steps;
            std::vector<std::size_t> running;
            for (int i = 0; i < processors && !ready.empty(); ++i) {
                running.push_back(ready.back());
                ready.pop_back();
            }
            for (const std::size_t v : running) {
                for (const std::size_t w : successors[v]) {
                    if (--pending[w] == 0) {
                        ready.push_back(w);
                    }
                }
            }
        }
        return steps;
    }
};

Dag random_dag(Rng& rng, std::size_t n, double density) {
    Dag dag;
    dag.n = n;
    dag.successors.resize(n);
    dag.in_degree.resize(n, 0);
    for (std::size_t u = 0; u < n; ++u) {
        for (std::size_t v = u + 1; v < n; ++v) {
            if (rng.coin(density)) {
                dag.successors[u].push_back(v);
                ++dag.in_degree[v];
            }
        }
    }
    return dag;
}

// Scans the feasible polygon slice by slice (fine in y, exact in x) as an independent check.
struct Region {
    bool feasible = false;
    double min_parallelism = kInfinity;
    double max_parallelism = 0;
    double min_work = kInfinity;
    double max_work = 0;
    double max_span = 0;
};

Region scan(std::span<const Measurement> ms) {
    double y_max = kInfinity;
    for (const Measurement& mea : ms) {
        y_max = std::min(y_max, mea.seconds);
    }
    Region region;
    const int steps = 30000;
    for (int s = 0; s <= steps; ++s) {
        const double y = y_max * s / steps;
        double x_lo = y;
        double x_hi = kInfinity;
        for (const Measurement& mea : ms) {
            const auto p = static_cast<double>(mea.processors);
            x_hi = std::min(x_hi, p * mea.seconds);
            x_lo = std::max(x_lo, p * mea.seconds - (p - 1) * y);
        }
        if (x_lo <= x_hi) {
            region.feasible = true;
            region.min_work = std::min(region.min_work, x_lo);
            region.max_work = std::max(region.max_work, x_hi);
            region.max_span = std::max(region.max_span, y);
            if (y > 0) {
                region.min_parallelism = std::min(region.min_parallelism, x_lo / y);
                region.max_parallelism = std::max(region.max_parallelism, x_hi / y);
            }
        }
    }
    return region;
}

}  // namespace

TEST(1, "invalid measurements are rejected") {
    CHECK_THROWS_AS(audit_measurements(std::span<const Measurement>()), std::invalid_argument);
    const std::vector<Measurement> bad_p = {m(0, 1.0)};
    CHECK_THROWS_AS(audit_measurements(bad_p), std::invalid_argument);
    const std::vector<Measurement> bad_t = {m(2, 0.0)};
    CHECK_THROWS_AS(audit_measurements(bad_t), std::invalid_argument);
}

TEST(1, "a single run on one processor pins the work exactly") {
    const std::vector<Measurement> ms = {m(1, 80.0)};
    const Audit a = audit_measurements(ms);
    CHECK(a.consistent);
    check_close(a.min_work, 80, "min work");
    check_close(a.max_work, 80, "max work");
    check_close(a.min_span, 0, "min span");
    check_close(a.max_span, 80, "max span");
    check_close(a.min_parallelism, 1, "min parallelism");
    check_close(a.max_parallelism, kInfinity, "max parallelism");
}

TEST(1, "one parallel run leaves wide ranges") {
    const std::vector<Measurement> ms = {m(3, 50.0)};
    const Audit a = audit_measurements(ms);
    CHECK(a.consistent);
    check_close(a.max_work, 150, "max work");
    check_close(a.min_work, 50, "min work");
    check_close(a.max_span, 50, "max span");
    check_close(a.min_span, 0, "min span");
    check_close(a.min_parallelism, 1, "min parallelism");
    check_close(a.max_parallelism, kInfinity, "max parallelism");
}

TEST(1, "two runs give the polygon from the laws") {
    // Vertices (80,20), (100,20), (100,60/7), (90,10).
    const std::vector<Measurement> ms = {m(2, 50.0), m(8, 20.0)};
    const Audit a = audit_measurements(ms);
    CHECK(a.consistent);
    check_close(a.min_work, 80, "min work");
    check_close(a.max_work, 100, "max work");
    check_close(a.min_span, 60.0 / 7.0, "min span");
    check_close(a.max_span, 20, "max span");
    check_close(a.min_parallelism, 4, "min parallelism");
    check_close(a.max_parallelism, 35.0 / 3.0, "max parallelism");
}

TEST(1, "a serial run plus a parallel run bound the span") {
    const std::vector<Measurement> ms = {m(1, 100.0), m(4, 30.0)};
    const Audit a = audit_measurements(ms);
    CHECK(a.consistent);
    check_close(a.min_work, 100, "min work");
    check_close(a.max_work, 100, "max work");
    check_close(a.min_span, 20.0 / 3.0, "min span");
    check_close(a.max_span, 30, "max span");
    check_close(a.min_parallelism, 100.0 / 30.0, "min parallelism");
    check_close(a.max_parallelism, 15, "max parallelism");
}

TEST(1, "impossible triples are detected") {
    const std::vector<Measurement> ms = {m(2, 60.0), m(5, 33.0), m(20, 9.0)};
    CHECK(!audit_measurements(ms).consistent);
}

TEST(1, "an impossible pair is detected") {
    // 4 processors cannot be slower than 2 processors allow, by the greedy bound.
    const std::vector<Measurement> ms = {m(1, 100.0), m(2, 10.0)};
    CHECK(!audit_measurements(ms).consistent);
}

TEST(1, "measurements from a real schedule are always consistent") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 120));
        const Dag dag = random_dag(rng, n, rng.real() * 0.1);
        const auto work = static_cast<double>(n);
        const auto span = static_cast<double>(dag.span());
        std::vector<Measurement> ms;
        for (const int p : {1, 2, 4, 8, 16}) {
            if (rng.coin(0.7)) {
                ms.push_back(m(p, static_cast<double>(dag.run(p))));
            }
        }
        if (ms.empty()) {
            ms.push_back(m(3, static_cast<double>(dag.run(3))));
        }
        const Audit a = audit_measurements(ms);
        INFO("round ", round, " work ", work, " span ", span);
        REQUIRE(a.consistent);
        CHECK_GE(work, a.min_work * (1 - kRelative) - kRelative);
        CHECK_LE(work, a.max_work * (1 + kRelative) + kRelative);
        CHECK_GE(span, a.min_span * (1 - kRelative) - kRelative);
        CHECK_LE(span, a.max_span * (1 + kRelative) + kRelative);
        const double parallelism = work / span;
        CHECK_GE(parallelism, a.min_parallelism * (1 - 1e-6));
        if (!std::isinf(a.max_parallelism)) {
            CHECK_LE(parallelism, a.max_parallelism * (1 + 1e-6));
        }
    }
}

TEST(1, "random instances agree with a slice-by-slice scan") {
    Rng rng;
    int consistent_seen = 0;
    for (int round = 0; round < 400; ++round) {
        const auto count = static_cast<std::size_t>(rng.uniform(1, 5));
        std::vector<Measurement> ms;
        for (std::size_t i = 0; i < count; ++i) {
            ms.push_back(m(static_cast<int>(rng.uniform(1, 32)), static_cast<double>(rng.uniform(1, 500))));
        }
        const Audit a = audit_measurements(ms);
        const Region r = scan(ms);
        INFO("round ", round);
        CHECK_EQ(a.consistent, r.feasible);
        if (a.consistent && r.feasible) {
            ++consistent_seen;
            CHECK_LE(std::abs(a.max_work - r.max_work), 0.01 * std::max(1.0, r.max_work));
            CHECK_LE(std::abs(a.min_work - r.min_work), 0.01 * std::max(1.0, r.min_work));
            CHECK_LE(std::abs(a.max_span - r.max_span), 0.01 * std::max(1.0, r.max_span));
        }
    }
    CHECK_GT(consistent_seen, 20);
}

TEST(2, "no conflict when everything is consistent") {
    const std::vector<Measurement> ms = {m(2, 50.0), m(8, 20.0)};
    CHECK(minimal_conflict(ms).empty());
    CHECK(minimal_conflict(std::vector<Measurement>{m(3, 9.0)}).empty());
}

TEST(2, "a contradictory pair is returned as the smallest witness") {
    const std::vector<Measurement> ms = {m(4, 30.0), m(2, 52.0), m(1, 30.0), m(8, 15.0)};
    CHECK_EQ(minimal_conflict(ms), std::vector<std::size_t>({1, 2}));
}

TEST(2, "a contradictory triple is found when no pair conflicts") {
    const std::vector<Measurement> ms = {m(2, 60.0), m(5, 33.0), m(20, 9.0)};
    // Every pair alone is possible; only all three together are not.
    CHECK_EQ(minimal_conflict(ms), std::vector<std::size_t>({0, 1, 2}));
}

TEST(2, "the witness is always minimal and lexicographically smallest") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const auto count = static_cast<std::size_t>(rng.uniform(1, 7));
        std::vector<Measurement> ms;
        for (std::size_t i = 0; i < count; ++i) {
            ms.push_back(m(static_cast<int>(rng.uniform(1, 16)), static_cast<double>(rng.uniform(1, 80))));
        }
        const std::vector<std::size_t> witness = minimal_conflict(ms);
        INFO("round ", round);
        if (audit_measurements(ms).consistent) {
            CHECK(witness.empty());
            continue;
        }
        REQUIRE(!witness.empty());
        REQUIRE(witness.size() <= 3);
        std::vector<Measurement> chosen;
        for (const std::size_t i : witness) {
            chosen.push_back(ms[i]);
        }
        CHECK(!audit_measurements(chosen).consistent);
        for (std::size_t drop = 0; drop < chosen.size(); ++drop) {
            std::vector<Measurement> smaller = chosen;
            smaller.erase(smaller.begin() + static_cast<std::ptrdiff_t>(drop));
            CHECK(smaller.empty() || audit_measurements(smaller).consistent);
        }
        // No lexicographically smaller witness of the same size exists.
        for (std::size_t mask = 0; mask < (std::size_t{1} << ms.size()); ++mask) {
            std::vector<std::size_t> indices;
            std::vector<Measurement> subset;
            for (std::size_t i = 0; i < ms.size(); ++i) {
                if ((mask >> i) & 1) {
                    indices.push_back(i);
                    subset.push_back(ms[i]);
                }
            }
            if (!subset.empty() && indices.size() <= witness.size() && indices < witness) {
                if (indices.size() < witness.size() || !audit_measurements(subset).consistent) {
                    const bool bad = indices.size() == witness.size() &&
                                     !audit_measurements(subset).consistent;
                    CHECK(!bad);
                }
            }
        }
    }
}

TEST(3, "the runtime prediction brackets a known case") {
    const std::vector<Measurement> ms = {m(2, 50.0), m(8, 20.0)};
    const RuntimeBounds b = predict_runtime(ms, 4);
    check_close(b.low, 20, "low");
    check_close(b.high, 40, "high");
}

TEST(3, "one processor means exactly the work") {
    const std::vector<Measurement> ms = {m(1, 100.0), m(4, 30.0)};
    const RuntimeBounds b = predict_runtime(ms, 1);
    check_close(b.low, 100, "low");
    check_close(b.high, 100, "high");
}

TEST(3, "inconsistent measurements are refused") {
    const std::vector<Measurement> ms = {m(1, 100.0), m(2, 10.0)};
    CHECK_THROWS_AS(predict_runtime(ms, 4), std::domain_error);
}

TEST(3, "predictions contain what a real schedule would do") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(2, 100));
        const Dag dag = random_dag(rng, n, rng.real() * 0.1);
        std::vector<Measurement> ms;
        for (const int p : {2, 4, 16}) {
            ms.push_back(m(p, static_cast<double>(dag.run(p))));
        }
        const int target = static_cast<int>(rng.uniform(1, 64));
        const RuntimeBounds b = predict_runtime(ms, target);
        const auto actual = static_cast<double>(dag.run(target));
        INFO("round ", round, " target ", target, " actual ", actual, " low ", b.low, " high ", b.high);
        CHECK_LE(b.low, b.high + 1e-6);
        CHECK_LE(b.low, actual * (1 + 1e-6));
        CHECK_GE(b.high * (1 + 1e-6), actual);
    }
}

TEST(3, "predictions agree with a direct scan of the region") {
    Rng rng;
    int checked = 0;
    for (int round = 0; round < 200; ++round) {
        const auto count = static_cast<std::size_t>(rng.uniform(1, 4));
        std::vector<Measurement> ms;
        for (std::size_t i = 0; i < count; ++i) {
            ms.push_back(m(static_cast<int>(rng.uniform(1, 16)), static_cast<double>(rng.uniform(5, 300))));
        }
        if (!audit_measurements(ms).consistent) {
            continue;
        }
        const int p = static_cast<int>(rng.uniform(1, 32));
        const RuntimeBounds b = predict_runtime(ms, p);
        double y_max = kInfinity;
        for (const Measurement& mea : ms) {
            y_max = std::min(y_max, mea.seconds);
        }
        double low = kInfinity;
        double high = 0;
        const auto consider = [&](double x, double y) {
            low = std::min(low, std::max(x / p, y));
            high = std::max(high, (x - y) / p + y);
        };
        const int steps = 20000;
        for (int s = 0; s <= steps; ++s) {
            const double y = y_max * s / steps;
            double x_lo = y;
            double x_hi = kInfinity;
            for (const Measurement& mea : ms) {
                const auto q = static_cast<double>(mea.processors);
                x_hi = std::min(x_hi, q * mea.seconds);
                x_lo = std::max(x_lo, q * mea.seconds - (q - 1) * y);
            }
            if (x_lo <= x_hi) {
                consider(x_lo, y);
                consider(x_hi, y);
                const double crossing = p * y;
                if (crossing >= x_lo && crossing <= x_hi) {
                    consider(crossing, y);
                }
            }
        }
        INFO("round ", round, " p ", p);
        CHECK_LE(std::abs(b.low - low), 0.01 * std::max(1.0, low));
        CHECK_LE(std::abs(b.high - high), 0.01 * std::max(1.0, high));
        ++checked;
    }
    CHECK_GT(checked, 20);
}
