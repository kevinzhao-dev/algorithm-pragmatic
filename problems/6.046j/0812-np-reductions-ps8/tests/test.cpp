#include <algorithm>
#include <functional>
#include <numeric>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

bool eval(const Cnf& f, const std::vector<bool>& a) {
    for (const auto& clause : f.clauses) {
        bool ok = false;
        for (int lit : clause) {
            ok = ok || (lit > 0 ? a[lit] : !a[-lit]);
        }
        if (!ok) {
            return false;
        }
    }
    return true;
}

std::vector<bool> decode(int n, int mask) {
    std::vector<bool> a(static_cast<std::size_t>(n + 1), false);
    for (int v = 1; v <= n; ++v) {
        a[v] = (mask >> (v - 1)) & 1;
    }
    return a;
}

std::optional<std::vector<bool>> brute_sat(const Cnf& f) {
    for (int mask = 0; mask < (1 << f.num_vars); ++mask) {
        const auto a = decode(f.num_vars, mask);
        if (eval(f, a)) {
            return a;
        }
    }
    return std::nullopt;
}

int count_sat(const Cnf& f) {
    int count = 0;
    for (int mask = 0; mask < (1 << f.num_vars); ++mask) {
        count += eval(f, decode(f.num_vars, mask)) ? 1 : 0;
    }
    return count;
}

std::vector<std::vector<bool>> all_sat(const Cnf& f) {
    std::vector<std::vector<bool>> out;
    for (int mask = 0; mask < (1 << f.num_vars); ++mask) {
        auto a = decode(f.num_vars, mask);
        if (eval(f, a)) {
            out.push_back(std::move(a));
        }
    }
    return out;
}

Cnf random_cnf(Rng& rng, int n, int m, int k) {
    Cnf f;
    f.num_vars = n;
    for (int c = 0; c < m; ++c) {
        std::vector<int> clause;
        for (int i = 0; i < k; ++i) {
            const int v = static_cast<int>(rng.uniform(1, n));
            clause.push_back(rng.coin() ? v : -v);
        }
        f.clauses.push_back(std::move(clause));
    }
    return f;
}

// Maximum-profit independent set by exhaustive search; returns (best, one optimal set).
std::pair<long long, std::vector<int>> brute_donut(const DonutInstance& inst) {
    std::vector<std::vector<int>> adj(inst.n);
    for (const auto& [u, v] : inst.edges) {
        adj[u].push_back(v);
        adj[v].push_back(u);
    }
    long long best = -1;
    int best_mask = 0;
    for (int mask = 0; mask < (1 << inst.n); ++mask) {
        bool ok = true;
        long long total = 0;
        for (int v = 0; v < inst.n && ok; ++v) {
            if (mask >> v & 1) {
                total += inst.profit[v];
                for (int w : adj[v]) {
                    ok = ok && !(mask >> w & 1);
                }
            }
        }
        if (ok && total > best) {
            best = total;
            best_mask = mask;
        }
    }
    std::vector<int> chosen;
    for (int v = 0; v < inst.n; ++v) {
        if (best_mask >> v & 1) {
            chosen.push_back(v);
        }
    }
    return {best, chosen};
}

// A valid schedule of profit >= k for the instance, found by exhaustive search (EDF per subset).
std::optional<Schedule> witness_schedule(const SchedInstance& inst) {
    const int n = static_cast<int>(inst.tasks.size());
    for (int mask = 0; mask < (1 << n); ++mask) {
        std::vector<int> ids;
        for (int i = 0; i < n; ++i) {
            if (mask >> i & 1) {
                ids.push_back(i);
            }
        }
        std::sort(ids.begin(), ids.end(), [&](int a, int b) {
            return inst.tasks[a].deadline < inst.tasks[b].deadline;
        });
        Schedule schedule;
        long long clock = 0, profit = 0;
        bool ok = true;
        for (int id : ids) {
            schedule.push_back({id, clock});
            clock += inst.tasks[id].time;
            profit += inst.tasks[id].profit;
            ok = ok && clock <= inst.tasks[id].deadline;
        }
        if (ok && profit >= inst.k) {
            return schedule;
        }
    }
    return std::nullopt;
}

