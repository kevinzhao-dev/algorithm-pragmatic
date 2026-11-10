#include <algorithm>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

long long brute_spt(const std::vector<long long>& durations) {
    std::vector<int> perm(durations.size());
    std::iota(perm.begin(), perm.end(), 0);
    long long best = std::numeric_limits<long long>::max();
    do {
        long long clock = 0, total = 0;
        for (int id : perm) {
            clock += durations[id];
            total += clock;
        }
        best = std::min(best, total);
    } while (std::next_permutation(perm.begin(), perm.end()));
    return best;
}

// Exhaustive search over unit-time slots with memoization on (time, remaining).
long long brute_srpt(const std::vector<Job>& jobs) {
    const int n = static_cast<int>(jobs.size());
    std::map<std::pair<long long, std::vector<long long>>, long long> memo;
    const std::function<long long(long long, std::vector<long long>)> solve =
        [&](long long t, std::vector<long long> left) -> long long {
        bool any = false;
        for (long long x : left) {
            any = any || x > 0;
        }
        if (!any) {
            return 0;
        }
        const auto key = std::make_pair(t, left);
        if (const auto it = memo.find(key); it != memo.end()) {
            return it->second;
        }
        long long best = std::numeric_limits<long long>::max();
        bool runnable = false;
        for (int i = 0; i < n; ++i) {
            if (jobs[i].release <= t && left[i] > 0) {
                runnable = true;
                --left[i];
                const long long here = left[i] == 0 ? t + 1 : 0;
                best = std::min(best, here + solve(t + 1, left));
                ++left[i];
            }
        }
        if (!runnable) {
            best = solve(t + 1, left);  // nothing released yet: idle one slot
        }
        return memo[key] = best;
    };
    std::vector<long long> left(n);
    for (int i = 0; i < n; ++i) {
        left[i] = jobs[i].duration;
    }
    return solve(0, left);
}

int max_overlap(const std::vector<Interval>& intervals) {
    std::vector<std::pair<long long, int>> events;
    for (const auto& iv : intervals) {
        events.emplace_back(iv.start, 1);
        events.emplace_back(iv.finish, -1);
    }
    std::sort(events.begin(), events.end());
    int depth = 0, best = 0;
    for (const auto& [_, delta] : events) {
        depth += delta;
        best = std::max(best, depth);
    }
    return best;
}

void check_allocation(const std::vector<Interval>& intervals, const Allocation& allocation) {
    REQUIRE_EQ(allocation.assignment.size(), intervals.size());
    CHECK_EQ(allocation.machines, max_overlap(intervals));
    std::vector<std::vector<int>> per_machine(static_cast<std::size_t>(allocation.machines));
    for (int i = 0; i < static_cast<int>(intervals.size()); ++i) {
        const int machine = allocation.assignment[i];
        REQUIRE(machine >= 0 && machine < allocation.machines);
        per_machine[machine].push_back(i);
    }
    for (auto& ids : per_machine) {
        std::sort(ids.begin(), ids.end(),
                  [&](int a, int b) { return intervals[a].start < intervals[b].start; });
        for (std::size_t i = 0; i + 1 < ids.size(); ++i) {
            CHECK_LE(intervals[ids[i]].finish, intervals[ids[i + 1]].start);
        }
    }
}

}  // namespace

TEST(1, "no jobs") {
    const std::vector<long long> none;
    CHECK(spt_order(none).empty());
    CHECK_EQ(total_completion_time(none, std::vector<int>{}), 0);
}

TEST(1, "total completion time adds up finishing times") {
    const std::vector<long long> durations{3, 1, 2};
    CHECK_EQ(total_completion_time(durations, std::vector<int>{1, 2, 0}), 1 + 3 + 6);
}

TEST(1, "shortest jobs run first") {
    const std::vector<long long> durations{5, 1, 3};
    CHECK_EQ(spt_order(durations), std::vector<int>{1, 2, 0});
}

