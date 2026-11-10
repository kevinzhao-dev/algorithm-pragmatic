#include <algorithm>
#include <bit>
#include <numeric>
#include <queue>
#include <random>
#include <vector>

#include "01_bench.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

std::size_t ceil_lg(std::size_t n) {
    return n <= 1 ? 0 : static_cast<std::size_t>(std::bit_width(n - 1));
}

// Random weighted digraph with no negative cycles, built with Johnson-style potentials.
std::vector<std::int64_t> random_graph(Rng& rng, std::size_t n, double density) {
    std::vector<std::int64_t> potential(n);
    for (auto& p : potential) {
        p = rng.uniform(-30, 30);
    }
    std::vector<std::int64_t> dist(n * n, kNoEdge);
    for (std::size_t i = 0; i < n; ++i) {
        dist[i * n + i] = 0;
        for (std::size_t j = 0; j < n; ++j) {
            if (i != j && rng.coin(density)) {
                dist[i * n + j] = rng.uniform(0, 50) + potential[i] - potential[j];
            }
        }
    }
    return dist;
}

std::vector<std::int64_t> serial_answer(std::vector<std::int64_t> dist, std::size_t n) {
    for (std::size_t k = 0; k < n; ++k) {
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                if (dist[i * n + k] != kNoEdge && dist[k * n + j] != kNoEdge &&
                    dist[i * n + k] + dist[k * n + j] < dist[i * n + j]) {
                    dist[i * n + j] = dist[i * n + k] + dist[k * n + j];
                }
            }
        }
    }
    return dist;
}

std::vector<std::uint64_t> make_rows(std::size_t n, const std::vector<std::pair<std::size_t, std::size_t>>& edges) {
    const std::size_t words = (n + 63) / 64;
    std::vector<std::uint64_t> rows(n * words, 0);
    for (const auto& [u, v] : edges) {
        rows[u * words + v / 64] |= std::uint64_t{1} << (v % 64);
    }
    return rows;
}

bool bit(const std::vector<std::uint64_t>& rows, std::size_t words, std::size_t i, std::size_t j) {
    return (rows[i * words + j / 64] >> (j % 64)) & 1;
}

std::vector<char> reachable_by_bfs(std::size_t n, const std::vector<std::vector<std::size_t>>& adjacency,
                                   std::size_t source) {
    std::vector<char> seen(n, 0);
    std::queue<std::size_t> queue;
    for (const std::size_t v : adjacency[source]) {
        if (seen[v] == 0) {
            seen[v] = 1;
            queue.push(v);
        }
    }
    while (!queue.empty()) {
        const std::size_t u = queue.front();
        queue.pop();
        for (const std::size_t v : adjacency[u]) {
            if (seen[v] == 0) {
                seen[v] = 1;
                queue.push(v);
            }
        }
    }
    return seen;
}

[[gnu::noinline]] void naive_closure(std::vector<char>& reach, std::size_t n) {
    for (std::size_t k = 0; k < n; ++k) {
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                if (reach[i * n + k] != 0 && reach[k * n + j] != 0) {
                    reach[i * n + j] = 1;
                }
            }
        }
    }
}

}  // namespace

TEST(1, "a small graph by hand") {
    std::vector<std::int64_t> dist = {
        0, 3, kNoEdge,
        kNoEdge, 0, -2,
        4, kNoEdge, 0,
    };
    exec::Meter meter;
    floyd_warshall(meter, std::span<std::int64_t>(dist), 3);
    CHECK_EQ(dist, std::vector<std::int64_t>({0, 3, 1, 2, 0, -2, 4, 7, 0}));
}

TEST(1, "unreachable pairs stay unreachable") {
    std::vector<std::int64_t> dist = {0, kNoEdge, kNoEdge, 0};
    exec::Meter meter;
    floyd_warshall(meter, std::span<std::int64_t>(dist), 2);
    CHECK_EQ(dist[1], kNoEdge);
    CHECK_EQ(dist[2], kNoEdge);
}

TEST(1, "negative edges without negative cycles") {
    std::vector<std::int64_t> dist = {
        0, -1, 10,
        kNoEdge, 0, -5,
        kNoEdge, kNoEdge, 0,
    };
    exec::Meter meter;
    floyd_warshall(meter, std::span<std::int64_t>(dist), 3);
    CHECK_EQ(dist[2], -6);
}

TEST(1, "agrees with a serial run on random graphs") {
    Rng rng;
    for (int round = 0; round < 80; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 40));
        std::vector<std::int64_t> dist = random_graph(rng, n, rng.real());
        const std::vector<std::int64_t> expected = serial_answer(dist, n);
        exec::Meter meter;
        floyd_warshall(meter, std::span<std::int64_t>(dist), n);
        INFO("round ", round, " n ", n);
        REQUIRE_EQ(dist, expected);
    }
}