long long brute_sched(const std::vector<Task>& tasks) {
    const int n = static_cast<int>(tasks.size());
    long long best = 0;
    for (int mask = 0; mask < (1 << n); ++mask) {
        std::vector<int> ids;
        for (int i = 0; i < n; ++i) {
            if (mask >> i & 1) {
                ids.push_back(i);
            }
        }
        std::sort(ids.begin(), ids.end(),
                  [&](int a, int b) { return tasks[a].deadline < tasks[b].deadline; });
        long long clock = 0, profit = 0;
        bool ok = true;
        for (int id : ids) {
            clock += tasks[id].time;
            profit += tasks[id].profit;
            ok = ok && clock <= tasks[id].deadline;
        }
        if (ok) {
            best = std::max(best, profit);
        }
    }
    return best;
}

}  // namespace

TEST(1, "satisfies evaluates every clause") {
    const Cnf f{2, {{1, -2}, {2}}};
    CHECK(satisfies(f, {false, true, true}));
    CHECK(!satisfies(f, {false, false, true}));
    CHECK(!satisfies(f, {false, true, false}));
    CHECK(satisfies(Cnf{1, {}}, {false, false}));
    CHECK(!satisfies(Cnf{1, {{}}}, {false, true}));
}

TEST(1, "an unsatisfiable formula stays unsatisfiable") {
    const Cnf f{1, {{1}, {-1}}};
    const Cnf g = reduce_sat_to_triple_sat(f);
    CHECK_EQ(count_sat(g), 0);
}

TEST(1, "a satisfiable formula gets at least three assignments") {
    const Cnf f{2, {{1, 2}, {-1, -2}, {1}}};
    const Cnf g = reduce_sat_to_triple_sat(f);
    CHECK_GE(count_sat(g), 3);
}

TEST(1, "random formulas preserve satisfiability") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 6));
        const int m = static_cast<int>(rng.uniform(0, 8));
        const Cnf f = random_cnf(rng, n, m, 2);
        const Cnf g = reduce_sat_to_triple_sat(f);
        INFO("round ", round);
        const bool original = brute_sat(f).has_value();
        CHECK_EQ(original, count_sat(g) >= 3);
        if (original) {
            for (const auto& a : all_sat(g)) {
                const auto lifted = lift_triple_sat_to_sat(f, a);
                REQUIRE_EQ(lifted.size(), f.num_vars + 1);
                CHECK(eval(f, lifted));
            }
        }
    }
}

TEST(2, "clause gadgets are triangles and every vertex pays one") {
    const Cnf f{3, {{1, 2, 3}}};
    const DonutInstance inst = reduce_3sat_to_donut(f);
    CHECK_EQ(inst.k, 1);
    CHECK_EQ(inst.n, 3);
    CHECK_EQ(inst.edges.size(), 3);
    for (long long p : inst.profit) {
        CHECK_EQ(p, 1);
    }
}

TEST(2, "complementary literals conflict across clauses") {
    const Cnf f{1, {{1, 1, 1}, {-1, -1, -1}}};
    const DonutInstance inst = reduce_3sat_to_donut(f);
    CHECK_EQ(brute_donut(inst).first, 1);
    CHECK_EQ(inst.k, 2);
}

TEST(2, "random 3-CNF instances preserve the answer and lift to witnesses") {
    Rng rng;
    for (int round = 0; round < 150; ++round) {
        const int n = static_cast<int>(rng.uniform(3, 6));
        const int m = static_cast<int>(rng.uniform(1, 5));
        const Cnf f = random_cnf(rng, n, m, 3);
        const DonutInstance inst = reduce_3sat_to_donut(f);
        const auto [best, chosen] = brute_donut(inst);
        const bool satisfiable = brute_sat(f).has_value();
        INFO("round ", round);
        CHECK_EQ(best >= inst.k, satisfiable);
        if (satisfiable) {
            REQUIRE(best >= inst.k);
            const auto a = lift_donut_to_3sat(f, inst, chosen);
            REQUIRE_EQ(a.size(), n + 1);
            CHECK(eval(f, a));
        }
    }
}

