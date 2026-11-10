#include <algorithm>
#include <cstddef>
#include <set>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr std::size_t idx(int value) { return static_cast<std::size_t>(value); }

// A random valid network: every rack is attached under some earlier vertex, so each component
// holds exactly one feed. Extra links only join racks already in the same component.
Topology random_topology(Rng& rng, int feeds, int racks, int extra_links) {
    Topology t{feeds, racks, {}};
    std::vector<int> component(idx(feeds + racks));
    for (int v = 0; v < feeds; ++v) {
        component[idx(v)] = v;
    }
    for (int v = feeds; v < feeds + racks; ++v) {
        const int anchor = static_cast<int>(rng.uniform(0, v - 1));
        component[idx(v)] = component[idx(anchor)];
        t.links.emplace_back(rng.coin() ? std::pair{v, anchor} : std::pair{anchor, v});
    }
    for (int i = 0; i < extra_links && racks >= 2; ++i) {
        const int a = static_cast<int>(rng.uniform(feeds, feeds + racks - 1));
        const int b = static_cast<int>(rng.uniform(feeds, feeds + racks - 1));
        if (a != b && component[idx(a)] == component[idx(b)]) {
            t.links.emplace_back(a, b);
        }
    }
    rng.shuffle(t.links);
    return t;
}

// Which racks stay powered when `cut` links are gone; a plain fixed-point loop, on purpose slow.
std::vector<char> brute_powered(const Topology& t, const std::vector<char>& cut) {
    std::vector<char> powered(idx(t.feeds + t.racks), 0);
    for (int f = 0; f < t.feeds; ++f) {
        powered[idx(f)] = 1;
    }
    bool changed = true;
    while (changed) {
        changed = false;
        for (std::size_t i = 0; i < t.links.size(); ++i) {
            if (cut[i] != 0) {
                continue;
            }
            const auto& [a, b] = t.links[i];
            if (powered[idx(a)] != powered[idx(b)]) {
                powered[idx(a)] = powered[idx(b)] = 1;
                changed = true;
            }
        }
    }
    return powered;
}

std::vector<int> brute_dark(const Topology& t, const std::vector<char>& cut) {
    const auto powered = brute_powered(t, cut);
    std::vector<int> dark;
    for (int v = t.feeds; v < t.feeds + t.racks; ++v) {
        if (powered[idx(v)] == 0) {
            dark.push_back(v);
        }
    }
    return dark;
}

// The feed of each rack, found by labelling components from each feed separately.
std::vector<int> brute_racks_per_feed(const Topology& t) {
    std::vector<int> counts(idx(t.feeds), 0);
    for (int feed = 0; feed < t.feeds; ++feed) {
        std::vector<char> reached(idx(t.feeds + t.racks), 0);
        reached[idx(feed)] = 1;
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto& [a, b] : t.links) {
                if (reached[idx(a)] != reached[idx(b)]) {
                    reached[idx(a)] = reached[idx(b)] = 1;
                    changed = true;
                }
            }
        }
        for (int v = t.feeds; v < t.feeds + t.racks; ++v) {
            counts[idx(feed)] += reached[idx(v)] != 0 ? 1 : 0;
        }
    }
    return counts;
}

}  // namespace

TEST(1, "a lone feed covers nothing") {
    const Topology t{1, 0, {}};
    const BackupChoice choice = choose_backup_feed(t);
    CHECK_EQ(choice.feed, 0);
    CHECK_EQ(choice.racks_covered, 0);
}

TEST(1, "the feed with the most racks wins") {
    const Topology t{2, 3, {{0, 2}, {1, 3}, {3, 4}}};
    const BackupChoice choice = choose_backup_feed(t);
    CHECK_EQ(choice.feed, 1);
    CHECK_EQ(choice.racks_covered, 2);
}

TEST(1, "ties go to the smallest feed id") {
    const Topology t{3, 2, {{1, 3}, {2, 4}}};
    const BackupChoice choice = choose_backup_feed(t);
    CHECK_EQ(choice.feed, 1);
    CHECK_EQ(choice.racks_covered, 1);
}

