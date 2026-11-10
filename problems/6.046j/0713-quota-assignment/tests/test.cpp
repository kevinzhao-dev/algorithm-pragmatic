#include <algorithm>
#include <functional>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Accepts = std::vector<std::vector<int>>;

namespace {

Accepts random_accepts(Rng& rng, int n, int m, int max_choices) {
    Accepts accepts(n);
    for (auto& list : accepts) {
        const int k = static_cast<int>(rng.uniform(0, max_choices));
        for (int i = 0; i < k; ++i) {
            list.push_back(static_cast<int>(rng.uniform(0, m - 1)));
        }
        std::sort(list.begin(), list.end());
        list.erase(std::unique(list.begin(), list.end()), list.end());
    }
    return accepts;
}

// Polynomial reference for the streaming test: Kuhn on unit slots.
int oracle_max_served(const Accepts& accepts, const std::vector<int>& quota) {
    std::vector<int> slot_of_resource;
    std::vector<std::vector<int>> slots(quota.size());
    int total = 0;
    for (std::size_t r = 0; r < quota.size(); ++r) {
        for (int k = 0; k < quota[r]; ++k) {
            slots[r].push_back(total++);
        }
    }
    std::vector<int> owner(static_cast<std::size_t>(total), -1);
    std::vector<char> seen;
    const std::function<bool(int)> augment = [&](int request) {
        for (int r : accepts[static_cast<std::size_t>(request)]) {
            for (int slot : slots[r]) {
                if (seen[slot]) {
                    continue;
                }
                seen[slot] = 1;
                if (owner[slot] == -1 || augment(owner[slot])) {
                    owner[slot] = request;
                    return true;
                }
            }
        }
        return false;
    };
    int served = 0;
    for (std::size_t i = 0; i < accepts.size(); ++i) {
        seen.assign(static_cast<std::size_t>(total), 0);
        served += augment(static_cast<int>(i)) ? 1 : 0;
    }
    return served;
}

int brute_max_served(const Accepts& accepts, const std::vector<int>& quota) {
    std::vector<int> left = quota;
    const std::function<int(std::size_t)> solve = [&](std::size_t i) -> int {
        if (i == accepts.size()) {
            return 0;
        }
        int best = solve(i + 1);
        for (int r : accepts[i]) {
            if (left[r] > 0) {
                --left[r];
                best = std::max(best, 1 + solve(i + 1));
                ++left[r];
            }
        }
        return best;
    };
    return solve(0);
}

void check_plan(const Plan& plan, const Accepts& accepts, const std::vector<int>& quota) {
    REQUIRE_EQ(plan.choice.size(), accepts.size());
    std::vector<int> used(quota.size(), 0);
    int rejected = 0;
    for (std::size_t i = 0; i < accepts.size(); ++i) {
        const int choice = plan.choice[i];
        if (choice == -1) {
            ++rejected;
            continue;
        }
        REQUIRE(choice >= 0 && choice < static_cast<int>(quota.size()));
        CHECK(std::find(accepts[i].begin(), accepts[i].end(), choice) != accepts[i].end());
        ++used[choice];
    }
    CHECK_EQ(rejected, plan.rejected);
    for (std::size_t r = 0; r < quota.size(); ++r) {
        CHECK_LE(used[r], quota[r]);
    }
}

}  // namespace

TEST(1, "no requests means no rejections") {
    const std::vector<int> quota{1, 2};
    const Plan plan = best_plan(Accepts{}, quota);
    CHECK(plan.choice.empty());
    CHECK_EQ(plan.rejected, 0);
}

TEST(1, "a request with no acceptable resource is rejected") {
    const Accepts accepts{{}};
    const std::vector<int> quota{3};
    const Plan plan = best_plan(accepts, quota);
    CHECK_EQ(plan.rejected, 1);
    CHECK_EQ(plan.choice, std::vector<int>{-1});
}

