#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr std::size_t idx(int value) { return static_cast<std::size_t>(value); }

constexpr std::int64_t INF = kUnreachable;

struct WEdge {
    int u;
    int v;
    std::int64_t w;
};

WeightedGraph make_graph(int n, const std::vector<WEdge>& edges) {
    WeightedGraph g(idx(n));
    for (const WEdge& e : edges) {
        g[idx(e.u)].push_back({e.v, e.w});
    }
    return g;
}

// Relaxes every edge until stable; fine as an oracle for graphs without negative cycles.
std::vector<std::int64_t> brute_dist(int n, const std::vector<WEdge>& edges, int source) {
    std::vector<std::int64_t> dist(idx(n), INF);
    dist[idx(source)] = 0;
    for (int round = 0; round <= n; ++round) {
        for (const WEdge& e : edges) {
            if (dist[idx(e.u)] != INF && dist[idx(e.u)] + e.w < dist[idx(e.v)]) {
                dist[idx(e.v)] = dist[idx(e.u)] + e.w;
            }
        }
    }
    return dist;
}

std::vector<WEdge> random_dag(Rng& rng, int n, int m, std::int64_t low, std::int64_t high) {
    std::vector<int> order = rng.permutation(idx(n));
    std::vector<WEdge> edges;
    for (int i = 0; i < m && n >= 2; ++i) {
        int a = static_cast<int>(rng.uniform(0, n - 1));
        int b = static_cast<int>(rng.uniform(0, n - 1));
        if (a == b) {
            continue;
        }
        if (a > b) {
            std::swap(a, b);
        }
        edges.push_back({order[idx(a)], order[idx(b)], rng.uniform(low, high)});
    }
    return edges;
}

// Non-negative weights rewritten through random potentials: negative edges appear but every
// cycle keeps its non-negative total, so shortest paths stay well defined.
std::vector<WEdge> random_general_graph(Rng& rng, int n, int m, std::int64_t max_weight) {
    std::vector<std::int64_t> potential(idx(n));
    for (auto& h : potential) {
        h = rng.uniform(-30, 30);
    }
    std::vector<WEdge> edges;
    for (int i = 0; i < m; ++i) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        const int v = static_cast<int>(rng.uniform(0, n - 1));
        const std::int64_t base = rng.coin(0.3) ? 0 : rng.uniform(0, max_weight);
        edges.push_back({u, v, base + potential[idx(u)] - potential[idx(v)]});
    }
    return edges;
}

void check_tree(int n, const std::vector<WEdge>& edges, int source,
                const std::vector<std::int64_t>& dist, const std::vector<int>& parent) {
    REQUIRE_EQ(parent.size(), n);
    CHECK_EQ(parent[idx(source)], kNoParent);
    for (int v = 0; v < n; ++v) {
        INFO("vertex ", v);
        if (dist[idx(v)] == INF || v == source) {
            if (v != source) {
                CHECK_EQ(parent[idx(v)], kNoParent);
            }
            continue;
        }
        const int p = parent[idx(v)];
        REQUIRE(p >= 0 && p < n);
        bool tight = false;
        for (const WEdge& e : edges) {
            if (e.u == p && e.v == v && dist[idx(p)] != INF && dist[idx(p)] + e.w == dist[idx(v)]) {
                tight = true;
                break;
            }
        }
        CHECK(tight);
        int walk = v;
        int steps = 0;
        while (walk != source && steps <= n) {
            walk = parent[idx(walk)];
            REQUIRE(walk != kNoParent);
            ++steps;
        }
        CHECK_EQ(walk, source);
    }
}

// Longest-chain oracle: enumerates every path through the job DAG.
struct BruteSchedule {
    std::int64_t makespan = 0;
    std::vector<std::int64_t> earliest;
    std::vector<std::int64_t> through;  // longest chain that contains each job
};

