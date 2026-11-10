#include <memory>
#include <optional>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "08_dsched.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
namespace dsched = practice::dsched;

namespace {

using Value = std::optional<std::uint64_t>;

template <typename Table>
void check_basic_table(Table& table) {
    CHECK(table.insert(3, 30));
    CHECK(table.insert(11, 110));  // same home slot as 3 in a table of 8
    CHECK(table.insert(7, 70));
    CHECK(table.insert(15, 150));  // home 7, wraps around to slot 0
    CHECK_EQ(table.find(3), Value(30));
    CHECK_EQ(table.find(11), Value(110));
    CHECK_EQ(table.find(7), Value(70));
    CHECK_EQ(table.find(15), Value(150));
    CHECK(!table.find(5).has_value());
    CHECK(table.insert(3, 31));
    CHECK_EQ(table.find(3), Value(31));
}

template <typename Table>
void check_full_table(Table& table) {
    for (std::uint64_t key = 1; key <= 4; ++key) {
        CHECK(table.insert(key, key));
    }
    CHECK(!table.insert(9, 9));
    for (std::uint64_t key = 1; key <= 4; ++key) {
        CHECK_EQ(table.find(key), Value(key));
    }
    CHECK(!table.find(9).has_value());
}

// CoarseTable has no stripes, so it takes just the capacity.
template <typename Table>
std::shared_ptr<Table> make_small_table() {
    if constexpr (std::is_constructible_v<Table, std::size_t, std::size_t>) {
        return std::make_shared<Table>(std::size_t{8}, std::size_t{2});
    } else {
        return std::make_shared<Table>(std::size_t{8});
    }
}

// Two colliding inserts explored over every interleaving.
template <typename Table>
dsched::Report explore_colliding_inserts() {
    return dsched::explore_dfs([] {
        auto table = make_small_table<Table>();
        dsched::Scenario scenario;
        scenario.threads = {[table] { table->insert(1, 11); }, [table] { table->insert(9, 99); }};
        scenario.check = [table] {
            return table->find(1) == Value(11) && table->find(9) == Value(99);
        };
        return scenario;
    });
}

// Threads probing out of occupied slots 1 and 2 move between stripes in opposite directions.
template <typename Table>
dsched::Report explore_crossing_probes() {
    return dsched::explore_dfs([] {
        auto table = make_small_table<Table>();
        table->insert(1, 10);
        table->insert(2, 20);
        table->insert(3, 30);
        dsched::Scenario scenario;
        scenario.threads = {[table] { table->insert(9, 90); },    // home 1, must step to 2
                            [table] { table->insert(10, 100); }}; // home 2, must step to 3
        scenario.check = [table] {
            return table->find(9) == Value(90) && table->find(10) == Value(100);
        };
        return scenario;
    });
}

template <typename Table>
void hammer_with_real_threads(Table& table) {
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&table, t] {
            for (std::uint64_t i = 1; i <= 400; ++i) {
                const std::uint64_t key = i * 4 + static_cast<std::uint64_t>(t);
                CHECK(table.insert(key, key * 2));
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    for (int t = 0; t < 4; ++t) {
        for (std::uint64_t i = 1; i <= 400; ++i) {
            const std::uint64_t key = i * 4 + static_cast<std::uint64_t>(t);
            REQUIRE_EQ(table.find(key), Value(key * 2));
        }
    }
}

}  // namespace

TEST(1, "the coarse table probes, wraps and overwrites") {
    CoarseTable<> table(8);
    check_basic_table(table);
}

TEST(1, "a full coarse table refuses new keys") {
    CoarseTable<> table(4);
    check_full_table(table);
}

TEST(1, "random operations match a model") {
    Rng rng;
    CoarseTable<> table(1024);
    std::unordered_map<std::uint64_t, std::uint64_t> model;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        const auto key = static_cast<std::uint64_t>(rng.uniform(1, 700));
        if (rng.coin(0.6)) {
            const auto value = static_cast<std::uint64_t>(rng.uniform(1, 1000));
            REQUIRE(table.insert(key, value));
            model[key] = value;
        } else {
            const auto found = model.find(key);
            REQUIRE_EQ(table.find(key), found == model.end() ? Value() : Value(found->second));
        }
    }
}

