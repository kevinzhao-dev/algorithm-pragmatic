#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Conflicts = std::vector<Conflict>;

Conflict pair_of(const std::string& a, const std::string& b) { return {a, b}; }

std::string name_of(int i) { return "svc-" + std::to_string(i); }

Conflicts random_conflicts(Rng& rng, int services, int count) {
    Conflicts out;
    for (int i = 0; i < count; ++i) {
        out.emplace_back(name_of(static_cast<int>(rng.uniform(0, services - 1))),
                         name_of(static_cast<int>(rng.uniform(0, services - 1))));
    }
    return out;
}

// Tries every assignment of at most `max_names` distinct names to two windows.
bool brute_fits(const Conflicts& conflicts) {
    std::vector<std::string> names;
    for (const auto& [a, b] : conflicts) {
        names.push_back(a);
        names.push_back(b);
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    const auto id = [&](const std::string& name) {
        return static_cast<std::size_t>(std::lower_bound(names.begin(), names.end(), name) - names.begin());
    };
    const std::uint32_t assignments = 1u << names.size();
    for (std::uint32_t mask = 0; mask < assignments; ++mask) {
        bool valid = true;
        for (const auto& [a, b] : conflicts) {
            if (((mask >> id(a)) & 1u) == ((mask >> id(b)) & 1u)) {
                valid = false;
                break;
            }
        }
        if (valid) {
            return true;
        }
    }
    return false;
}

std::set<std::pair<std::string, std::string>> conflict_set(const Conflicts& conflicts) {
    std::set<std::pair<std::string, std::string>> out;
    for (const auto& [a, b] : conflicts) {
        out.emplace(a, b);
        out.emplace(b, a);
    }
    return out;
}

void check_partition(const Conflicts& conflicts, const Partition& plan) {
    std::set<std::string> everyone;
    for (const auto& [a, b] : conflicts) {
        everyone.insert(a);
        everyone.insert(b);
    }
    std::set<std::string> first(plan.first.begin(), plan.first.end());
    std::set<std::string> second(plan.second.begin(), plan.second.end());
    CHECK_EQ(first.size(), plan.first.size());
    CHECK_EQ(second.size(), plan.second.size());
    CHECK_EQ(first.size() + second.size(), everyone.size());
    for (const auto& name : first) {
        CHECK_EQ(second.count(name), 0);
        CHECK_EQ(everyone.count(name), 1);
    }
    for (const auto& name : second) {
        CHECK_EQ(everyone.count(name), 1);
    }
    for (const auto& [a, b] : conflicts) {
        CHECK(first.count(a) != first.count(b));
    }
}

void check_odd_cycle(const Conflicts& conflicts, const OddCycle& cycle) {
    REQUIRE(!cycle.members.empty());
    CHECK_EQ(cycle.members.size() % 2, 1);
    const std::set<std::string> distinct(cycle.members.begin(), cycle.members.end());
    CHECK_EQ(distinct.size(), cycle.members.size());
    const auto known = conflict_set(conflicts);
    for (std::size_t i = 0; i < cycle.members.size(); ++i) {
        const auto& a = cycle.members[i];
        const auto& b = cycle.members[(i + 1) % cycle.members.size()];
        CHECK_EQ(known.count({a, b}), 1);
    }
}

}  // namespace

TEST(1, "no conflicts always fit") {
    CHECK(can_fit_two_windows({}));
}

TEST(1, "one conflict fits") {
    const Conflicts conflicts = {pair_of("a", "b")};
    CHECK(can_fit_two_windows(conflicts));
}

TEST(1, "three services in a triangle do not fit") {
    const Conflicts conflicts = {pair_of("a", "b"), pair_of("b", "c"), pair_of("c", "a")};
    CHECK(!can_fit_two_windows(conflicts));
}

TEST(1, "a service in conflict with itself never fits") {
    const Conflicts conflicts = {pair_of("a", "a")};
    CHECK(!can_fit_two_windows(conflicts));
}

TEST(1, "even rings fit and odd rings do not") {
    Conflicts ring;
    for (int i = 0; i < 5; ++i) {
        ring.push_back(pair_of(name_of(i), name_of((i + 1) % 5)));
    }
    CHECK(!can_fit_two_windows(ring));
    ring.clear();
    for (int i = 0; i < 6; ++i) {
        ring.push_back(pair_of(name_of(i), name_of((i + 1) % 6)));
    }
    CHECK(can_fit_two_windows(ring));
}

TEST(1, "repeated and reversed conflicts are harmless") {
    const Conflicts conflicts = {pair_of("a", "b"), pair_of("b", "a"), pair_of("a", "b")};
    CHECK(can_fit_two_windows(conflicts));
}

