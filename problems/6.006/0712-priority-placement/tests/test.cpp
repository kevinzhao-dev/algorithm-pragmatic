#include <algorithm>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <vector>

#include "07_heap_meter.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Tasks = std::vector<Task>;
using Servers = std::vector<Server>;
using Assignment = std::vector<std::size_t>;

std::int64_t overload(const Task& task, const Server& server) {
    return std::max(0, task.demand - server.capacity);
}

// Validates an assignment and returns its total cost.
std::int64_t check_assignment(const Tasks& tasks, const Servers& servers,
                              const Assignment& assignment) {
    REQUIRE_EQ(assignment.size(), tasks.size());
    std::int64_t total = 0;
    std::vector<bool> used(servers.size(), false);
    for (std::size_t i = 0; i < tasks.size(); ++i) {
        REQUIRE(assignment[i] < servers.size());
        REQUIRE(!used[assignment[i]]);
        used[assignment[i]] = true;
        total += overload(tasks[i], servers[assignment[i]]);
    }
    for (std::size_t a = 0; a < tasks.size(); ++a) {
        for (std::size_t b = 0; b < tasks.size(); ++b) {
            if (tasks[a].priority > tasks[b].priority) {
                REQUIRE(servers[assignment[a]].latency < servers[assignment[b]].latency);
            }
        }
    }
    return total;
}

// Same checks for a plan that may drop tasks; returns its total cost.
std::int64_t check_plan(const Tasks& tasks, const Servers& servers,
                        const std::vector<std::int64_t>& penalties, const DropPlan& plan) {
    REQUIRE_EQ(plan.assignment.size(), tasks.size());
    std::int64_t total = 0;
    std::vector<bool> used(servers.size(), false);
    for (std::size_t i = 0; i < tasks.size(); ++i) {
        if (plan.assignment[i]) {
            REQUIRE(*plan.assignment[i] < servers.size());
            REQUIRE(!used[*plan.assignment[i]]);
            used[*plan.assignment[i]] = true;
            total += overload(tasks[i], servers[*plan.assignment[i]]);
        } else {
            total += penalties[i];
        }
    }
    for (std::size_t a = 0; a < tasks.size(); ++a) {
        for (std::size_t b = 0; b < tasks.size(); ++b) {
            if (plan.assignment[a] && plan.assignment[b] && tasks[a].priority > tasks[b].priority) {
                REQUIRE(servers[*plan.assignment[a]].latency <
                        servers[*plan.assignment[b]].latency);
            }
        }
    }
    return total;
}

Tasks random_tasks(Rng& rng, std::size_t count) {
    std::vector<int> priorities = rng.permutation(count);
    Tasks tasks(count);
    for (std::size_t i = 0; i < count; ++i) {
        tasks[i] = {priorities[i], static_cast<int>(rng.uniform(1, 30))};
    }
    return tasks;
}

Servers random_servers(Rng& rng, std::size_t count) {
    std::vector<int> latencies = rng.permutation(count);
    Servers servers(count);
    for (std::size_t i = 0; i < count; ++i) {
        servers[i] = {latencies[i], static_cast<int>(rng.uniform(1, 30))};
    }
    return servers;
}

// Enumerates every way to give the tasks (best first) an increasing run of servers (closest
// first); optionally a task may be dropped at its penalty.
std::int64_t brute_force(const Tasks& tasks, const Servers& servers,
                         const std::vector<std::int64_t>* penalties) {
    std::vector<std::size_t> task_order(tasks.size());
    std::iota(task_order.begin(), task_order.end(), std::size_t{0});
    std::ranges::sort(task_order, [&](std::size_t a, std::size_t b) {
        return tasks[a].priority > tasks[b].priority;
    });
    Servers sorted = servers;
    std::ranges::sort(sorted, [](const Server& a, const Server& b) { return a.latency < b.latency; });

    constexpr std::int64_t kInfinity = std::numeric_limits<std::int64_t>::max() / 4;
    const auto recurse = [&](auto&& self, std::size_t i, std::size_t j) -> std::int64_t {
        if (i == tasks.size()) {
            return 0;
        }
        std::int64_t best = kInfinity;
        if (penalties != nullptr) {
            best = (*penalties)[task_order[i]] + self(self, i + 1, j);
        }
        for (std::size_t k = j; k < sorted.size(); ++k) {
            best = std::min(best, overload(tasks[task_order[i]], sorted[k]) + self(self, i + 1, k + 1));
        }
        return best;
    };
    return recurse(recurse, 0, 0);
}

}  // namespace

