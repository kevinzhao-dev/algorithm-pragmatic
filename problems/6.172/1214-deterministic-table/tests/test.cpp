#include <algorithm>
#include <memory>
#include <numeric>
#include <set>
#include <thread>
#include <vector>

#include "08_dsched.hpp"
#include "forkjoin.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
namespace dsched = practice::dsched;

namespace {

using Layout = std::vector<std::uint64_t>;
using SchedTable = DeterministicTable<dsched::Atomic>;

Layout serial_layout(std::size_t capacity, const std::vector<std::uint64_t>& keys) {
    DeterministicTable<> table(capacity);
    for (const std::uint64_t key : keys) {
        table.insert(key);
    }
    return table.layout();
}

std::vector<std::uint64_t> random_distinct_keys(std::size_t count, Rng& rng) {
    std::set<std::uint64_t> keys;
    while (keys.size() < count) {
        keys.insert(static_cast<std::uint64_t>(rng.uniform(1, 1'000'000)));
    }
    return {keys.begin(), keys.end()};
}

// Every interleaving of these concurrent inserts must end in the same canonical layout.
dsched::Report explore_inserts(std::size_t capacity,
                               std::vector<std::vector<std::uint64_t>> per_thread,
                               dsched::Options options = {}) {
    std::vector<std::uint64_t> all;
    for (const auto& keys : per_thread) {
        all.insert(all.end(), keys.begin(), keys.end());
    }
    const Layout expected = serial_layout(capacity, all);
    return dsched::explore_dfs(
        [=] {
            auto table = std::make_shared<SchedTable>(capacity);
            dsched::Scenario scenario;
            for (const auto& keys : per_thread) {
                scenario.threads.push_back([table, keys] {
                    for (const std::uint64_t key : keys) {
                        table->insert(key);
                    }
                });
            }
            scenario.check = [table, expected] { return table->layout() == expected; };
            return scenario;
        },
        options);
}

}  // namespace

TEST(1, "a key lands in its home slot when the table is empty") {
    DeterministicTable<> table(8);
    table.insert(3);
    CHECK_EQ(table.layout(), Layout({0, 0, 0, 3, 0, 0, 0, 0}));
    CHECK(table.contains(3));
    CHECK(!table.contains(11));
}

TEST(1, "colliding keys are kept in descending order along the probe") {
    DeterministicTable<> table(8);
    table.insert(1);
    table.insert(17);
    table.insert(9);
    CHECK_EQ(table.layout(), Layout({0, 17, 9, 1, 0, 0, 0, 0}));
}

TEST(1, "a displaced key keeps probing from the next slot") {
    DeterministicTable<> table(8);
    table.insert(2);   // home 2
    table.insert(10);  // home 2: takes the slot, 2 moves to slot 3
    table.insert(3);   // home 3: larger than 2, so 2 moves on to slot 4
    CHECK_EQ(table.layout(), Layout({0, 0, 10, 3, 2, 0, 0, 0}));
}

TEST(1, "probing wraps around the end of the table") {
    DeterministicTable<> table(4);
    table.insert(3);
    table.insert(7);
    CHECK_EQ(table.layout(), Layout({3, 0, 0, 7}));
    CHECK(table.contains(3));
    CHECK(table.contains(7));
}

TEST(1, "duplicates leave the table unchanged") {
    DeterministicTable<> table(8);
    table.insert(5);
    table.insert(13);
    const Layout before = table.layout();
    table.insert(5);
    table.insert(13);
    CHECK_EQ(table.layout(), before);
}

TEST(1, "every insertion order of a small set gives one layout") {
    std::vector<std::uint64_t> keys{1, 9, 17, 2, 10, 7};
    std::sort(keys.begin(), keys.end());
    const Layout expected = serial_layout(16, keys);
    do {
        REQUIRE_EQ(serial_layout(16, keys), expected);
    } while (std::next_permutation(keys.begin(), keys.end()));
}

TEST(1, "shuffled insertions of a large set give one layout") {
    Rng rng;
    auto keys = random_distinct_keys(600, rng);
    const Layout expected = serial_layout(1024, keys);
    for (int trial = 0; trial < 40; ++trial) {
        INFO("trial ", trial);
        rng.shuffle(keys);
        REQUIRE_EQ(serial_layout(1024, keys), expected);
    }
}

TEST(1, "lookup stops at the first smaller key") {
    DeterministicTable<> table(16);
    table.insert(17);  // home 1
    table.insert(33);  // home 1, sorted before 17
    table.insert(49);  // home 1, sorted before 33
    CHECK_EQ(table.lookup(49), (std::pair<bool, std::size_t>{true, 1}));
    CHECK_EQ(table.lookup(17), (std::pair<bool, std::size_t>{true, 3}));
    CHECK_EQ(table.lookup(65), (std::pair<bool, std::size_t>{false, 1}));  // larger than 49
    CHECK_EQ(table.lookup(1), (std::pair<bool, std::size_t>{false, 4}));   // runs off to the empty slot
}

TEST(1, "contains agrees with a set on random keys") {
    Rng rng;
    const auto keys = random_distinct_keys(500, rng);
    const DeterministicTable<> empty(4);
    CHECK(!empty.contains(1));
    DeterministicTable<> table(1024);
    for (const std::uint64_t key : keys) {
        table.insert(key);
    }
    const std::set<std::uint64_t> present(keys.begin(), keys.end());
    for (int step = 0; step < 5000; ++step) {
        const auto key = static_cast<std::uint64_t>(rng.uniform(1, 1'000'000));
        REQUIRE_EQ(table.contains(key), present.contains(key));
    }
}

TEST(2, "two colliding inserts end in the canonical layout in every interleaving") {
    const auto report = explore_inserts(8, {{1}, {9}});
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(2, "chains of displacement resolve the same way in every interleaving") {
    const auto report = explore_inserts(8, {{1, 17}, {9}});
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(2, "three threads colliding still end in the canonical layout") {
    const auto report = explore_inserts(8, {{1}, {9}, {17}}, {.max_preemptions = 3});
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(2, "the same key inserted twice concurrently appears once") {
    const auto report = explore_inserts(8, {{1, 9}, {9}});
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(2, "real threads produce the serial layout") {
    Rng rng;
    const auto keys = random_distinct_keys(2000, rng);
    const Layout expected = serial_layout(4096, keys);
    for (int round = 0; round < 8; ++round) {
        INFO("round ", round);
        DeterministicTable<> table(4096);
        std::vector<std::thread> threads;
        for (std::size_t t = 0; t < 4; ++t) {
            threads.emplace_back([&table, &keys, t] {
                for (std::size_t i = t; i < keys.size(); i += 4) {
                    table.insert(keys[i]);
                }
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }
        REQUIRE_EQ(table.layout(), expected);
    }
}

TEST(3, "parallel construction matches the serial layout for any worker count") {
    Rng rng;
    const auto keys = random_distinct_keys(3000, rng);
    const Layout expected = serial_layout(8192, keys);
    for (const unsigned workers : {1U, 2U, 4U}) {
        INFO("workers ", workers);
        practice::set_workers(workers);
        DeterministicTable<> table(8192);
        table.insert_all(keys);
        REQUIRE_EQ(table.layout(), expected);
    }
}

TEST(3, "an empty batch changes nothing") {
    practice::set_workers(2);
    DeterministicTable<> table(8);
    table.insert_all({});
    CHECK_EQ(table.layout(), Layout(8, 0));
}