TEST(1, "work is exact and span is n lg n") {
    Rng rng;
    for (const std::size_t n : {2, 3, 8, 16, 33, 64}) {
        std::vector<std::int64_t> dist = random_graph(rng, n, 0.5);
        exec::Meter meter;
        floyd_warshall(meter, std::span<std::int64_t>(dist), n);
        INFO("n ", n, " span ", meter.span());
        CHECK_EQ(meter.work() - meter.spawns(), static_cast<std::int64_t>(n * (n - 1) * (n - 1)));
        CHECK_LE(meter.span(), static_cast<std::int64_t>(n * (2 * ceil_lg(n) + 6)));
    }
}

TEST(1, "no race on the real pool") {
    Rng rng;
    exec::ForkJoin pool;
    for (const unsigned workers : {1U, 4U, 8U}) {
        practice::set_workers(workers);
        const std::size_t n = 90;
        std::vector<std::int64_t> dist = random_graph(rng, n, 0.3);
        const std::vector<std::int64_t> expected = serial_answer(dist, n);
        floyd_warshall(pool, std::span<std::int64_t>(dist), n);
        INFO("workers ", workers);
        CHECK_EQ(dist, expected);
    }
}

TEST(2, "next pointers start from the edges") {
    std::vector<std::int64_t> dist = {0, 3, kNoEdge, 0};
    std::vector<std::int64_t> next(4, 7);
    exec::Meter meter;
    floyd_warshall_paths(meter, std::span<std::int64_t>(dist), std::span<std::int64_t>(next), 2);
    CHECK_EQ(next, std::vector<std::int64_t>({-1, 1, -1, -1}));
}

TEST(2, "paths are reconstructed through intermediates") {
    std::vector<std::int64_t> dist = {
        0, 1, kNoEdge, 10,
        kNoEdge, 0, 1, kNoEdge,
        kNoEdge, kNoEdge, 0, 1,
        kNoEdge, kNoEdge, kNoEdge, 0,
    };
    std::vector<std::int64_t> next(16);
    exec::Meter meter;
    floyd_warshall_paths(meter, std::span<std::int64_t>(dist), std::span<std::int64_t>(next), 4);
    CHECK_EQ(dist[3], 3);
    CHECK_EQ(reconstruct_path(next, 4, 0, 3), std::vector<std::size_t>({0, 1, 2, 3}));
    CHECK_EQ(reconstruct_path(next, 4, 1, 1), std::vector<std::size_t>({1}));
    CHECK(reconstruct_path(next, 4, 3, 0).empty());
}

TEST(2, "every reconstructed path is real and has the shortest length") {
    Rng rng;
    for (int round = 0; round < 40; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 25));
        const std::vector<std::int64_t> original = random_graph(rng, n, rng.real());
        std::vector<std::int64_t> dist = original;
        std::vector<std::int64_t> next(n * n);
        exec::Meter meter;
        floyd_warshall_paths(meter, std::span<std::int64_t>(dist), std::span<std::int64_t>(next), n);
        for (std::size_t s = 0; s < n; ++s) {
            for (std::size_t t = 0; t < n; ++t) {
                const auto path = reconstruct_path(next, n, s, t);
                INFO("round ", round, " s ", s, " t ", t);
                if (s == t) {
                    CHECK_EQ(path, std::vector<std::size_t>({s}));
                } else if (dist[s * n + t] == kNoEdge) {
                    CHECK(path.empty());
                } else {
                    REQUIRE_EQ(path.front(), s);
                    REQUIRE_EQ(path.back(), t);
                    std::int64_t total = 0;
                    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
                        const std::int64_t weight = original[path[i] * n + path[i + 1]];
                        REQUIRE(weight != kNoEdge);
                        total += weight;
                    }
                    CHECK_EQ(total, dist[s * n + t]);
                }
            }
        }
    }
}

TEST(2, "no race while maintaining next pointers") {
    Rng rng;
    exec::ForkJoin pool;
    practice::set_workers(8);
    const std::size_t n = 80;
    std::vector<std::int64_t> dist = random_graph(rng, n, 0.4);
    const std::vector<std::int64_t> expected = serial_answer(dist, n);
    std::vector<std::int64_t> next(n * n);
    floyd_warshall_paths(pool, std::span<std::int64_t>(dist), std::span<std::int64_t>(next), n);
    CHECK_EQ(dist, expected);
}

