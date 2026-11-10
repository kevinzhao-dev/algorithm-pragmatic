#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "00_alloc_counter.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice6006::AllocationScope;

namespace {

// (longest, count) computed by looking at every maximal run directly.
std::pair<std::size_t, std::size_t> brute_force(const std::vector<int>& values) {
    std::size_t longest = 0;
    std::size_t count = 0;
    std::size_t i = 0;
    while (i < values.size()) {
        std::size_t j = i + 1;
        while (j < values.size() && values[j - 1] < values[j]) {
            ++j;
        }
        const std::size_t length = j - i;
        if (length > longest) {
            longest = length;
            count = 1;
        } else if (length == longest) {
            ++count;
        }
        i = j;
    }
    return {longest, count};
}

}  // namespace

TEST(1, "an empty array has no runs") {
    const std::vector<int> empty;
    CHECK_EQ(count_longest_runs<int>(empty), 0);
}

TEST(1, "a single element is one run of length one") {
    const std::vector<int> values = {42};
    CHECK_EQ(count_longest_runs<int>(values), 1);
}

TEST(1, "strictly increasing is one long run") {
    const std::vector<int> values = {1, 2, 3, 4, 5};
    CHECK_EQ(count_longest_runs<int>(values), 1);
}

TEST(1, "strictly decreasing makes every element its own longest run") {
    const std::vector<int> values = {5, 4, 3, 2, 1};
    CHECK_EQ(count_longest_runs<int>(values), 5);
}

TEST(1, "equal neighbors break a run") {
    const std::vector<int> values = {1, 2, 2, 3};
    CHECK_EQ(count_longest_runs<int>(values), 2);
    const std::vector<int> all_equal = {7, 7, 7};
    CHECK_EQ(count_longest_runs<int>(all_equal), 3);
}

TEST(1, "several tied longest runs are all counted") {
    const std::vector<int> values = {1, 5, 2, 6, 0, 3, 9, 4, 10, 11};
    CHECK_EQ(count_longest_runs<int>(values), 2);  // (0, 3, 9) and (4, 10, 11)
}

TEST(1, "a later longer run replaces earlier ties") {
    const std::vector<int> values = {1, 2, 1, 2, 3, 0, 1, 2, 3, 4};
    CHECK_EQ(count_longest_runs<int>(values), 1);
}

TEST(1, "the longest run can end at the very last element") {
    const std::vector<int> values = {3, 1, 2, 3, 4};
    CHECK_EQ(count_longest_runs<int>(values), 1);
}

TEST(1, "works with any ordered element type") {
    const std::vector<std::string> words = {"a", "b", "a", "c", "b"};
    CHECK_EQ(count_longest_runs<std::string>(words), 2);
}

TEST(1, "random arrays match a brute-force count") {
    Rng rng;
    for (int round = 0; round < 3'000; ++round) {
        const auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 30)), 0, 12);
        INFO("round ", round);
        REQUIRE_EQ(count_longest_runs<int>(values), brute_force(values).second);
    }
}

PERF(1, "a single linear scan", 500) {
    Rng rng;
    const auto values = rng.ints(20'000'000, 0, 1'000'000);
    CHECK_GT(count_longest_runs<int>(values), 0);
}

TEST(2, "a fresh tracker reports nothing") {
    RunTracker<int> tracker;
    CHECK_EQ(tracker.longest(), 0);
    CHECK_EQ(tracker.count(), 0);
    CHECK_EQ(tracker.current(), 0);
}

TEST(2, "pushes update current, longest and count") {
    RunTracker<int> tracker;
    tracker.push(5);
    CHECK_EQ(tracker.current(), 1);
    CHECK_EQ(tracker.longest(), 1);
    CHECK_EQ(tracker.count(), 1);

    tracker.push(6);
    CHECK_EQ(tracker.current(), 2);
    CHECK_EQ(tracker.longest(), 2);
    CHECK_EQ(tracker.count(), 1);

    tracker.push(1);
    CHECK_EQ(tracker.current(), 1);
    CHECK_EQ(tracker.longest(), 2);
    CHECK_EQ(tracker.count(), 1);

    tracker.push(2);
    CHECK_EQ(tracker.current(), 2);
    CHECK_EQ(tracker.longest(), 2);
    CHECK_EQ(tracker.count(), 2);
}