void extend_chain(const std::vector<Job>& jobs, const std::vector<std::vector<int>>& successors,
                  int job, std::vector<int>& chain, std::int64_t length, BruteSchedule& out) {
    chain.push_back(job);
    length += jobs[idx(job)].duration;
    if (successors[idx(job)].empty()) {
        out.makespan = std::max(out.makespan, length);
        for (const int member : chain) {
            out.through[idx(member)] = std::max(out.through[idx(member)], length);
        }
    }
    for (const int next : successors[idx(job)]) {
        extend_chain(jobs, successors, next, chain, length, out);
    }
    chain.pop_back();
}

BruteSchedule brute_schedule(const std::vector<Job>& jobs) {
    const auto n = static_cast<int>(jobs.size());
    BruteSchedule out;
    out.earliest.assign(idx(n), 0);
    out.through.assign(idx(n), 0);
    std::vector<std::vector<int>> successors(idx(n));
    for (int job = 0; job < n; ++job) {
        for (const int dep : jobs[idx(job)].depends_on) {
            successors[idx(dep)].push_back(job);
        }
    }
    for (int job = 0; job < n; ++job) {
        if (jobs[idx(job)].depends_on.empty()) {
            std::vector<int> chain;
            extend_chain(jobs, successors, job, chain, 0, out);
        }
    }
    for (int pass = 0; pass <= n; ++pass) {
        for (int job = 0; job < n; ++job) {
            for (const int dep : jobs[idx(job)].depends_on) {
                out.earliest[idx(job)] = std::max(out.earliest[idx(job)],
                                                  out.earliest[idx(dep)] + jobs[idx(dep)].duration);
            }
        }
    }
    return out;
}

std::vector<Job> random_jobs(Rng& rng, int n, int edges) {
    std::vector<Job> jobs(idx(n));
    for (Job& job : jobs) {
        job.duration = rng.uniform(0, 20);
    }
    std::vector<int> order = rng.permutation(idx(n));
    std::set<std::pair<int, int>> used;
    for (int i = 0; i < edges && n >= 2; ++i) {
        int a = static_cast<int>(rng.uniform(0, n - 1));
        int b = static_cast<int>(rng.uniform(0, n - 1));
        if (a >= b) {
            continue;
        }
        if (used.emplace(a, b).second) {
            jobs[idx(order[idx(b)])].depends_on.push_back(order[idx(a)]);
        }
    }
    return jobs;
}

void check_critical_path(const std::vector<Job>& jobs, const PipelinePlan& plan) {
    if (jobs.empty()) {
        CHECK(plan.critical_path.empty());
        return;
    }
    REQUIRE(!plan.critical_path.empty());
    std::int64_t total = 0;
    for (std::size_t i = 0; i < plan.critical_path.size(); ++i) {
        const int job = plan.critical_path[i];
        REQUIRE(job >= 0 && job < static_cast<int>(jobs.size()));
        total += jobs[idx(job)].duration;
        if (i > 0) {
            const auto& deps = jobs[idx(job)].depends_on;
            CHECK(std::find(deps.begin(), deps.end(), plan.critical_path[i - 1]) != deps.end());
        }
    }
    CHECK_EQ(total, plan.makespan);
    CHECK_EQ(plan.earliest_start[idx(plan.critical_path.front())], 0);
    const int last = plan.critical_path.back();
    CHECK_EQ(plan.earliest_start[idx(last)] + jobs[idx(last)].duration, plan.makespan);
}

}  // namespace

TEST(1, "a single vertex is at distance zero from itself") {
    const WeightedGraph g(1);
    CHECK_EQ(dag_shortest_paths(g, 0), std::vector<std::int64_t>{0});
}

TEST(1, "a bad source throws out_of_range") {
    const WeightedGraph g(1);
    CHECK_THROWS_AS(dag_shortest_paths(g, 1), std::out_of_range);
    CHECK_THROWS_AS(dag_shortest_paths(g, -1), std::out_of_range);
}