TEST(1, "quotas limit how many can be served") {
    const Accepts accepts{{0}, {0}, {0}};
    const std::vector<int> quota{2};
    const Plan plan = best_plan(accepts, quota);
    check_plan(plan, accepts, quota);
    CHECK_EQ(plan.rejected, 1);
}

TEST(1, "the flexible request yields to the picky one") {
    const Accepts accepts{{0, 1}, {0}};
    const std::vector<int> quota{1, 1};
    const Plan plan = best_plan(accepts, quota);
    check_plan(plan, accepts, quota);
    CHECK_EQ(plan.rejected, 0);
    CHECK_EQ(plan.choice[1], 0);
    CHECK_EQ(plan.choice[0], 1);
}

TEST(1, "long displacement chains are found") {
    Accepts accepts;
    const int n = 200;
    std::vector<int> quota(n, 1);
    for (int i = 0; i + 1 < n; ++i) {
        accepts.push_back({i, i + 1});
    }
    accepts.push_back({0});
    const Plan plan = best_plan(accepts, quota);
    check_plan(plan, accepts, quota);
    CHECK_EQ(plan.rejected, 0);
}

TEST(1, "random instances match exhaustive search") {
    Rng rng;
    for (int round = 0; round < 400; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        const int m = static_cast<int>(rng.uniform(1, 5));
        auto accepts = random_accepts(rng, n, m, m);
        std::vector<int> quota(m);
        for (auto& q : quota) {
            q = static_cast<int>(rng.uniform(0, 3));
        }
        INFO("round ", round);
        const Plan plan = best_plan(accepts, quota);
        check_plan(plan, accepts, quota);
        CHECK_EQ(n - plan.rejected, brute_max_served(accepts, quota));
    }
}

PERF(1, "tens of thousands of requests", 3000) {
    Rng rng;
    const int n = 15'000;
    const int m = 2000;
    auto accepts = random_accepts(rng, n, m, 6);
    std::vector<int> quota(m);
    for (auto& q : quota) {
        q = static_cast<int>(rng.uniform(1, 6));  // scarce: most searches must fail fast
    }
    const Plan plan = best_plan(accepts, quota);
    CHECK_GT(plan.rejected, 0);
}

TEST(2, "no shortage reports nothing") {
    const Accepts accepts{{0}};
    const std::vector<int> quota{1};
    const Shortage shortage = shortage_report(accepts, quota);
    CHECK(shortage.requests.empty());
    CHECK(shortage.resources.empty());
}

TEST(2, "two requests fighting over one unit") {
    const Accepts accepts{{0}, {0}, {1}};
    const std::vector<int> quota{1, 1};
    const Shortage shortage = shortage_report(accepts, quota);
    CHECK_EQ(shortage.requests, std::vector<int>{0, 1});
    CHECK_EQ(shortage.resources, std::vector<int>{0});
}

TEST(2, "a request with no resources is a shortage of size one") {
    const Accepts accepts{{}};
    const std::vector<int> quota{1};
    const Shortage shortage = shortage_report(accepts, quota);
    CHECK_EQ(shortage.requests, std::vector<int>{0});
    CHECK(shortage.resources.empty());
}

TEST(2, "random instances: report exists exactly when someone is rejected") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        const int m = static_cast<int>(rng.uniform(1, 5));
        auto accepts = random_accepts(rng, n, m, m);
        std::vector<int> quota(m);
        for (auto& q : quota) {
            q = static_cast<int>(rng.uniform(0, 3));
        }
        INFO("round ", round);
        const Shortage shortage = shortage_report(accepts, quota);
        const Plan plan = best_plan(accepts, quota);
        CHECK_EQ(shortage.requests.empty(), plan.rejected == 0);
        CHECK(std::is_sorted(shortage.requests.begin(), shortage.requests.end()));
        CHECK(std::is_sorted(shortage.resources.begin(), shortage.resources.end()));
        if (!shortage.requests.empty()) {
            const std::set<int> allowed(shortage.resources.begin(), shortage.resources.end());
            long long capacity = 0;
            for (int r : shortage.resources) {
                capacity += quota[r];
            }
            CHECK_LT(capacity, static_cast<long long>(shortage.requests.size()));
            for (int i : shortage.requests) {
                for (int r : accepts[i]) {
                    CHECK(allowed.count(r) == 1);
                }
            }
        }
    }
}