TEST(1, "colliding inserts are safe under every interleaving") {
    const auto report = explore_colliding_inserts<CoarseTable<dsched::Mutex>>();
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(1, "real threads insert without a race") {
    CoarseTable<> table(4096);
    hammer_with_real_threads(table);
}

TEST(2, "the striped table probes, wraps and overwrites") {
    StripedTable<> table(8, 2);
    check_basic_table(table);
}

TEST(2, "a full striped table refuses new keys") {
    StripedTable<> table(4, 2);
    check_full_table(table);
}

TEST(2, "stripes are at least a cache line apart") {
    const StripedTable<> table(64, 8);
    for (std::size_t i = 0; i + 1 < 8; ++i) {
        INFO("stripe ", i);
        const auto a = reinterpret_cast<std::uintptr_t>(table.stripe_address(i));
        const auto b = reinterpret_cast<std::uintptr_t>(table.stripe_address(i + 1));
        CHECK_GE(b - a, 64);
        CHECK_EQ(a % 64, 0);
    }
}

TEST(2, "colliding inserts are safe under every interleaving") {
    const auto report = explore_colliding_inserts<StripedTable<dsched::Mutex>>();
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(2, "probes that cross stripes never deadlock when one lock is held at a time") {
    const auto report = explore_crossing_probes<StripedTable<dsched::Mutex>>();
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(2, "real threads insert into stripes without a race") {
    StripedTable<> table(4096, 16);
    hammer_with_real_threads(table);
}

TEST(3, "the fair table behaves like a hash table when alone") {
    FairTable<> table(8, 2);
    check_basic_table(table);
    FairTable<> full(4, 2);
    check_full_table(full);
}

TEST(3, "colliding inserts on the fair table finish in every interleaving") {
    const auto report = explore_colliding_inserts<FairTable<dsched::Mutex>>();
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(3, "hand-over-hand probing across two stripes can deadlock") {
    const auto report = explore_crossing_probes<FairTable<dsched::Mutex>>();
    INFO("runs ", report.runs, " deadlocks ", report.deadlocks);
    CHECK(report.exhausted);
    CHECK_GT(report.deadlocks, 0);
    CHECK_LT(report.deadlocks, report.runs);  // only some interleavings get stuck
    CHECK_EQ(report.failures, 0);
}

TEST(3, "the deadlocking schedule replays to the same deadlock") {
    const auto make = [] {
        auto table = std::make_shared<FairTable<dsched::Mutex>>(8, 2);
        table->insert(1, 10);
        table->insert(2, 20);
        table->insert(3, 30);
        dsched::Scenario scenario;
        scenario.threads = {[table] { table->insert(9, 90); }, [table] { table->insert(10, 100); }};
        return scenario;
    };
    const auto report = dsched::explore_dfs(make);
    REQUIRE(!report.bad_trace.empty());
    CHECK(dsched::replay(make, report.bad_trace).deadlocked);
}

TEST(4, "the ordered table behaves like a hash table when alone") {
    OrderedFairTable<> table(8, 2);
    check_basic_table(table);
    OrderedFairTable<> full(4, 2);
    check_full_table(full);
}

TEST(4, "the same crossing probes can no longer deadlock") {
    const auto report = explore_crossing_probes<OrderedFairTable<dsched::Mutex>>();
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(4, "probes that wrap around the stripes do not deadlock either") {
    const auto report = dsched::explore_dfs([] {
        auto table = std::make_shared<OrderedFairTable<dsched::Mutex>>(4, 2);
        table->insert(1, 10);
        table->insert(3, 30);
        dsched::Scenario scenario;
        scenario.threads = {[table] { table->insert(5, 50); },    // home 1, steps into stripe 0
                            [table] { table->insert(7, 70); }};   // home 3, wraps to stripe 0
        scenario.check = [table] {
            return table->find(5) == Value(50) && table->find(7) == Value(70);
        };
        return scenario;
    });
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(4, "real threads work once the deadlock is gone") {
    REQUIRE_EQ(explore_crossing_probes<OrderedFairTable<dsched::Mutex>>().deadlocks, 0);
    OrderedFairTable<> table(4096, 16);
    hammer_with_real_threads(table);
}