TEST(1, "separate groups are judged independently") {
    const Conflicts fine = {pair_of("a", "b"), pair_of("c", "d")};
    CHECK(can_fit_two_windows(fine));
    const Conflicts odd_elsewhere = {pair_of("a", "b"), pair_of("c", "d"), pair_of("d", "e"),
                                     pair_of("e", "c")};
    CHECK(!can_fit_two_windows(odd_elsewhere));
}

TEST(1, "names are arbitrary strings, including the empty one") {
    const Conflicts conflicts = {pair_of("", "svc with spaces"), pair_of("", std::string(10'000, 'x'))};
    CHECK(can_fit_two_windows(conflicts));
    const Conflicts triangle = {pair_of("", "a"), pair_of("a", std::string(10'000, 'y')),
                                pair_of(std::string(10'000, 'y'), "")};
    CHECK(!can_fit_two_windows(triangle));
}

TEST(1, "random conflict lists match trying every assignment") {
    Rng rng;
    int yes = 0;
    int no = 0;
    for (int round = 0; round < 3000; ++round) {
        const int services = static_cast<int>(rng.uniform(1, 11));
        const int count = static_cast<int>(rng.uniform(0, 2 * services));
        const Conflicts conflicts = random_conflicts(rng, services, count);
        INFO("round ", round, " conflicts=", conflicts);
        const bool expected = brute_fits(conflicts);
        REQUIRE_EQ(can_fit_two_windows(conflicts), expected);
        (expected ? yes : no) += 1;
    }
    CHECK_GT(yes, 200);
    CHECK_GT(no, 200);
}

// Looking names up in a list, or walking the whole conflict list per service, is quadratic.
PERF(1, "a million conflicts over two million services", 2500) {
    Rng rng;
    Conflicts conflicts;
    const int n = 1'000'000;
    conflicts.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        conflicts.emplace_back(name_of(2 * i + static_cast<int>(rng.uniform(0, 1))),
                               name_of(2 * static_cast<int>(rng.uniform(0, n - 1)) + 1));
    }
    conflicts.emplace_back(name_of(0), name_of(0));
    CHECK(!can_fit_two_windows(conflicts));
}

TEST(2, "no conflicts give an empty plan") {
    const auto plan = plan_two_windows({});
    const auto* partition = std::get_if<Partition>(&plan);
    REQUIRE(partition != nullptr);
    CHECK(partition->first.empty());
    CHECK(partition->second.empty());
}

TEST(2, "a single conflict splits into one service each") {
    const Conflicts conflicts = {pair_of("a", "b")};
    const auto plan = plan_two_windows(conflicts);
    const auto* partition = std::get_if<Partition>(&plan);
    REQUIRE(partition != nullptr);
    check_partition(conflicts, *partition);
}

TEST(2, "a triangle comes back as a three-member cycle") {
    const Conflicts conflicts = {pair_of("a", "b"), pair_of("b", "c"), pair_of("c", "a")};
    const auto plan = plan_two_windows(conflicts);
    const auto* cycle = std::get_if<OddCycle>(&plan);
    REQUIRE(cycle != nullptr);
    CHECK_EQ(cycle->members.size(), 3);
    check_odd_cycle(conflicts, *cycle);
}

TEST(2, "a self-conflict is a cycle of length one") {
    const Conflicts conflicts = {pair_of("a", "b"), pair_of("x", "x")};
    const auto plan = plan_two_windows(conflicts);
    const auto* cycle = std::get_if<OddCycle>(&plan);
    REQUIRE(cycle != nullptr);
    CHECK_EQ(cycle->members, std::vector<std::string>{"x"});
}

TEST(2, "a cycle does not drag in services hanging off it") {
    // A five-ring where the spokes 0-5, 5-6 and 2-7 are pendant trees.
    const Conflicts conflicts = {pair_of("n0", "n1"), pair_of("n1", "n2"), pair_of("n2", "n3"),
                                 pair_of("n3", "n4"), pair_of("n4", "n0"), pair_of("n0", "n5"),
                                 pair_of("n5", "n6"), pair_of("n2", "n7")};
    const auto plan = plan_two_windows(conflicts);
    const auto* cycle = std::get_if<OddCycle>(&plan);
    REQUIRE(cycle != nullptr);
    check_odd_cycle(conflicts, *cycle);
    CHECK_EQ(cycle->members.size(), 5);
}

TEST(2, "an odd ring deep in a component stays simple") {
    // A long tail leads into a ring; walking both ends of the bad edge to the root would repeat vertices.
    Conflicts conflicts;
    for (int i = 0; i < 20; ++i) {
        conflicts.push_back(pair_of(name_of(i), name_of(i + 1)));
    }
    for (int i = 20; i < 26; ++i) {
        conflicts.push_back(pair_of(name_of(i), name_of(i + 1)));
    }
    conflicts.push_back(pair_of(name_of(26), name_of(20)));
    const auto plan = plan_two_windows(conflicts);
    const auto* cycle = std::get_if<OddCycle>(&plan);
    REQUIRE(cycle != nullptr);
    check_odd_cycle(conflicts, *cycle);
    CHECK_EQ(cycle->members.size(), 7);
}

TEST(2, "random lists produce valid certificates") {
    Rng rng;
    for (int round = 0; round < 2000; ++round) {
        const int services = static_cast<int>(rng.uniform(1, 10));
        const int count = static_cast<int>(rng.uniform(0, 2 * services));
        const Conflicts conflicts = random_conflicts(rng, services, count);
        INFO("round ", round, " conflicts=", conflicts);
        const auto plan = plan_two_windows(conflicts);
        if (brute_fits(conflicts)) {
            const auto* partition = std::get_if<Partition>(&plan);
            REQUIRE(partition != nullptr);
            check_partition(conflicts, *partition);
        } else {
            const auto* cycle = std::get_if<OddCycle>(&plan);
            REQUIRE(cycle != nullptr);
            check_odd_cycle(conflicts, *cycle);
        }
    }
}

PERF(2, "certificates on a million conflicts stay linear", 2500) {
    Rng rng;
    const int n = 800'000;
    Conflicts chain;
    chain.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i + 1 < n; ++i) {
        chain.emplace_back(name_of(i), name_of(i + 1));
    }
    chain.emplace_back(name_of(n - 1), name_of(n - 3));  // a triangle at the far end of the chain
    rng.shuffle(chain);
    const auto plan = plan_two_windows(chain);
    const auto* cycle = std::get_if<OddCycle>(&plan);
    REQUIRE(cycle != nullptr);
    CHECK_EQ(cycle->members.size() % 2, 1);
}

TEST(3, "lists that never break return nullopt") {
    const Conflicts conflicts = {pair_of("a", "b"), pair_of("b", "c")};
    CHECK_EQ(first_breaking_conflict(conflicts), std::optional<std::size_t>());
    CHECK_EQ(first_breaking_conflict({}), std::optional<std::size_t>());
}

TEST(3, "the conflict closing the odd ring is reported") {
    const Conflicts conflicts = {pair_of("a", "b"), pair_of("c", "d"), pair_of("b", "c"),
                                 pair_of("c", "a"), pair_of("x", "y")};
    CHECK_EQ(first_breaking_conflict(conflicts), std::optional<std::size_t>(3));
}

TEST(3, "a self-conflict breaks immediately") {
    const Conflicts conflicts = {pair_of("a", "a"), pair_of("b", "c")};
    CHECK_EQ(first_breaking_conflict(conflicts), std::optional<std::size_t>(0));
}

TEST(3, "random lists match checking every prefix") {
    Rng rng;
    for (int round = 0; round < 1500; ++round) {
        const int services = static_cast<int>(rng.uniform(1, 9));
        const int count = static_cast<int>(rng.uniform(0, 14));
        Conflicts conflicts = random_conflicts(rng, services, count);
        INFO("round ", round, " conflicts=", conflicts);
        std::optional<std::size_t> expected;
        for (std::size_t i = 0; i < conflicts.size() && !expected; ++i) {
            const Conflicts prefix(conflicts.begin(), conflicts.begin() + static_cast<long>(i) + 1);
            if (!brute_fits(prefix)) {
                expected = i;
            }
        }
        REQUIRE_EQ(first_breaking_conflict(conflicts), expected);
    }
}

// Re-checking every prefix is ~300,000 full traversals.
PERF(3, "the breaking conflict is found with few traversals", 2000) {
    Rng rng;
    const int n = 300'000;
    Conflicts conflicts;
    conflicts.reserve(static_cast<std::size_t>(n) + 1);
    for (int i = 0; i < n; ++i) {
        conflicts.emplace_back(name_of(2 * static_cast<int>(rng.uniform(0, n - 1))),
                               name_of(2 * static_cast<int>(rng.uniform(0, n - 1)) + 1));
    }
    conflicts.emplace_back(name_of(0), name_of(2));
    conflicts.emplace_back(name_of(2), name_of(3));
    conflicts.emplace_back(name_of(3), name_of(0));
    const auto index = first_breaking_conflict(conflicts);
    CHECK(index.has_value());
    CHECK_GE(index.value_or(0), static_cast<std::size_t>(n));
}