TEST(1, "repeated links do not change the count") {
    const Topology t{1, 2, {{0, 1}, {1, 2}, {0, 1}, {2, 1}}};
    CHECK_EQ(choose_backup_feed(t).racks_covered, 2);
}

TEST(1, "racks can be chained far from their feed") {
    Topology t{2, 5, {{0, 2}, {2, 3}, {3, 4}, {4, 5}, {1, 6}}};
    const BackupChoice choice = choose_backup_feed(t);
    CHECK_EQ(choice.feed, 0);
    CHECK_EQ(choice.racks_covered, 4);
}

TEST(1, "random networks match a brute-force per-feed count") {
    Rng rng;
    for (int round = 0; round < 600; ++round) {
        const int feeds = static_cast<int>(rng.uniform(1, 6));
        const int racks = static_cast<int>(rng.uniform(0, 30));
        const Topology t = random_topology(rng, feeds, racks, static_cast<int>(rng.uniform(0, 12)));
        INFO("round ", round, " feeds=", feeds, " racks=", racks, " links=", t.links);
        const BackupChoice choice = choose_backup_feed(t);
        const auto counts = brute_racks_per_feed(t);
        const auto best = std::max_element(counts.begin(), counts.end());
        CHECK_EQ(choice.racks_covered, *best);
        CHECK_EQ(choice.feed, best - counts.begin());
    }
}

