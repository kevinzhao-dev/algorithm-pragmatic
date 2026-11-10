#include <atomic>
#include <memory>
#include <numeric>
#include <set>
#include <thread>
#include <vector>

#include "08_dsched.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
namespace dsched = practice::dsched;

namespace {

using SchedBank = Bank<dsched::Mutex>;

std::int64_t total_of(const SchedBank& bank) {
    std::int64_t total = 0;
    for (std::size_t i = 0; i < bank.accounts(); ++i) {
        total += bank.balance(i);
    }
    return total;
}

}  // namespace

TEST(1, "a transfer moves money and reports success") {
    Bank<> bank({100, 50});
    CHECK(bank.transfer(0, 1, 30));
    CHECK_EQ(bank.balance(0), 70);
    CHECK_EQ(bank.balance(1), 80);
    CHECK_EQ(bank.accounts(), 2);
}

TEST(1, "an overdraft is refused and leaves balances unchanged") {
    Bank<> bank({10, 0});
    CHECK(!bank.transfer(0, 1, 11));
    CHECK_EQ(bank.balance(0), 10);
    CHECK_EQ(bank.balance(1), 0);
}

TEST(1, "transferring to oneself is a no-op, not a self-deadlock") {
    Bank<> bank({10});
    CHECK(bank.transfer(0, 0, 5));
    CHECK_EQ(bank.balance(0), 10);
}

namespace {

// Exhaustive check that opposite transfers cannot deadlock; real-thread tests depend on it.
dsched::Report opposite_transfers_report() {
    return dsched::explore_dfs([] {
        auto bank = std::make_shared<SchedBank>(std::vector<std::int64_t>{10, 10});
        dsched::Scenario scenario;
        scenario.threads = {[bank] { bank->transfer(0, 1, 3); }, [bank] { bank->transfer(1, 0, 7); }};
        scenario.check = [bank] { return total_of(*bank) == 20; };
        return scenario;
    });
}

}  // namespace

TEST(1, "two opposite transfers never deadlock under any interleaving") {
    const auto report = opposite_transfers_report();
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK_EQ(report.deadlocks, 0);
    CHECK_EQ(report.failures, 0);
}

TEST(1, "a cycle of three transfers never deadlocks") {
    const auto report = dsched::explore_dfs(
        [] {
            auto bank = std::make_shared<SchedBank>(std::vector<std::int64_t>{5, 5, 5});
            dsched::Scenario scenario;
            scenario.threads = {[bank] { bank->transfer(0, 1, 1); },
                                [bank] { bank->transfer(1, 2, 1); },
                                [bank] { bank->transfer(2, 0, 1); }};
            scenario.check = [bank] { return total_of(*bank) == 15; };
            return scenario;
        },
        {.max_preemptions = 3});
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK_EQ(report.deadlocks, 0);
    CHECK_EQ(report.failures, 0);
}