TEST(1, "no tasks cost nothing") {
    const Servers servers{{5, 10}};
    CHECK_EQ(min_overload({}, servers), std::optional<std::int64_t>(0));
    CHECK_EQ(min_overload({}, {}), std::optional<std::int64_t>(0));
}

TEST(1, "more tasks than servers is infeasible") {
    const Tasks tasks{{1, 5}, {2, 5}};
    const Servers servers{{1, 10}};
    CHECK_EQ(min_overload(tasks, servers), std::optional<std::int64_t>());
}

TEST(1, "a task that fits costs nothing") {
    const Tasks tasks{{1, 7}};
    const Servers servers{{3, 7}};
    CHECK_EQ(min_overload(tasks, servers), std::optional<std::int64_t>(0));
}

TEST(1, "overload is demand minus capacity") {
    const Tasks tasks{{1, 9}};
    const Servers servers{{3, 7}};
    CHECK_EQ(min_overload(tasks, servers), std::optional<std::int64_t>(2));
}

TEST(1, "input order does not matter") {
    // The more important task sits last in the input.
    const Tasks tasks{{1, 2}, {9, 20}};
    const Servers servers{{50, 25}, {10, 3}, {30, 2}};
    // Forced: the important task must land closer, so it takes latency 10 or 30.
    CHECK_EQ(min_overload(tasks, servers), std::optional<std::int64_t>(17));
}

TEST(1, "the order rule beats simply matching demand to capacity") {
    const Tasks tasks{{2, 2}, {1, 10}};
    const Servers servers{{1, 2}, {2, 10}};
    CHECK_EQ(min_overload(tasks, servers), std::optional<std::int64_t>(0));

    // Without the rule, demand 10 would take capacity 10 and demand 2 capacity 2 at no cost.
    const Tasks reversed{{2, 10}, {1, 2}};
    CHECK_EQ(min_overload(reversed, servers), std::optional<std::int64_t>(8));
}

TEST(1, "random instances match brute force") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const auto task_count = static_cast<std::size_t>(rng.uniform(0, 5));
        const auto server_count = task_count + static_cast<std::size_t>(rng.uniform(0, 3));
        const Tasks tasks = random_tasks(rng, task_count);
        const Servers servers = random_servers(rng, server_count);
        INFO("trial ", trial);
        REQUIRE_EQ(min_overload(tasks, servers),
                   std::optional<std::int64_t>(brute_force(tasks, servers, nullptr)));
    }
}

PERF(1, "two hundred tasks onto two hundred thousand servers", 800) {
    Rng rng;
    const Tasks tasks = random_tasks(rng, 200);
    const Servers servers = random_servers(rng, 200'000);
    CHECK(min_overload(tasks, servers).has_value());
}

TEST(2, "more tasks than servers has no assignment") {
    const Tasks tasks{{1, 5}, {2, 5}};
    const Servers servers{{1, 10}};
    CHECK(!assign_servers(tasks, servers).has_value());
}

TEST(2, "no tasks gives an empty assignment") {
    const Servers servers{{1, 10}};
    const auto assignment = assign_servers({}, servers);
    REQUIRE(assignment.has_value());
    CHECK(assignment->empty());
}

TEST(2, "indices refer to the original input positions") {
    const Tasks tasks{{1, 2}, {9, 20}};
    const Servers servers{{50, 25}, {10, 3}, {30, 2}};
    const auto assignment = assign_servers(tasks, servers);
    REQUIRE(assignment.has_value());
    CHECK_EQ(check_assignment(tasks, servers, *assignment), *min_overload(tasks, servers));
}

TEST(2, "random assignments are valid and optimal") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const auto task_count = static_cast<std::size_t>(rng.uniform(1, 5));
        const auto server_count = task_count + static_cast<std::size_t>(rng.uniform(0, 3));
        const Tasks tasks = random_tasks(rng, task_count);
        const Servers servers = random_servers(rng, server_count);
        INFO("trial ", trial);
        const auto assignment = assign_servers(tasks, servers);
        REQUIRE(assignment.has_value());
        REQUIRE_EQ(check_assignment(tasks, servers, *assignment),
                   brute_force(tasks, servers, nullptr));
    }
}

