#include <limits>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

bool compatible(const Request& a, const Request& b) {
    return a.finish <= b.start || b.finish <= a.start;
}

// Confirms the indices are valid, distinct, pairwise compatible and listed by start time.
void check_feasible(const std::vector<Request>& requests, const std::vector<int>& chosen) {
    std::set<int> seen;
    for (std::size_t i = 0; i < chosen.size(); ++i) {
        REQUIRE(chosen[i] >= 0 && chosen[i] < static_cast<int>(requests.size()));
        CHECK(seen.insert(chosen[i]).second);
        if (i > 0) {
            CHECK(requests[static_cast<std::size_t>(chosen[i - 1])].finish <=
                  requests[static_cast<std::size_t>(chosen[i])].start);
        }
    }
}

std::int64_t total_of(const std::vector<Request>& requests, const std::vector<int>& chosen) {
    std::int64_t total = 0;
    for (int index : chosen) {
        total += requests[static_cast<std::size_t>(index)].value;
    }
    return total;
}

// Exhaustive search over include/exclude, used as the oracle for small inputs.
struct BruteResult {
    int best_count = 0;
    std::int64_t best_value = 0;
};

void brute_walk(const std::vector<Request>& requests, std::size_t index, std::vector<int>& taken,
                BruteResult& out) {
    if (index == requests.size()) {
        out.best_count = std::max(out.best_count, static_cast<int>(taken.size()));
        std::int64_t value = 0;
        for (int t : taken) {
            value += requests[static_cast<std::size_t>(t)].value;
        }
        out.best_value = std::max(out.best_value, value);
        return;
    }
    brute_walk(requests, index + 1, taken, out);
    bool fits = true;
    for (int t : taken) {
        fits = fits && compatible(requests[static_cast<std::size_t>(t)], requests[index]);
    }
    if (fits) {
        taken.push_back(static_cast<int>(index));
        brute_walk(requests, index + 1, taken, out);
        taken.pop_back();
    }
}

BruteResult brute(const std::vector<Request>& requests) {
    BruteResult out;
    std::vector<int> taken;
    brute_walk(requests, 0, taken, out);
    return out;
}

std::vector<Request> random_requests(Rng& rng, std::size_t n, std::int64_t horizon,
                                     std::int64_t max_value) {
    std::vector<Request> requests(n);
    for (Request& r : requests) {
        r.start = rng.uniform(0, horizon - 1);
        r.finish = rng.uniform(r.start + 1, horizon);
        r.value = rng.uniform(0, max_value);
    }
    return requests;
}

std::vector<Request> large_requests(Rng& rng, std::size_t n) {
    std::vector<Request> requests(n);
    for (Request& r : requests) {
        r.start = rng.uniform(0, 1'000'000'000);
        r.finish = r.start + rng.uniform(1, 1'000'000);
        r.value = rng.uniform(0, 1'000'000);
    }
    return requests;
}

}  // namespace

TEST(1, "no requests means an empty selection") {
    CHECK(max_compatible(std::vector<Request>{}).empty());
}

TEST(1, "a single request is selected") {
    const std::vector<Request> requests{{3, 9, 1}};
    CHECK_EQ(max_compatible(requests), std::vector<int>{0});
}

TEST(1, "requests that only touch are compatible") {
    const std::vector<Request> requests{{5, 8, 1}, {0, 5, 1}, {8, 9, 1}};
    CHECK_EQ(max_compatible(requests), (std::vector<int>{1, 0, 2}));
}

TEST(1, "identical requests are chosen only once") {
    const std::vector<Request> requests{{1, 4, 1}, {1, 4, 1}, {1, 4, 1}};
    CHECK_EQ(max_compatible(requests).size(), 1);
}

TEST(1, "the earliest-start rule would be wrong here") {
    const std::vector<Request> requests{{0, 100, 1}, {1, 2, 1}, {3, 4, 1}, {5, 6, 1}};
    CHECK_EQ(max_compatible(requests), (std::vector<int>{1, 2, 3}));
}

TEST(1, "the shortest-request rule would be wrong here") {
    const std::vector<Request> requests{{0, 10, 1}, {9, 12, 1}, {11, 20, 1}};
    CHECK_EQ(max_compatible(requests).size(), 2);
}

TEST(1, "a zero-length request is rejected") {
    const std::vector<Request> requests{{4, 4, 1}};
    CHECK_THROWS_AS(max_compatible(requests), std::invalid_argument);
}

TEST(1, "a negative value is rejected") {
    const std::vector<Request> requests{{1, 2, -1}};
    CHECK_THROWS_AS(max_compatible(requests), std::invalid_argument);
}

TEST(1, "random requests match an exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        INFO("trial ", trial);
        const auto requests =
            random_requests(rng, static_cast<std::size_t>(rng.uniform(1, 12)), 20, 10);
        const auto chosen = max_compatible(requests);
        check_feasible(requests, chosen);
        REQUIRE_EQ(static_cast<int>(chosen.size()), brute(requests).best_count);
    }
}

PERF(1, "one million requests", 2500) {
    Rng rng;
    const auto requests = large_requests(rng, 1'000'000);
    const auto chosen = max_compatible(requests);
    CHECK_GT(chosen.size(), 0);
}