TEST(3, "assignments respect quota as requests arrive") {
    Allocator allocator({1});
    CHECK_EQ(allocator.add_request({0}), 0);
    CHECK_EQ(allocator.choice_of(0), 0);
    CHECK_EQ(allocator.rejected(), 0);
    CHECK_EQ(allocator.add_request({0}), 1);
    CHECK_EQ(allocator.choice_of(1), -1);
    CHECK_EQ(allocator.rejected(), 1);
}

TEST(3, "an earlier flexible request moves over for a picky newcomer") {
    Allocator allocator({1, 1});
    const int a = allocator.add_request({0, 1});
    const int b = allocator.add_request({0});
    CHECK_EQ(allocator.rejected(), 0);
    CHECK_EQ(allocator.choice_of(b), 0);
    CHECK_EQ(allocator.choice_of(a), 1);
}

TEST(3, "cancelling frees capacity for a rejected request") {
    Allocator allocator({1});
    const int a = allocator.add_request({0});
    const int b = allocator.add_request({0});
    CHECK_EQ(allocator.rejected(), 1);
    allocator.cancel(a);
    CHECK_EQ(allocator.rejected(), 0);
    CHECK_EQ(allocator.choice_of(b), 0);
}

TEST(3, "cancelling a rejected request changes nothing else") {
    Allocator allocator({1});
    const int a = allocator.add_request({0});
    const int b = allocator.add_request({0});
    allocator.cancel(b);
    CHECK_EQ(allocator.rejected(), 0);
    CHECK_EQ(allocator.choice_of(a), 0);
}

TEST(3, "random streams stay optimal after every operation") {
    Rng rng;
    for (int round = 0; round < 100; ++round) {
        const int m = static_cast<int>(rng.uniform(1, 8));
        std::vector<int> quota(m);
        for (auto& q : quota) {
            q = static_cast<int>(rng.uniform(0, 3));
        }
        Allocator allocator(quota);
        Accepts live;
        std::vector<int> ids;
        for (int step = 0; step < 60; ++step) {
            INFO("round ", round, " step ", step);
            if (ids.empty() || rng.coin(0.7)) {
                auto choices = random_accepts(rng, 1, m, m)[0];
                ids.push_back(allocator.add_request(choices));
                live.push_back(choices);
            } else {
                const auto pos =
                    static_cast<std::size_t>(rng.uniform(0, static_cast<int>(ids.size()) - 1));
                allocator.cancel(ids[pos]);
                ids.erase(ids.begin() + static_cast<std::ptrdiff_t>(pos));
                live.erase(live.begin() + static_cast<std::ptrdiff_t>(pos));
            }
            std::vector<int> used(m, 0);
            int rejected = 0;
            for (std::size_t i = 0; i < ids.size(); ++i) {
                const int choice = allocator.choice_of(ids[i]);
                if (choice == -1) {
                    ++rejected;
                    continue;
                }
                CHECK(std::find(live[i].begin(), live[i].end(), choice) != live[i].end());
                ++used[choice];
            }
            CHECK_EQ(rejected, allocator.rejected());
            for (int r = 0; r < m; ++r) {
                CHECK_LE(used[r], quota[r]);
            }
            CHECK_EQ(static_cast<int>(ids.size()) - rejected, oracle_max_served(live, quota));
        }
    }
}

PERF(3, "incremental arrivals avoid a full replan", 3500) {
    Rng rng;
    const int m = 2000;
    std::vector<int> quota(m);
    for (auto& q : quota) {
        q = static_cast<int>(rng.uniform(1, 8));
    }
    Allocator allocator(quota);
    for (int i = 0; i < 20'000; ++i) {
        auto choices = random_accepts(rng, 1, m, 5)[0];
        allocator.add_request(std::move(choices));
    }
    CHECK_GE(allocator.rejected(), 0);
}
