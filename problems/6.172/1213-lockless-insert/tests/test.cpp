#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "08_dsched.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
namespace dsched = practice::dsched;

namespace {

using Value = std::optional<std::uint64_t>;
using SchedTable = LocklessTable<dsched::Atomic>;

constexpr int kWriters = 4;
constexpr std::uint64_t kKeysPerWriter = 300;

std::uint64_t key_for(int writer, std::uint64_t i) {
    return i * static_cast<std::uint64_t>(kWriters) + static_cast<std::uint64_t>(writer);
}

std::string text_for(std::uint64_t key) { return "payload-" + std::to_string(key) + std::string(64, 'x'); }

// A scenario where one thread finishes an insert while another is mid-insert on the same chain.
dsched::Report explore_no_false_negative() {
    return dsched::explore_dfs([] {
        auto table = std::make_shared<SchedTable>(8);
        auto found = std::make_shared<Value>();
        dsched::Scenario scenario;
        scenario.threads = {[table] { table->insert(1, 111); },
                            [table, found] {
                                table->insert(9, 999);   // same home slot as key 1
                                *found = table->find(9);
                            }};
        scenario.check = [table, found] {
            return *found == Value(999) && table->find(1) == Value(111);
        };
        return scenario;
    });
}

}  // namespace

TEST(1, "insert and find round-trip") {
    LocklessTable<> table(16);
    CHECK(table.insert(3, 30));
    CHECK_EQ(table.find(3), Value(30));
    CHECK(!table.find(4).has_value());
}

TEST(1, "colliding keys probe past each other") {
    LocklessTable<> table(8);
    CHECK(table.insert(1, 10));
    CHECK(table.insert(9, 90));
    CHECK(table.insert(17, 170));
    CHECK_EQ(table.find(1), Value(10));
    CHECK_EQ(table.find(9), Value(90));
    CHECK_EQ(table.find(17), Value(170));
}

TEST(1, "probing wraps past the end of the table") {
    LocklessTable<> table(4);
    CHECK(table.insert(3, 30));
    CHECK(table.insert(7, 70));   // home 3 is taken, wraps to slot 0
    CHECK_EQ(table.find(7), Value(70));
    CHECK_EQ(table.find(3), Value(30));
}

TEST(1, "inserting an existing key overwrites its value") {
    LocklessTable<> table(8);
    CHECK(table.insert(5, 50));
    CHECK(table.insert(5, 51));
    CHECK_EQ(table.find(5), Value(51));
}

TEST(1, "a full table refuses a new key") {
    LocklessTable<> table(4);
    for (std::uint64_t key = 1; key <= 4; ++key) {
        CHECK(table.insert(key, key));
    }
    CHECK(!table.insert(9, 9));
    CHECK_EQ(table.find(4), Value(4));
    CHECK(!table.find(9).has_value());
}