TEST(1, "random lists match permutation search") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 8));
        std::vector<long long> durations(n);
        for (auto& d : durations) {
            d = rng.uniform(1, 20);
        }
        INFO("round ", round);
        const auto order = spt_order(durations);
        REQUIRE_EQ(order.size(), n);
        auto sorted_order = order;
        std::sort(sorted_order.begin(), sorted_order.end());
        std::vector<int> identity(n);
        std::iota(identity.begin(), identity.end(), 0);
        CHECK_EQ(sorted_order, identity);
        CHECK_EQ(total_completion_time(durations, order), brute_spt(durations));
    }
}

TEST(2, "a single job runs from its release") {
    const std::vector<Job> jobs{{5, 3}};
    CHECK_EQ(srpt_completion(jobs), std::vector<long long>{8});
}

TEST(2, "a short job preempts a long one") {
    const std::vector<Job> jobs{{0, 10}, {2, 1}};
    CHECK_EQ(srpt_completion(jobs), std::vector<long long>{11, 3});
}

TEST(2, "idle gaps wait for the next release") {
    const std::vector<Job> jobs{{0, 2}, {10, 3}};
    CHECK_EQ(srpt_completion(jobs), std::vector<long long>{2, 13});
}

TEST(2, "simultaneous arrivals run shortest first") {
    const std::vector<Job> jobs{{0, 3}, {0, 1}, {0, 2}};
    CHECK_EQ(srpt_completion(jobs), std::vector<long long>{6, 1, 3});
}

TEST(2, "random small instances match exhaustive search") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 4));
        std::vector<Job> jobs(n);
        for (auto& j : jobs) {
            j = {rng.uniform(0, 6), rng.uniform(1, 4)};
        }
        const auto done = srpt_completion(jobs);
        REQUIRE_EQ(done.size(), n);
        INFO("round ", round);
        for (int i = 0; i < n; ++i) {
            CHECK_GE(done[i], jobs[i].release + jobs[i].duration);
        }
        CHECK_EQ(std::accumulate(done.begin(), done.end(), 0LL), brute_srpt(jobs));
    }
}

PERF(2, "a million online arrivals", 1500) {
    Rng rng;
    std::vector<Job> jobs(1'000'000);
    for (auto& j : jobs) {
        j = {rng.uniform(0, 1'000'000'000), rng.uniform(1, 2000)};
    }
    const auto done = srpt_completion(jobs);
    CHECK_EQ(done.size(), jobs.size());
}

TEST(3, "no intervals needs no machines") {
    const auto allocation = allocate(std::vector<Interval>{});
    CHECK_EQ(allocation.machines, 0);
    CHECK(allocation.assignment.empty());
}

TEST(3, "touching intervals can share a machine") {
    const std::vector<Interval> intervals{{0, 2}, {2, 4}, {4, 6}};
    const auto allocation = allocate(intervals);
    CHECK_EQ(allocation.machines, 1);
    check_allocation(intervals, allocation);
}

TEST(3, "three overlapping intervals need three machines") {
    const std::vector<Interval> intervals{{0, 10}, {1, 9}, {2, 8}};
    const auto allocation = allocate(intervals);
    CHECK_EQ(allocation.machines, 3);
    check_allocation(intervals, allocation);
}

TEST(3, "order of input does not matter") {
    const std::vector<Interval> intervals{{5, 7}, {0, 3}, {1, 6}, {6, 9}};
    check_allocation(intervals, allocate(intervals));
}

TEST(3, "random calendars use the minimum number of machines") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 60));
        std::vector<Interval> intervals(n);
        for (auto& iv : intervals) {
            iv.start = rng.uniform(0, 100);
            iv.finish = iv.start + rng.uniform(1, 30);
        }
        INFO("round ", round);
        check_allocation(intervals, allocate(intervals));
    }
}

PERF(3, "a million intervals", 1500) {
    Rng rng;
    std::vector<Interval> intervals(1'000'000);
    for (auto& iv : intervals) {
        iv.start = rng.uniform(0, 1'000'000'000);
        iv.finish = iv.start + rng.uniform(1, 100'000);
    }
    const auto allocation = allocate(intervals);
    CHECK_GT(allocation.machines, 0);
}