TEST(2, "no requests has total zero") {
    const Schedule schedule = max_value_schedule(std::vector<Request>{});
    CHECK_EQ(schedule.total, 0);
    CHECK(schedule.chosen.empty());
}

TEST(2, "one heavy request beats many light ones") {
    const std::vector<Request> requests{{0, 10, 100}, {0, 3, 10}, {3, 6, 10}, {6, 10, 10}};
    const Schedule schedule = max_value_schedule(requests);
    CHECK_EQ(schedule.total, 100);
    CHECK_EQ(schedule.chosen, std::vector<int>{0});
}

TEST(2, "many light requests beat one heavy request") {
    const std::vector<Request> requests{{0, 10, 25}, {0, 3, 10}, {3, 6, 10}, {6, 10, 10}};
    const Schedule schedule = max_value_schedule(requests);
    CHECK_EQ(schedule.total, 30);
    CHECK_EQ(schedule.chosen, (std::vector<int>{1, 2, 3}));
}

TEST(2, "the total counts touching requests together") {
    const std::vector<Request> requests{{0, 2, 5}, {2, 4, 7}};
    CHECK_EQ(max_value_schedule(requests).total, 12);
}

TEST(2, "large values do not overflow") {
    std::vector<Request> requests;
    for (std::int64_t i = 0; i < 100'000; ++i) {
        requests.push_back({i, i + 1, 1'000'000'000'000});
    }
    const Schedule schedule = max_value_schedule(requests);
    CHECK_EQ(schedule.total, 100'000 * 1'000'000'000'000LL);
    CHECK_EQ(schedule.chosen.size(), 100'000);
}

TEST(2, "an invalid request is rejected") {
    const std::vector<Request> requests{{0, 1, 1}, {5, 2, 3}};
    CHECK_THROWS_AS(max_value_schedule(requests), std::invalid_argument);
}

TEST(2, "random requests match an exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        INFO("trial ", trial);
        const auto requests =
            random_requests(rng, static_cast<std::size_t>(rng.uniform(1, 12)), 25, 30);
        const Schedule schedule = max_value_schedule(requests);
        check_feasible(requests, schedule.chosen);
        REQUIRE_EQ(schedule.total, total_of(requests, schedule.chosen));
        REQUIRE_EQ(schedule.total, brute(requests).best_value);
    }
}

PERF(2, "weighted schedule over one million requests", 3000) {
    Rng rng;
    const auto requests = large_requests(rng, 1'000'000);
    const Schedule schedule = max_value_schedule(requests);
    CHECK_EQ(schedule.total, total_of(requests, schedule.chosen));
}

TEST(3, "with no conflicts every request has zero regret") {
    const std::vector<Request> requests{{0, 1, 2}, {1, 2, 3}, {2, 3, 4}};
    const WhatIfPlanner planner(requests);
    CHECK_EQ(planner.best_total(), 9);
    for (std::size_t i = 0; i < requests.size(); ++i) {
        CHECK_EQ(planner.regret(i), 0);
        CHECK_EQ(planner.best_with(i), 9);
    }
}

TEST(3, "forcing a request reports what it costs") {
    const std::vector<Request> requests{{0, 10, 100}, {0, 3, 10}, {3, 6, 10}};
    const WhatIfPlanner planner(requests);
    CHECK_EQ(planner.best_total(), 100);
    CHECK_EQ(planner.best_with(1), 20);
    CHECK_EQ(planner.regret(1), 80);
    CHECK_EQ(planner.regret(0), 0);
}

TEST(3, "an out of range index throws") {
    const std::vector<Request> requests{{0, 1, 2}};
    const WhatIfPlanner planner(requests);
    CHECK_THROWS_AS(planner.best_with(1), std::out_of_range);
}

TEST(3, "the planner owns its data after construction") {
    std::vector<Request> requests{{0, 2, 5}, {1, 3, 9}, {3, 4, 1}};
    const WhatIfPlanner planner(requests);
    requests.assign(1000, Request{7, 8, 0});
    requests.clear();
    requests.shrink_to_fit();
    CHECK_EQ(planner.best_total(), 10);
    CHECK_EQ(planner.best_with(0), 6);
}

TEST(3, "random requests match forced exhaustive searches") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        INFO("trial ", trial);
        const auto requests =
            random_requests(rng, static_cast<std::size_t>(rng.uniform(1, 10)), 16, 20);
        const WhatIfPlanner planner(requests);
        const std::int64_t overall = brute(requests).best_value;
        REQUIRE_EQ(planner.best_total(), overall);
        for (std::size_t i = 0; i < requests.size(); ++i) {
            std::vector<Request> rest;
            for (std::size_t j = 0; j < requests.size(); ++j) {
                if (j != i && compatible(requests[i], requests[j])) {
                    rest.push_back(requests[j]);
                }
            }
            const std::int64_t forced = requests[i].value + brute(rest).best_value;
            REQUIRE_EQ(planner.best_with(i), forced);
            REQUIRE_EQ(planner.regret(i), overall - forced);
        }
    }
}

PERF(3, "two hundred thousand what-if queries", 1500) {
    Rng rng;
    const auto requests = large_requests(rng, 200'000);
    const WhatIfPlanner planner(requests);
    std::int64_t sum = 0;
    for (std::size_t i = 0; i < requests.size(); ++i) {
        sum += planner.regret(i);
    }
    CHECK_GE(sum, 0);
}