TEST(1, "unreachable vertices stay at kUnreachable") {
    const WeightedGraph g = make_graph(3, {{0, 1, 5}});
    CHECK_EQ(dag_shortest_paths(g, 0), std::vector<std::int64_t>{0, 5, INF});
    CHECK_EQ(dag_shortest_paths(g, 2), std::vector<std::int64_t>{INF, INF, 0});
}

TEST(1, "the lightest of several paths is chosen") {
    const WeightedGraph g = make_graph(4, {{0, 1, 4}, {0, 2, 1}, {2, 1, 2}, {1, 3, 1}, {2, 3, 7}});
    CHECK_EQ(dag_shortest_paths(g, 0), std::vector<std::int64_t>{0, 3, 1, 4});
}

TEST(1, "negative edges are handled") {
    const WeightedGraph g = make_graph(4, {{0, 1, 5}, {0, 2, 2}, {1, 3, -10}, {2, 3, -1}});
    CHECK_EQ(dag_shortest_paths(g, 0), std::vector<std::int64_t>{0, 5, 2, -5});
}

TEST(1, "unreachable vertices are not relaxed through (no kUnreachable overflow)") {
    const WeightedGraph g = make_graph(4, {{1, 2, -100}, {2, 3, -100}, {0, 3, 7}});
    CHECK_EQ(dag_shortest_paths(g, 0), std::vector<std::int64_t>{0, INF, INF, 7});
}

TEST(1, "parallel edges use the lightest one") {
    const WeightedGraph g = make_graph(2, {{0, 1, 9}, {0, 1, 3}, {0, 1, 6}});
    CHECK_EQ(dag_shortest_paths(g, 0), std::vector<std::int64_t>{0, 3});
}

TEST(1, "any directed cycle is rejected, even one the source cannot reach") {
    const WeightedGraph reachable = make_graph(3, {{0, 1, 1}, {1, 2, 1}, {2, 1, 1}});
    CHECK_THROWS_AS(dag_shortest_paths(reachable, 0), std::invalid_argument);
    const WeightedGraph elsewhere = make_graph(4, {{0, 1, 1}, {2, 3, 1}, {3, 2, 1}});
    CHECK_THROWS_AS(dag_shortest_paths(elsewhere, 0), std::invalid_argument);
    const WeightedGraph self_loop = make_graph(2, {{1, 1, 0}});
    CHECK_THROWS_AS(dag_shortest_paths(self_loop, 0), std::invalid_argument);
}

TEST(1, "random DAGs match repeated relaxation") {
    Rng rng;
    for (int round = 0; round < 1500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 25));
        const auto edges = random_dag(rng, n, static_cast<int>(rng.uniform(0, 4 * n)), -50, 50);
        const WeightedGraph g = make_graph(n, edges);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round, " n=", n, " s=", s);
        REQUIRE_EQ(dag_shortest_paths(g, s), brute_dist(n, edges, s));
    }
}

// Edge order is hostile, so round-based relaxation needs about 1,000,000 rounds here.
PERF(1, "long chains with shuffled labels are handled in linear time", 1500) {
    Rng rng;
    const int n = 1'000'000;
    const std::vector<int> label = rng.permutation(idx(n));
    WeightedGraph g(idx(n));
    for (int i = 0; i + 1 < n; ++i) {
        g[idx(label[idx(i)])].push_back({label[idx(i + 1)], static_cast<std::int64_t>(i % 7) - 3});
    }
    for (int i = 0; i + 40 < n; i += 3) {
        g[idx(label[idx(i)])].push_back({label[idx(i + 40)], rng.uniform(-100, 100)});
    }
    const auto dist = dag_shortest_paths(g, label[0]);
    CHECK_NE(dist[idx(label[idx(n - 1)])], INF);
}