TEST(2, "after every push the tracker matches a brute-force prefix scan") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        RunTracker<int> tracker;
        std::vector<int> prefix;
        for (int step = 0; step < 40; ++step) {
            const int value = static_cast<int>(rng.uniform(0, 8));
            prefix.push_back(value);
            tracker.push(value);
            const auto [longest, count] = brute_force(prefix);
            INFO("round ", round, " step ", step);
            REQUIRE_EQ(tracker.longest(), longest);
            REQUIRE_EQ(tracker.count(), count);
        }
    }
}

TEST(2, "a custom comparator tracks decreasing runs") {
    RunTracker<int, std::greater<int>> tracker;
    for (int value : {9, 5, 1, 6, 2}) {
        tracker.push(value);
    }
    CHECK_EQ(tracker.longest(), 3);
    CHECK_EQ(tracker.count(), 1);
    CHECK_EQ(tracker.current(), 2);
}

TEST(2, "pushing never allocates") {
    Rng rng;
    const auto values = rng.ints(200'000, 0, 100);
    RunTracker<int> tracker;
    const AllocationScope scope;
    for (int value : values) {
        tracker.push(value);
    }
    CHECK_EQ(scope.allocations(), 0);
    CHECK_GT(tracker.longest(), 0);
}

PERF(2, "ten million pushes", 400) {
    RunTracker<std::int64_t> tracker;
    Rng rng;
    for (int i = 0; i < 10'000'000; ++i) {
        tracker.push(rng.uniform(0, 1'000'000));
    }
    CHECK_GT(tracker.longest(), 0);
}

TEST(3, "the alarm fires exactly once when the run reaches the threshold") {
    StreakAlarm<int> alarm(3);
    CHECK(!alarm.observe(1));
    CHECK(!alarm.observe(2));
    CHECK(alarm.observe(3));
    CHECK(!alarm.observe(4));
    CHECK(!alarm.observe(5));
}

TEST(3, "a new run can fire again") {
    StreakAlarm<int> alarm(2);
    CHECK(!alarm.observe(1));
    CHECK(alarm.observe(2));
    CHECK(!alarm.observe(2));  // equal breaks the run; 2 starts a new run of length 1
    CHECK(alarm.observe(3));
    CHECK(!alarm.observe(0));
    CHECK(alarm.observe(1));
}

TEST(3, "threshold one fires on every run start") {
    StreakAlarm<int> alarm(1);
    CHECK(alarm.observe(5));
    CHECK(!alarm.observe(6));
    CHECK(alarm.observe(1));
    CHECK(alarm.observe(1));
}

TEST(3, "threshold zero throws invalid_argument") {
    CHECK_THROWS_AS(StreakAlarm<int>(0), std::invalid_argument);
}

TEST(3, "random stream matches a run-length model") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const auto threshold = static_cast<std::size_t>(rng.uniform(1, 6));
        StreakAlarm<int> alarm(threshold);
        int previous = -1;
        std::size_t run = 0;
        bool have_previous = false;
        for (int step = 0; step < 200; ++step) {
            const int value = static_cast<int>(rng.uniform(0, 5));
            run = (have_previous && previous < value) ? run + 1 : 1;
            previous = value;
            have_previous = true;
            INFO("round ", round, " step ", step);
            REQUIRE_EQ(alarm.observe(value), run == threshold);
        }
    }
}

TEST(3, "observing never allocates") {
    Rng rng;
    const auto values = rng.ints(100'000, 0, 100);
    StreakAlarm<int> alarm(4);
    const AllocationScope scope;
    int fired = 0;
    for (int value : values) {
        fired += alarm.observe(value) ? 1 : 0;
    }
    CHECK_EQ(scope.allocations(), 0);
    CHECK_GT(fired, 0);
}