TEST(1, "random operations match a model") {
    Rng rng;
    LocklessTable<> table(1024);
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

TEST(2, "two colliding inserts both land in every interleaving") {
    const auto report = dsched::explore_dfs([] {
        auto table = std::make_shared<SchedTable>(8);
        dsched::Scenario scenario;
        scenario.threads = {[table] { CHECK(table->insert(1, 111)); },
                            [table] { CHECK(table->insert(9, 999)); }};
        scenario.check = [table] {
            return table->find(1) == Value(111) && table->find(9) == Value(999);
        };
        return scenario;
    });
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(2, "a reader sees nothing or the whole value") {
    const auto report = dsched::explore_dfs([] {
        auto table = std::make_shared<SchedTable>(8);
        auto seen = std::make_shared<Value>();
        dsched::Scenario scenario;
        scenario.threads = {[table] { table->insert(1, 111); },
                            [table, seen] { *seen = table->find(1); }};
        scenario.check = [seen] { return !seen->has_value() || *seen == Value(111); };
        return scenario;
    });
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(2, "a completed insert is found past a half-written slot") {
    const auto report = explore_no_false_negative();
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(2, "a reader that sees a neighbour mid-write still finds its own key later") {
    const auto report = dsched::explore_dfs([] {
        auto table = std::make_shared<SchedTable>(8);
        table->insert(9, 999);  // a complete entry sits further along the probe chain
        auto seen = std::make_shared<Value>();
        dsched::Scenario scenario;
        scenario.threads = {[table] { table->insert(1, 111); },
                            [table, seen] { *seen = table->find(9); }};
        scenario.check = [seen] { return *seen == Value(999); };
        return scenario;
    });
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(2, "real threads insert concurrently without losing keys") {
    REQUIRE(explore_no_false_negative().clean());
    LocklessTable<> table(4096);
    std::vector<std::thread> threads;
    for (int w = 0; w < kWriters; ++w) {
        threads.emplace_back([&table, w] {
            for (std::uint64_t i = 1; i <= kKeysPerWriter; ++i) {
                CHECK(table.insert(key_for(w, i), i));
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    for (int w = 0; w < kWriters; ++w) {
        for (std::uint64_t i = 1; i <= kKeysPerWriter; ++i) {
            REQUIRE_EQ(table.find(key_for(w, i)), Value(i));
        }
    }
}

TEST(3, "published text round-trips") {
    PublishedTable<> table(16);
    CHECK(table.insert(3, "hello"));
    CHECK_EQ(table.find(3), std::optional<std::string>("hello"));
    CHECK(!table.find(4).has_value());
}

TEST(3, "an existing key is left unchanged") {
    PublishedTable<> table(16);
    CHECK(table.insert(3, "first"));
    CHECK(!table.insert(3, "second"));
    CHECK_EQ(table.find(3), std::optional<std::string>("first"));
}

TEST(3, "collisions and wraparound work for published text") {
    PublishedTable<> table(4);
    CHECK(table.insert(3, "a"));
    CHECK(table.insert(7, "b"));
    CHECK_EQ(table.find(7), std::optional<std::string>("b"));
    CHECK_EQ(table.find(3), std::optional<std::string>("a"));
    CHECK(table.insert(11, "c"));
    CHECK(table.insert(15, "d"));
    CHECK(!table.insert(19, "e"));
}

TEST(3, "readers polling during inserts only see complete text") {
    PublishedTable<> table(4096);
    // Writers wait for the readers through relaxed counters only, so the two groups overlap
    // and this coordination adds no synchronization edge that could hide a missing release.
    constexpr int kReaders = 3;
    std::atomic<int> readers_running{0};
    std::atomic<bool> writers_done{false};
    std::vector<std::thread> threads;
    for (int w = 0; w < kWriters; ++w) {
        threads.emplace_back([&table, &readers_running, w] {
            while (readers_running.load(std::memory_order_relaxed) < kReaders) {
                std::this_thread::yield();
            }
            for (std::uint64_t i = 1; i <= kKeysPerWriter; ++i) {
                const std::uint64_t key = key_for(w, i);
                CHECK(table.insert(key, text_for(key)));
            }
        });
    }
    for (int r = 0; r < kReaders; ++r) {
        threads.emplace_back([&table, &writers_done, &readers_running, r] {
            Rng rng(static_cast<std::uint64_t>(r) + 1234);
            readers_running.fetch_add(1, std::memory_order_relaxed);
            // Keep polling after the writers stop: reads of published text are what TSan checks.
            long found = 0;
            long polls_after_done = 0;
            while (polls_after_done < 200'000 &&
                   (!writers_done.load(std::memory_order_relaxed) || found < 400)) {
                if (writers_done.load(std::memory_order_relaxed)) {
                    ++polls_after_done;
                }
                const auto key = key_for(static_cast<int>(rng.uniform(0, kWriters - 1)),
                                         static_cast<std::uint64_t>(rng.uniform(1, kKeysPerWriter)));
                if (const auto text = table.find(key)) {
                    ++found;
                    CHECK_EQ(*text, text_for(key));
                }
            }
        });
    }
    for (int i = 0; i < kWriters; ++i) {
        threads[static_cast<std::size_t>(i)].join();
    }
    writers_done.store(true, std::memory_order_relaxed);
    for (std::size_t i = kWriters; i < threads.size(); ++i) {
        threads[i].join();
    }
    for (int w = 0; w < kWriters; ++w) {
        for (std::uint64_t i = 1; i <= kKeysPerWriter; ++i) {
            const std::uint64_t key = key_for(w, i);
            REQUIRE_EQ(table.find(key), std::optional<std::string>(text_for(key)));
        }
    }
}