TEST(3, "min_overload uses memory linear in the number of servers") {
    Rng rng;
    const Tasks tasks = random_tasks(rng, 1000);
    const Servers servers = random_servers(rng, 4000);
    heap_meter::Scope scope;
    const auto cost = min_overload(tasks, servers);
    CHECK_LE(scope.peak_bytes(), 96 * servers.size());
    CHECK(cost.has_value());
}

TEST(3, "rolling arrays keep the same answers") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        const auto task_count = static_cast<std::size_t>(rng.uniform(1, 30));
        const auto server_count = task_count + static_cast<std::size_t>(rng.uniform(0, 30));
        const Tasks tasks = random_tasks(rng, task_count);
        const Servers servers = random_servers(rng, server_count);
        INFO("trial ", trial);
        REQUIRE_EQ(*min_overload(tasks, servers),
                   check_assignment(tasks, servers, *assign_servers(tasks, servers)));
    }
}

TEST(4, "with no tasks nothing is dropped") {
    const Servers servers{{1, 5}};
    const auto plan = plan_with_drops({}, servers, {});
    CHECK_EQ(plan.cost, 0);
    CHECK(plan.assignment.empty());
}

TEST(4, "without servers everything is dropped") {
    const Tasks tasks{{1, 5}, {2, 7}};
    const std::vector<std::int64_t> penalties{3, 4};
    const auto plan = plan_with_drops(tasks, {}, penalties);
    CHECK_EQ(plan.cost, 7);
    CHECK(!plan.assignment[0].has_value());
    CHECK(!plan.assignment[1].has_value());
}

TEST(4, "a cheap drop is better than a large overload") {
    const Tasks tasks{{1, 100}};
    const Servers servers{{1, 1}};
    const std::vector<std::int64_t> penalties{5};
    const auto plan = plan_with_drops(tasks, servers, penalties);
    CHECK_EQ(plan.cost, 5);
    CHECK(!plan.assignment[0].has_value());
}

TEST(4, "tasks may outnumber servers") {
    const Tasks tasks{{3, 1}, {2, 1}, {1, 1}};
    const Servers servers{{1, 1}};
    const std::vector<std::int64_t> penalties{9, 1, 9};
    const auto plan = plan_with_drops(tasks, servers, penalties);
    CHECK_EQ(plan.cost, 10);  // keep one of the two expensive tasks, drop the others
}

TEST(4, "random plans are valid and optimal") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const auto task_count = static_cast<std::size_t>(rng.uniform(0, 5));
        const auto server_count = static_cast<std::size_t>(rng.uniform(0, 5));
        const Tasks tasks = random_tasks(rng, task_count);
        const Servers servers = random_servers(rng, server_count);
        std::vector<std::int64_t> penalties(task_count);
        for (auto& penalty : penalties) {
            penalty = rng.uniform(0, 40);
        }
        INFO("trial ", trial);
        const auto plan = plan_with_drops(tasks, servers, penalties);
        REQUIRE_EQ(check_plan(tasks, servers, penalties, plan), plan.cost);
        REQUIRE_EQ(plan.cost, brute_force(tasks, servers, &penalties));
    }
}

PERF(4, "five hundred tasks onto twenty thousand servers", 800) {
    Rng rng;
    const Tasks tasks = random_tasks(rng, 500);
    const Servers servers = random_servers(rng, 20'000);
    const std::vector<std::int64_t> penalties(500, 10);
    CHECK_GE(plan_with_drops(tasks, servers, penalties).cost, 0);
}