TEST(2, "negative cycles are detected") {
    std::vector<std::int64_t> dist = {
        0, 2, kNoEdge,
        kNoEdge, 0, -4,
        1, kNoEdge, 0,
    };
    exec::Meter meter;
    floyd_warshall(meter, std::span<std::int64_t>(dist), 3);
    CHECK(has_negative_cycle(dist, 3));
    Rng rng;
    for (int round = 0; round < 40; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(2, 20));
        std::vector<std::int64_t> graph = random_graph(rng, n, 0.5);
        exec::Meter meter_inner;
        floyd_warshall(meter_inner, std::span<std::int64_t>(graph), n);
        CHECK(!has_negative_cycle(graph, n));
    }
}

TEST(3, "closure follows chains and leaves absent edges clear") {
    std::vector<std::uint64_t> rows = make_rows(3, {{0, 1}, {1, 2}});
    exec::Meter meter;
    transitive_closure(meter, std::span<std::uint64_t>(rows), 3);
    CHECK(bit(rows, 1, 0, 2));
    CHECK(bit(rows, 1, 0, 1));
    CHECK(bit(rows, 1, 1, 2));
    CHECK(!bit(rows, 1, 2, 0));
    CHECK(!bit(rows, 1, 0, 0));  // no cycle returns to 0
}

TEST(3, "a cycle reaches itself") {
    std::vector<std::uint64_t> rows = make_rows(3, {{0, 1}, {1, 2}, {2, 0}});
    exec::Meter meter;
    transitive_closure(meter, std::span<std::uint64_t>(rows), 3);
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 3; ++j) {
            CHECK(bit(rows, 1, i, j));
        }
    }
}

TEST(3, "matches BFS on random graphs across word boundaries") {
    Rng rng;
    for (int round = 0; round < 60; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 200));
        std::vector<std::pair<std::size_t, std::size_t>> edges;
        std::vector<std::vector<std::size_t>> adjacency(n);
        for (std::size_t u = 0; u < n; ++u) {
            for (std::size_t v = 0; v < n; ++v) {
                if (rng.coin(0.015)) {
                    edges.emplace_back(u, v);
                    adjacency[u].push_back(v);
                }
            }
        }
        std::vector<std::uint64_t> rows = make_rows(n, edges);
        exec::Meter meter;
        transitive_closure(meter, std::span<std::uint64_t>(rows), n);
        const std::size_t words = (n + 63) / 64;
        for (std::size_t s = 0; s < n; ++s) {
            const std::vector<char> seen = reachable_by_bfs(n, adjacency, s);
            for (std::size_t t = 0; t < n; ++t) {
                INFO("round ", round, " s ", s, " t ", t);
                REQUIRE_EQ(bit(rows, words, s, t), seen[t] != 0);
            }
        }
    }
}

TEST(3, "no race on the real pool") {
    Rng rng;
    exec::ForkJoin pool;
    practice::set_workers(8);
    const std::size_t n = 400;
    std::vector<std::pair<std::size_t, std::size_t>> edges;
    std::vector<std::vector<std::size_t>> adjacency(n);
    for (std::size_t u = 0; u < n; ++u) {
        for (std::size_t v = 0; v < n; ++v) {
            if (rng.coin(0.004)) {
                edges.emplace_back(u, v);
                adjacency[u].push_back(v);
            }
        }
    }
    std::vector<std::uint64_t> rows = make_rows(n, edges);
    transitive_closure(pool, std::span<std::uint64_t>(rows), n);
    const std::size_t words = (n + 63) / 64;
    for (std::size_t s = 0; s < n; ++s) {
        const std::vector<char> seen = reachable_by_bfs(n, adjacency, s);
        for (std::size_t t = 0; t < n; ++t) {
            REQUIRE_EQ(bit(rows, words, s, t), seen[t] != 0);
        }
    }
}

PERF(3, "bitset closure is far faster than cell-by-cell", 12000) {
    const std::size_t n = 512;
    std::mt19937_64 engine(0x616);
    std::vector<std::pair<std::size_t, std::size_t>> edges;
    std::vector<char> cells(n * n, 0);
    for (std::size_t u = 0; u < n; ++u) {
        for (std::size_t v = 0; v < n; ++v) {
            if (engine() % 100 == 0) {
                edges.emplace_back(u, v);
                cells[u * n + v] = 1;
            }
        }
    }
    const std::vector<std::uint64_t> original = make_rows(n, edges);
    practice::set_workers(1);
    exec::ForkJoin pool;
    const double ratio = bench::speedup(
        3,
        [&] {
            std::vector<char> copy = cells;
            naive_closure(copy, n);
            bench::do_not_optimize(copy.data());
        },
        [&] {
            std::vector<std::uint64_t> rows = original;
            transitive_closure(pool, std::span<std::uint64_t>(rows), n);
            bench::do_not_optimize(rows.data());
        });
    INFO("speedup ", ratio);
    CHECK_GE(ratio, 10.0);
}