TEST(3, "valid schedule checks overlap and deadlines") {
    const std::vector<Task> tasks{{2, 5, 4}, {3, 7, 8}};
    CHECK(valid_schedule(tasks, Schedule{{0, 0}, {1, 2}}));
    CHECK(valid_schedule(tasks, Schedule{}));
    CHECK(!valid_schedule(tasks, Schedule{{0, 1}, {1, 2}}));   // overlap
    CHECK(!valid_schedule(tasks, Schedule{{0, 3}}));           // deadline miss
    CHECK(!valid_schedule(tasks, Schedule{{0, 0}, {0, 2}}));   // repeated task
}

TEST(3, "profit sums the scheduled tasks") {
    const std::vector<Task> tasks{{2, 5, 4}, {3, 7, 8}};
    CHECK_EQ(schedule_profit(tasks, Schedule{{1, 0}}), 7);
    CHECK_EQ(schedule_profit(tasks, Schedule{}), 0);
}

TEST(3, "random instances: reduction preserves the answer, lift recovers the subset") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        SubsetSumInstance src;
        for (int i = 0; i < n; ++i) {
            src.a.push_back(rng.uniform(1, 20));
        }
        const long long total = std::accumulate(src.a.begin(), src.a.end(), 0LL);
        src.t = rng.uniform(0, total);
        const SchedInstance dst = reduce_subset_sum_to_sched(src);
        REQUIRE_EQ(dst.tasks.size(), n);
        INFO("round ", round);
        bool yes = false;
        for (int mask = 0; mask < (1 << n); ++mask) {
            long long sum = 0;
            for (int i = 0; i < n; ++i) {
                sum += (mask >> i & 1) ? src.a[i] : 0;
            }
            yes = yes || sum == src.t;
        }
        CHECK_EQ(brute_sched(dst.tasks) >= dst.k, yes);
        if (yes) {
            // Build any witness schedule for the target instance and lift it back.
            const auto schedule = witness_schedule(dst);
            REQUIRE(schedule.has_value());
            CHECK(valid_schedule(dst.tasks, *schedule));
            CHECK_GE(schedule_profit(dst.tasks, *schedule), dst.k);
            const auto subset = lift_sched_to_subset_sum(src, dst, *schedule);
            long long sum = 0;
            std::set<int> seen;
            for (int i : subset) {
                REQUIRE(i >= 0 && i < n);
                CHECK(seen.insert(i).second);
                sum += src.a[i];
            }
            CHECK_EQ(sum, src.t);
        }
    }
}

TEST(4, "no tasks means nothing to schedule") {
    CHECK(best_schedule(std::vector<Task>{}).empty());
}

TEST(4, "a single task that fits is taken") {
    const std::vector<Task> tasks{{3, 9, 5}};
    const auto schedule = best_schedule(tasks);
    REQUIRE(valid_schedule(tasks, schedule));
    CHECK_EQ(schedule_profit(tasks, schedule), 9);
}

TEST(4, "a task that cannot meet its deadline is skipped") {
    const std::vector<Task> tasks{{5, 9, 3}};
    CHECK_EQ(schedule_profit(tasks, best_schedule(tasks)), 0);
}

TEST(4, "tasks may be ordered by deadline even if a later one was listed first") {
    const std::vector<Task> tasks{{2, 3, 6}, {3, 4, 3}};
    const auto schedule = best_schedule(tasks);
    REQUIRE(valid_schedule(tasks, schedule));
    CHECK_EQ(schedule_profit(tasks, schedule), 7);
}

TEST(4, "random task sets match brute force") {
    Rng rng;
    for (int round = 0; round < 400; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 10));
        std::vector<Task> tasks(n);
        for (auto& t : tasks) {
            t = {rng.uniform(1, 6), rng.uniform(1, 20), rng.uniform(1, 18)};
        }
        const auto schedule = best_schedule(tasks);
        INFO("round ", round);
        REQUIRE(valid_schedule(tasks, schedule));
        CHECK_EQ(schedule_profit(tasks, schedule), brute_sched(tasks));
    }
}

PERF(4, "pseudo-polynomial DP on thousands of tasks", 800) {
    Rng rng;
    std::vector<Task> tasks(3000);
    for (auto& t : tasks) {
        t = {rng.uniform(1, 50), rng.uniform(1, 1000), rng.uniform(1, 30'000)};
    }
    const auto schedule = best_schedule(tasks);
    CHECK(valid_schedule(tasks, schedule));
    CHECK_GT(schedule_profit(tasks, schedule), 0);
}