TEST(1, "a chain of half a million racks does not overflow the stack") {
    Topology t{1, 500'000, {}};
    t.links.reserve(500'000);
    for (int v = 1; v <= 500'000; ++v) {
        t.links.emplace_back(v - 1, v);
    }
    CHECK_EQ(choose_backup_feed(t).racks_covered, 500'000);
}

// Starting a fresh search from every rack, or rescanning the whole link list per vertex, is quadratic.
PERF(1, "choosing the feed is linear", 1500) {
    Rng rng;
    const int feeds = 1000;
    const int racks = 2'000'000;
    Topology t{feeds, racks, {}};
    t.links.reserve(idx(racks));
    for (int v = feeds; v < feeds + racks; ++v) {
        t.links.emplace_back(v, v == feeds ? 0 : v - 1);
    }
    CHECK_EQ(choose_backup_feed(t).racks_covered, racks);
}

TEST(2, "cutting nothing leaves everything powered") {
    const Topology t{1, 2, {{0, 1}, {1, 2}}};
    CHECK(racks_without_power(t, {}).empty());
}

TEST(2, "cutting a link in a chain darkens everything past it") {
    const Topology t{1, 3, {{0, 1}, {1, 2}, {2, 3}}};
    const std::vector<std::size_t> cut = {1};
    CHECK_EQ(racks_without_power(t, cut), std::vector<int>{2, 3});
}

TEST(2, "only the cut copy of a parallel link goes away") {
    const Topology t{1, 1, {{0, 1}, {0, 1}}};
    const std::vector<std::size_t> one = {0};
    CHECK(racks_without_power(t, one).empty());
    const std::vector<std::size_t> both = {0, 1};
    CHECK_EQ(racks_without_power(t, both), std::vector<int>{1});
}

TEST(2, "duplicates in the cut list are harmless") {
    const Topology t{1, 2, {{0, 1}, {1, 2}}};
    const std::vector<std::size_t> cut = {1, 1};
    CHECK_EQ(racks_without_power(t, cut), std::vector<int>{2});
}

TEST(2, "a rack can stay up through another path") {
    const Topology t{1, 3, {{0, 1}, {1, 2}, {0, 3}, {3, 2}}};
    const std::vector<std::size_t> cut = {1};
    CHECK(racks_without_power(t, cut).empty());
}

TEST(2, "power may come from any feed after a cut") {
    const Topology t{2, 2, {{0, 2}, {2, 3}, {1, 3}}};
    const std::vector<std::size_t> cut = {1};
    CHECK(racks_without_power(t, cut).empty());
}

TEST(2, "random cuts match brute force") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const int feeds = static_cast<int>(rng.uniform(1, 5));
        const int racks = static_cast<int>(rng.uniform(0, 25));
        const Topology t = random_topology(rng, feeds, racks, static_cast<int>(rng.uniform(0, 10)));
        std::vector<std::size_t> cut_links;
        std::vector<char> cut(t.links.size(), 0);
        for (std::size_t i = 0; i < t.links.size(); ++i) {
            if (rng.coin(0.3)) {
                cut_links.push_back(i);
                cut[i] = 1;
            }
        }
        rng.shuffle(cut_links);
        INFO("round ", round, " links=", t.links, " cut=", cut_links);
        REQUIRE_EQ(racks_without_power(t, cut_links), brute_dark(t, cut));
    }
}

PERF(2, "a large cut set is still one traversal", 2500) {
    Rng rng;
    const int feeds = 100;
    const int racks = 1'500'000;
    Topology t = random_topology(rng, feeds, racks, 300'000);
    std::vector<std::size_t> cut_links;
    for (std::size_t i = 0; i < t.links.size(); i += 3) {
        cut_links.push_back(i);
    }
    const auto dark = racks_without_power(t, cut_links);
    CHECK_GT(dark.size(), 0);
}

TEST(3, "a chain darkens everything below each link") {
    const Topology t{1, 3, {{0, 1}, {1, 2}, {2, 3}}};
    CHECK_EQ(blast_radius(t), std::vector<int>{3, 2, 1});
}

TEST(3, "link order in the input does not matter") {
    const Topology t{1, 3, {{3, 2}, {0, 1}, {2, 1}}};
    CHECK_EQ(blast_radius(t), std::vector<int>{1, 3, 2});
}

TEST(3, "a ring protects all of its links") {
    const Topology t{1, 3, {{0, 1}, {1, 2}, {2, 3}, {3, 0}}};
    CHECK_EQ(blast_radius(t), std::vector<int>{0, 0, 0, 0});
}

TEST(3, "parallel links protect each other") {
    const Topology t{1, 2, {{0, 1}, {0, 1}, {1, 2}}};
    CHECK_EQ(blast_radius(t), std::vector<int>{0, 0, 1});
}

TEST(3, "a tree of racks counts each subtree") {
    const Topology t{1, 4, {{0, 1}, {1, 2}, {1, 3}, {0, 4}}};
    CHECK_EQ(blast_radius(t), std::vector<int>{3, 1, 1, 1});
}

TEST(3, "separate feeds are independent") {
    const Topology t{2, 3, {{0, 2}, {2, 3}, {1, 4}}};
    CHECK_EQ(blast_radius(t), std::vector<int>{2, 1, 1});
}

TEST(3, "random networks match cutting every link") {
    Rng rng;
    for (int round = 0; round < 1500; ++round) {
        const int feeds = static_cast<int>(rng.uniform(1, 4));
        const int racks = static_cast<int>(rng.uniform(0, 14));
        const Topology t = random_topology(rng, feeds, racks, static_cast<int>(rng.uniform(0, 8)));
        std::vector<int> expected;
        std::vector<char> cut(t.links.size(), 0);
        for (std::size_t i = 0; i < t.links.size(); ++i) {
            cut[i] = 1;
            expected.push_back(static_cast<int>(brute_dark(t, cut).size()));
            cut[i] = 0;
        }
        INFO("round ", round, " links=", t.links);
        REQUIRE_EQ(blast_radius(t), expected);
    }
}

TEST(3, "a very long chain does not overflow the stack") {
    Topology t{1, 400'000, {}};
    t.links.reserve(400'000);
    for (int v = 1; v <= 400'000; ++v) {
        t.links.emplace_back(v - 1, v);
    }
    const auto radius = blast_radius(t);
    CHECK_EQ(radius.front(), 400'000);
    CHECK_EQ(radius.back(), 1);
}

// Re-running a traversal per link takes hours at this size.
PERF(3, "blast radius is linear", 3000) {
    Rng rng;
    const int feeds = 50;
    const int racks = 1'000'000;
    const Topology t = random_topology(rng, feeds, racks, 400'000);
    const auto radius = blast_radius(t);
    CHECK_EQ(radius.size(), t.links.size());
}