TEST(2, "the source and unreachable vertices have no parent") {
    const WeightedGraph g(2);
    CHECK_EQ(shortest_path_tree(g, 0, std::vector<std::int64_t>{0, INF}), std::vector<int>{kNoParent, kNoParent});
}

TEST(2, "a tree is built from known distances") {
    const std::vector<WEdge> edges = {{0, 1, 4}, {0, 2, 1}, {2, 1, 2}, {1, 3, 1}};
    const WeightedGraph g = make_graph(4, edges);
    const std::vector<std::int64_t> dist = {0, 3, 1, 4};
    const auto parent = shortest_path_tree(g, 0, dist);
    check_tree(4, edges, 0, dist, parent);
    CHECK_EQ(parent, std::vector<int>{kNoParent, 2, 0, 1});
}

TEST(2, "negative edges and cycles of positive weight are fine") {
    const std::vector<WEdge> edges = {{0, 1, 5}, {1, 2, -3}, {2, 1, 4}, {2, 3, 1}};
    const WeightedGraph g = make_graph(4, edges);
    const std::vector<std::int64_t> dist = {0, 5, 2, 3};
    check_tree(4, edges, 0, dist, shortest_path_tree(g, 0, dist));
}

TEST(2, "a zero-weight cycle does not trap the parent pointers") {
    // 1 and 2 reach each other for free; picking any tight incoming edge can close a parent loop.
    const std::vector<WEdge> edges = {{0, 1, 3}, {1, 2, 0}, {2, 1, 0}, {2, 3, 1}, {1, 3, 2}};
    const WeightedGraph g = make_graph(4, edges);
    const std::vector<std::int64_t> dist = {0, 3, 3, 4};
    check_tree(4, edges, 0, dist, shortest_path_tree(g, 0, dist));
}

TEST(2, "a larger zero-weight loop stays acyclic") {
    const std::vector<WEdge> edges = {{0, 1, 0}, {1, 2, 0}, {2, 3, 0}, {3, 1, 0}, {3, 4, 2}, {4, 5, -1}, {5, 3, 1}};
    const WeightedGraph g = make_graph(6, edges);
    const std::vector<std::int64_t> dist = {0, 0, 0, 0, 2, 1};
    check_tree(6, edges, 0, dist, shortest_path_tree(g, 0, dist));
}

TEST(2, "random graphs produce valid trees") {
    Rng rng;
    for (int round = 0; round < 1500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 16));
        const auto edges = random_general_graph(rng, n, static_cast<int>(rng.uniform(0, 3 * n)), 12);
        const WeightedGraph g = make_graph(n, edges);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        const auto dist = brute_dist(n, edges, s);
        INFO("round ", round);
        check_tree(n, edges, s, dist, shortest_path_tree(g, s, dist));
    }
}

PERF(2, "tree construction is linear", 1500) {
    Rng rng;
    const int n = 500'000;
    WeightedGraph g(idx(n));
    std::vector<std::int64_t> dist(idx(n));
    for (int i = 0; i < n; ++i) {
        dist[idx(i)] = i;
    }
    for (int i = 0; i + 1 < n; ++i) {
        g[idx(i)].push_back({i + 1, 1});
    }
    for (int i = 0; i < 1'000'000; ++i) {
        const int a = static_cast<int>(rng.uniform(0, n - 1));
        const int b = static_cast<int>(rng.uniform(0, n - 1));
        g[idx(a)].push_back({b, b - a + rng.uniform(1, 50)});
    }
    const auto parent = shortest_path_tree(g, 0, dist);
    CHECK_EQ(parent[idx(n - 1)], n - 2);
}

TEST(3, "no jobs is an empty plan") {
    const PipelinePlan plan = plan_pipeline({});
    CHECK_EQ(plan.makespan, 0);
    CHECK(plan.earliest_start.empty());
    CHECK(plan.critical_path.empty());
}

