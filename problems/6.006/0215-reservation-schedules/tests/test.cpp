#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

// Sweep over all endpoints: slow but obviously correct.
Schedule brute_schedule(const std::vector<JobRequest>& jobs) {
    std::map<std::int64_t, std::int64_t> delta;
    for (const JobRequest& job : jobs) {
        delta[job.start] += 1;
        delta[job.end] -= 1;
    }
    Schedule out;
    std::int64_t count = 0;
    std::int64_t previous = 0;
    for (const auto& [time, change] : delta) {
        if (count > 0) {
            if (!out.empty() && out.back().end == previous && out.back().machines == count) {
                out.back().end = time;
            } else {
                out.push_back({count, previous, time});
            }
        }
        count += change;
        previous = time;
    }
    return out;
}

std::vector<JobRequest> random_jobs(Rng& rng, std::size_t count, int horizon) {
    std::vector<JobRequest> jobs;
    jobs.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const std::int64_t start = rng.uniform(0, horizon - 1);
        const std::int64_t end = rng.uniform(start + 1, horizon);
        jobs.push_back({start, end});
    }
    return jobs;
}

void check_valid_schedule(const Schedule& schedule) {
    for (std::size_t i = 0; i < schedule.size(); ++i) {
        INFO("piece ", i);
        REQUIRE(schedule[i].machines > 0);
        REQUIRE(schedule[i].start < schedule[i].end);
        if (i + 1 < schedule.size()) {
            REQUIRE(schedule[i].end <= schedule[i + 1].start);
            if (schedule[i].end == schedule[i + 1].start) {
                REQUIRE(schedule[i].machines != schedule[i + 1].machines);
            }
        }
    }
}

}  // namespace

TEST(1, "merging with an empty schedule returns the other one") {
    const Schedule a = {{2, 1, 3}};
    const Schedule empty;
    CHECK_EQ(merge_schedules(a, empty), a);
    CHECK_EQ(merge_schedules(empty, a), a);
    CHECK(merge_schedules(empty, empty).empty());
}

TEST(1, "non-overlapping reservations stay separate") {
    const Schedule a = {{1, 0, 2}};
    const Schedule b = {{3, 5, 7}};
    CHECK_EQ(merge_schedules(a, b), Schedule({{1, 0, 2}, {3, 5, 7}}));
    CHECK_EQ(merge_schedules(b, a), Schedule({{1, 0, 2}, {3, 5, 7}}));
}

TEST(1, "overlapping reservations add their machines") {
    const Schedule a = {{1, 0, 4}};
    const Schedule b = {{2, 2, 6}};
    CHECK_EQ(merge_schedules(a, b), Schedule({{1, 0, 2}, {3, 2, 4}, {2, 4, 6}}));
}

TEST(1, "one reservation spanning several others") {
    const Schedule a = {{1, 0, 10}};
    const Schedule b = {{1, 2, 4}, {1, 6, 8}};
    CHECK_EQ(merge_schedules(a, b),
             Schedule({{1, 0, 2}, {2, 2, 4}, {1, 4, 6}, {2, 6, 8}, {1, 8, 10}}));
}

TEST(1, "equal adjacent pieces are coalesced") {
    const Schedule a = {{1, 0, 2}};
    const Schedule b = {{1, 2, 3}};
    CHECK_EQ(merge_schedules(a, b), Schedule({{1, 0, 3}}));
}

TEST(1, "identical schedules double every count") {
    const Schedule a = {{1, 0, 3}, {2, 5, 9}};
    CHECK_EQ(merge_schedules(a, a), Schedule({{2, 0, 3}, {4, 5, 9}}));
}

TEST(1, "reservations starting and ending together") {
    const Schedule a = {{1, 2, 5}};
    const Schedule b = {{3, 2, 5}};
    CHECK_EQ(merge_schedules(a, b), Schedule({{4, 2, 5}}));
}

TEST(1, "a boundary of a falls exactly on a boundary of b") {
    const Schedule a = {{1, 0, 2}, {2, 2, 4}};
    const Schedule b = {{5, 2, 3}};
    CHECK_EQ(merge_schedules(a, b), Schedule({{1, 0, 2}, {7, 2, 3}, {2, 3, 4}}));
}

TEST(1, "random schedules match a brute-force merge") {
    Rng rng;
    for (int round = 0; round < 3'000; ++round) {
        const auto first = random_jobs(rng, static_cast<std::size_t>(rng.uniform(0, 25)), 40);
        const auto second = random_jobs(rng, static_cast<std::size_t>(rng.uniform(0, 25)), 40);
        auto both = first;
        both.insert(both.end(), second.begin(), second.end());
        const Schedule a = brute_schedule(first);
        const Schedule b = brute_schedule(second);
        INFO("round ", round);
        const Schedule merged = merge_schedules(a, b);
        check_valid_schedule(merged);
        REQUIRE_EQ(merged, brute_schedule(both));
    }
}