TEST(1, "real threads conserve the total and stay race free") {
    // A schedule-dependent deadlock would hang real threads, so require the exhaustive check first.
    REQUIRE_EQ(opposite_transfers_report().deadlocks, 0);
    const int accounts = 16;
    Bank<> bank(std::vector<std::int64_t>(accounts, 1000));
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&bank, t] {
            Rng rng(static_cast<std::uint64_t>(t) + 77);
            for (int i = 0; i < 4000; ++i) {
                const auto from = static_cast<std::size_t>(rng.uniform(0, accounts - 1));
                const auto to = static_cast<std::size_t>(rng.uniform(0, accounts - 1));
                bank.transfer(from, to, rng.uniform(0, 20));
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    std::int64_t total = 0;
    for (std::size_t i = 0; i < bank.accounts(); ++i) {
        CHECK_GE(bank.balance(i), 0);
        total += bank.balance(i);
    }
    CHECK_EQ(total, accounts * 1000);
}

TEST(2, "a meal runs with both forks held") {
    DiningTable<> table(3);
    int meals = 0;
    table.eat(0, [&] { ++meals; });
    table.eat(1, [&] { ++meals; });
    table.eat(2, [&] { ++meals; });
    CHECK_EQ(meals, 3);
}

TEST(2, "neighbours never eat at the same time and nobody deadlocks") {
    const auto report = dsched::explore_dfs(
        [] {
            auto table = std::make_shared<DiningTable<dsched::Mutex>>(3);
            auto eating = std::make_shared<std::array<dsched::Atomic<int>, 3>>();
            auto violations = std::make_shared<int>(0);
            dsched::Scenario scenario;
            for (std::size_t p = 0; p < 3; ++p) {
                scenario.threads.push_back([=] {
                    table->eat(p, [&] {
                        const std::size_t left = (p + 2) % 3;
                        const std::size_t right = (p + 1) % 3;
                        (*eating)[p].store(1);
                        if ((*eating)[left].load() == 1 || (*eating)[right].load() == 1) {
                            ++*violations;
                        }
                        dsched::yield();
                        (*eating)[p].store(0);
                    });
                });
            }
            scenario.check = [violations] { return *violations == 0; };
            return scenario;
        },
        {.max_preemptions = 3});
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK_EQ(report.deadlocks, 0);
    CHECK_EQ(report.failures, 0);
}

TEST(2, "two philosophers sharing all forks still finish") {
    const auto report = dsched::explore_dfs([] {
        auto table = std::make_shared<DiningTable<dsched::Mutex>>(2);
        auto meals = std::make_shared<int>(0);
        dsched::Scenario scenario;
        for (std::size_t p = 0; p < 2; ++p) {
            scenario.threads.push_back([=] {
                table->eat(p, [&] { ++*meals; });
                table->eat(p, [&] { ++*meals; });
            });
        }
        scenario.check = [meals] { return *meals == 4; };
        return scenario;
    });
    INFO("bad schedule ", report.bad_trace);
    CHECK_EQ(report.deadlocks, 0);
    CHECK_EQ(report.failures, 0);
}

TEST(3, "locks are held for the rest of a transaction and released at the end") {
    LockTable<dsched::Mutex> table(3);
    int attempts = 0;
    table.run([&](auto& txn) {
        ++attempts;
        txn.access(1);
        txn.access(2);
        txn.access(0);
    });
    CHECK_EQ(attempts, 1);  // nothing else is running, so no conflict and no restart

    // Everything is free again afterwards.
    int again = 0;
    table.run([&](auto& txn) {
        ++again;
        txn.access(0);
        txn.access(1);
        txn.access(2);
    });
    CHECK_EQ(again, 1);
}

TEST(3, "repeated access to the same lock is a no-op") {
    LockTable<dsched::Mutex> table(2);
    table.run([](auto& txn) {
        txn.access(1);
        txn.access(1);
        txn.access(0);
        txn.access(1);
    });
}

TEST(3, "an exception inside the body still releases the locks") {
    LockTable<dsched::Mutex> table(2);
    CHECK_THROWS_AS(table.run([](auto& txn) {
                        txn.access(0);
                        throw std::runtime_error("boom");
                    }),
                    std::runtime_error);
    int ok = 0;
    table.run([&](auto& txn) {
        ++ok;
        txn.access(0);
    });
    CHECK_EQ(ok, 1);
}

namespace {

dsched::Report out_of_order_report() {
    return dsched::explore_dfs(
        [] {
            auto table = std::make_shared<LockTable<dsched::Mutex>>(2);
            auto counters = std::make_shared<std::array<dsched::Atomic<int>, 2>>();
            auto attempts = std::make_shared<int>(0);
            dsched::Scenario scenario;
            const auto worker = [=](std::size_t first, std::size_t second) {
                return [=] {
                    table->run([&](auto& txn) {
                        ++*attempts;
                        txn.access(first);
                        txn.access(second);
                        // Both locks are held here, so the check-and-increment is safe.
                        const int a = (*counters)[0].load();
                        const int b = (*counters)[1].load();
                        (*counters)[0].store(a + 1);
                        (*counters)[1].store(b + 1);
                    });
                };
            };
            scenario.threads = {worker(0, 1), worker(1, 0)};
            scenario.check = [counters, attempts] {
                return (*counters)[0].load() == 2 && (*counters)[1].load() == 2 && *attempts <= 6;
            };
            return scenario;
        },
        {.max_preemptions = 4, .max_runs = 40'000});
}

}  // namespace

TEST(3, "out-of-order access never deadlocks and always gets both locks") {
    const auto report = out_of_order_report();
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK_EQ(report.deadlocks, 0);
    CHECK_EQ(report.failures, 0);
    CHECK(report.truncated == 0);
}

TEST(3, "transactions under real threads stay race free") {
    REQUIRE_EQ(out_of_order_report().deadlocks, 0);  // a deadlock here would hang real threads
    const std::size_t locks = 8;
    LockTable<> table(locks);
    std::vector<int> counters(locks, 0);
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&, t] {
            Rng rng(static_cast<std::uint64_t>(t) + 11);
            for (int i = 0; i < 1500; ++i) {
                std::array<std::size_t, 3> picks;
                for (auto& pick : picks) {
                    pick = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(locks) - 1));
                }
                table.run([&](auto& txn) {
                    for (const std::size_t pick : picks) {
                        txn.access(pick);
                    }
                    for (const std::size_t pick : picks) {
                        ++counters[pick];
                    }
                });
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    CHECK_EQ(std::accumulate(counters.begin(), counters.end(), 0), 4 * 1500 * 3);
}