TEST(3, "independent jobs run in parallel") {
    const std::vector<Job> jobs = {{3, {}}, {7, {}}};
    const PipelinePlan plan = plan_pipeline(jobs);
    CHECK_EQ(plan.makespan, 7);
    CHECK_EQ(plan.earliest_start, std::vector<std::int64_t>{0, 0});
    CHECK_EQ(plan.slack, std::vector<std::int64_t>{4, 0});
    CHECK_EQ(plan.critical_path, std::vector<int>{1});
}

TEST(3, "a chain's jobs are all critical") {
    const std::vector<Job> jobs = {{2, {}}, {3, {0}}, {4, {1}}};
    const PipelinePlan plan = plan_pipeline(jobs);
    CHECK_EQ(plan.makespan, 9);
    CHECK_EQ(plan.earliest_start, std::vector<std::int64_t>{0, 2, 5});
    CHECK_EQ(plan.slack, std::vector<std::int64_t>{0, 0, 0});
    CHECK_EQ(plan.critical_path, std::vector<int>{0, 1, 2});
}

TEST(3, "a side branch has slack but the critical path does not") {
    // 0 -> 2 is long, 1 -> 2 is short; the side job can slip by the difference.
    const std::vector<Job> jobs = {{10, {}}, {4, {}}, {1, {0, 1}}};
    const PipelinePlan plan = plan_pipeline(jobs);
    CHECK_EQ(plan.makespan, 11);
    CHECK_EQ(plan.earliest_start, std::vector<std::int64_t>{0, 0, 10});
    CHECK_EQ(plan.slack, std::vector<std::int64_t>{0, 6, 0});
    CHECK_EQ(plan.critical_path, std::vector<int>{0, 2});
}

TEST(3, "bad inputs are rejected") {
    const std::vector<Job> cycle = {{1, {1}}, {1, {0}}};
    CHECK_THROWS_AS(plan_pipeline(cycle), std::invalid_argument);
    const std::vector<Job> self = {{1, {0}}};
    CHECK_THROWS_AS(plan_pipeline(self), std::invalid_argument);
    const std::vector<Job> unknown = {{1, {3}}};
    CHECK_THROWS_AS(plan_pipeline(unknown), std::out_of_range);
}

TEST(3, "zero-duration jobs are allowed") {
    const std::vector<Job> jobs = {{0, {}}, {0, {0}}, {5, {1}}};
    const PipelinePlan plan = plan_pipeline(jobs);
    CHECK_EQ(plan.makespan, 5);
    check_critical_path(jobs, plan);
}

TEST(3, "random pipelines match path enumeration") {
    Rng rng;
    for (int round = 0; round < 1200; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 11));
        const std::vector<Job> jobs = random_jobs(rng, n, static_cast<int>(rng.uniform(0, 2 * n)));
        INFO("round ", round);
        const BruteSchedule expected = brute_schedule(jobs);
        const PipelinePlan plan = plan_pipeline(jobs);
        CHECK_EQ(plan.makespan, expected.makespan);
        CHECK_EQ(plan.earliest_start, expected.earliest);
        std::vector<std::int64_t> slack;
        for (int j = 0; j < n; ++j) {
            slack.push_back(expected.makespan - expected.through[idx(j)]);
        }
        CHECK_EQ(plan.slack, slack);
        check_critical_path(jobs, plan);
    }
}

PERF(3, "a million jobs are planned in linear time", 1500) {
    Rng rng;
    const int n = 1'000'000;
    std::vector<Job> jobs(idx(n));
    for (int i = 0; i < n; ++i) {
        jobs[idx(i)].duration = rng.uniform(0, 100);
        if (i > 0) {
            jobs[idx(i)].depends_on.push_back(i - 1);
        }
        if (i > 3) {
            jobs[idx(i)].depends_on.push_back(i - 3);
        }
    }
    const PipelinePlan plan = plan_pipeline(jobs);
    check_critical_path(jobs, plan);
}