// Quadratic merging would need trillions of steps at this size.
PERF(1, "merging millions of pieces is linear", 1500) {
    const std::int64_t pieces = 2'000'000;
    Schedule a;
    Schedule b;
    a.reserve(static_cast<std::size_t>(pieces));
    b.reserve(static_cast<std::size_t>(pieces));
    for (std::int64_t i = 0; i < pieces; ++i) {
        a.push_back({1 + i % 2, 3 * i, 3 * i + 2});
        b.push_back({1 + i % 3, 3 * i + 1, 3 * i + 3});
    }
    const Schedule merged = merge_schedules(a, b);
    CHECK_GT(merged.size(), 1'000'000);
}

TEST(2, "no jobs means an empty schedule") {
    CHECK(satisfying_schedule({}).empty());
}

TEST(2, "a single job") {
    const std::vector<JobRequest> jobs = {{3, 9}};
    CHECK_EQ(satisfying_schedule(jobs), Schedule({{1, 3, 9}}));
}

TEST(2, "invalid jobs throw") {
    const std::vector<JobRequest> jobs = {{3, 3}};
    CHECK_THROWS_AS(satisfying_schedule(jobs), std::invalid_argument);
    const std::vector<JobRequest> backwards = {{1, 5}, {7, 2}};
    CHECK_THROWS_AS(satisfying_schedule(backwards), std::invalid_argument);
}

TEST(2, "the README example") {
    const std::vector<JobRequest> jobs = {{0, 2}, {1, 3}, {2, 6}};
    CHECK_EQ(satisfying_schedule(jobs), Schedule({{1, 0, 1}, {2, 1, 3}, {1, 3, 6}}));
}

TEST(2, "contiguous jobs coalesce into one reservation") {
    const std::vector<JobRequest> jobs = {{0, 1}, {1, 2}, {2, 3}};
    CHECK_EQ(satisfying_schedule(jobs), Schedule({{1, 0, 3}}));
}

TEST(2, "identical jobs stack") {
    const std::vector<JobRequest> jobs = {{1, 4}, {1, 4}, {1, 4}};
    CHECK_EQ(satisfying_schedule(jobs), Schedule({{3, 1, 4}}));
}

TEST(2, "random job sets match a sweep") {
    Rng rng;
    for (int round = 0; round < 2'000; ++round) {
        const auto jobs = random_jobs(rng, static_cast<std::size_t>(rng.uniform(0, 60)), 50);
        INFO("round ", round);
        const Schedule schedule = satisfying_schedule(jobs);
        check_valid_schedule(schedule);
        REQUIRE_EQ(schedule, brute_schedule(jobs));
    }
}

TEST(2, "times may be large and may be negative") {
    const std::vector<JobRequest> jobs = {
        {-1'000'000'000'000, 5},
        {0, 1'000'000'000'000},
        {-1'000'000'000'000, -1},
    };
    CHECK_EQ(satisfying_schedule(jobs),
             Schedule({{2, -1'000'000'000'000, -1},
                       {1, -1, 0},
                       {2, 0, 5},
                       {1, 5, 1'000'000'000'000}}));
}

// Inserting jobs one at a time into a flat schedule is about n^2.
PERF(2, "a quarter million random jobs", 2000) {
    Rng rng;
    const auto jobs = random_jobs(rng, 250'000, 1'000'000);
    const Schedule schedule = satisfying_schedule(jobs);
    CHECK_GT(schedule.size(), 0);
}

TEST(3, "an empty planner") {
    IncrementalPlanner planner;
    CHECK_EQ(planner.size(), 0);
    CHECK(planner.schedule().empty());
    CHECK_EQ(planner.peak(), 0);
}

TEST(3, "adding jobs one at a time") {
    IncrementalPlanner planner;
    planner.add({0, 2});
    planner.add({1, 3});
    planner.add({2, 6});
    CHECK_EQ(planner.size(), 3);
    CHECK_EQ(planner.schedule(), Schedule({{1, 0, 1}, {2, 1, 3}, {1, 3, 6}}));
    CHECK_EQ(planner.peak(), 2);
}

TEST(3, "invalid jobs are rejected") {
    IncrementalPlanner planner;
    CHECK_THROWS_AS(planner.add({5, 5}), std::invalid_argument);
    CHECK_EQ(planner.size(), 0);
}

TEST(3, "the schedule can be observed between additions") {
    Rng rng;
    IncrementalPlanner planner;
    std::vector<JobRequest> jobs;
    for (int step = 0; step < 600; ++step) {
        const auto batch = random_jobs(rng, 1, 30);
        planner.add(batch[0]);
        jobs.push_back(batch[0]);
        INFO("step ", step);
        const Schedule expected = brute_schedule(jobs);
        REQUIRE_EQ(planner.schedule(), expected);
        std::int64_t peak = 0;
        for (const Reservation& piece : expected) {
            peak = std::max(peak, piece.machines);
        }
        REQUIRE_EQ(planner.peak(), peak);
        REQUIRE_EQ(planner.size(), jobs.size());
    }
}

// Recomputing or re-merging everything on every add would take far too long.
PERF(3, "adds are polylogarithmic amortized", 2500) {
    Rng rng;
    IncrementalPlanner planner;
    const auto jobs = random_jobs(rng, 400'000, 2'000'000);
    for (const JobRequest& job : jobs) {
        planner.add(job);
    }
    const Schedule schedule = planner.schedule();
    CHECK_GT(schedule.size(), 0);
    CHECK_EQ(planner.size(), jobs.size());
}
