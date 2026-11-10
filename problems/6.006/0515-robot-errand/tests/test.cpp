#include <algorithm>
#include <cstddef>
#include <limits>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr std::size_t idx(int value) { return static_cast<std::size_t>(value); }

constexpr int kMaxDoors = std::numeric_limits<int>::max();

Site site_of(int locations, int dock, std::vector<Door> doors, std::vector<int> items,
             std::vector<int> badges = {}) {
    return Site{locations, dock, std::move(doors), std::move(items), std::move(badges)};
}

Door open_two_way(int a, int b) { return {a, b, true, kNoBadge}; }
Door open_one_way(int a, int b) { return {a, b, false, kNoBadge}; }
Door locked_two_way(int a, int b, int badge) { return {a, b, true, badge}; }
Door locked_one_way(int a, int b, int badge) { return {a, b, false, badge}; }

// Inventory (badges held, item held) collected on arrival at each location, computed directly.
std::vector<int> contents_of(const Site& site) {
    std::vector<int> contents(idx(site.locations), 0);
    for (std::size_t badge = 0; badge < site.badge_locations.size(); ++badge) {
        contents[idx(site.badge_locations[badge])] |= 1 << badge;
    }
    for (const int loc : site.item_locations) {
        contents[idx(loc)] |= 1 << site.badge_locations.size();
    }
    return contents;
}

// Depth-first search that simulates the rules door by door; a state is only re-entered if it is
// reached with strictly fewer doors than ever before. Exponential in the worst case but tiny here.
void brute_visit(const Site& site, int location, int inventory, int doors,
                 const std::vector<std::vector<std::pair<int, int>>>& moves,
                 const std::vector<int>& contents,
                 std::map<std::pair<int, int>, int>& best_seen, int& best) {
    inventory |= contents[idx(location)];
    const int item_bit = 1 << site.badge_locations.size();
    if (location == site.dock && (inventory & item_bit) != 0) {
        best = std::min(best, doors);
        return;
    }
    if (doors + 1 >= best) {
        return;
    }
    const auto [slot, inserted] = best_seen.try_emplace({location, inventory}, doors);
    if (!inserted) {
        if (slot->second <= doors) {
            return;
        }
        slot->second = doors;
    }
    for (const auto& [to, needed] : moves[idx(location)]) {
        if (needed == kNoBadge || ((inventory >> needed) & 1) != 0) {
            brute_visit(site, to, inventory, doors + 1, moves, contents, best_seen, best);
        }
    }
}

std::optional<int> brute_fewest(const Site& site) {
    std::vector<std::vector<std::pair<int, int>>> moves(idx(site.locations));
    for (const Door& door : site.doors) {
        moves[idx(door.a)].emplace_back(door.b, door.badge);
        if (door.two_way) {
            moves[idx(door.b)].emplace_back(door.a, door.badge);
        }
    }
    int best = kMaxDoors;
    std::map<std::pair<int, int>, int> best_seen;
    brute_visit(site, site.dock, 0, 0, moves, contents_of(site), best_seen, best);
    if (best == kMaxDoors) {
        return std::nullopt;
    }
    return best;
}

Site random_site(Rng& rng, int max_locations, int max_doors, int max_badges) {
    const int n = static_cast<int>(rng.uniform(1, max_locations));
    const int badge_count = static_cast<int>(rng.uniform(0, max_badges));
    Site site{n, static_cast<int>(rng.uniform(0, n - 1)), {}, {}, {}};
    for (int b = 0; b < badge_count; ++b) {
        site.badge_locations.push_back(static_cast<int>(rng.uniform(0, n - 1)));
    }
    const int doors = static_cast<int>(rng.uniform(0, max_doors));
    for (int d = 0; d < doors; ++d) {
        const int a = static_cast<int>(rng.uniform(0, n - 1));
        const int b = static_cast<int>(rng.uniform(0, n - 1));
        const bool locked = badge_count > 0 && rng.coin(0.4);
        site.doors.push_back({a, b, rng.coin(), locked ? static_cast<int>(rng.uniform(0, badge_count - 1)) : kNoBadge});
    }
    const int items = static_cast<int>(rng.uniform(0, 3));
    for (int i = 0; i < items; ++i) {
        site.item_locations.push_back(static_cast<int>(rng.uniform(0, n - 1)));
    }
    return site;
}

// Replays a claimed route under the rules; a door with a held badge (or none) must connect each step.
void check_route(const Site& site, const std::vector<int>& route, int expected_doors) {
    REQUIRE_EQ(route.size(), expected_doors + 1);
    CHECK_EQ(route.front(), site.dock);
    CHECK_EQ(route.back(), site.dock);
    const auto contents = contents_of(site);
    int inventory = contents[idx(site.dock)];
    for (std::size_t i = 0; i + 1 < route.size(); ++i) {
        const int from = route[i];
        const int to = route[i + 1];
        REQUIRE(to >= 0 && to < site.locations);
        bool passable = false;
        for (const Door& door : site.doors) {
            const bool connects = (door.a == from && door.b == to) ||
                                  (door.two_way && door.a == to && door.b == from);
            if (connects && (door.badge == kNoBadge || ((inventory >> door.badge) & 1) != 0)) {
                passable = true;
                break;
            }
        }
        INFO("step ", i, " from ", from, " to ", to);
        REQUIRE(passable);
        inventory |= contents[idx(to)];
    }
    CHECK_NE(inventory & (1 << site.badge_locations.size()), 0);
}

}  // namespace

TEST(1, "an item at the dock needs no doors") {
    const Site site = site_of(2, 0, {open_two_way(0, 1)}, {0});
    CHECK_EQ(fewest_doors(site), std::optional<int>(0));
}

TEST(1, "no items means no errand") {
    const Site site = site_of(2, 0, {open_two_way(0, 1)}, {});
    CHECK_EQ(fewest_doors(site), std::optional<int>());
}

TEST(1, "there and back through a two-way door") {
    const Site site = site_of(2, 0, {open_two_way(0, 1)}, {1});
    CHECK_EQ(fewest_doors(site), std::optional<int>(2));
}

TEST(1, "a one-way door with no way back fails") {
    const Site site = site_of(2, 0, {open_one_way(0, 1)}, {1});
    CHECK_EQ(fewest_doors(site), std::optional<int>());
}

TEST(1, "unreachable items are ignored") {
    const Site site = site_of(4, 0, {open_two_way(0, 1), open_two_way(2, 3)}, {3, 1});
    CHECK_EQ(fewest_doors(site), std::optional<int>(2));
}

TEST(1, "a one-way loop counts every door once") {
    const Site site = site_of(3, 0, {open_one_way(0, 1), open_one_way(1, 2), open_one_way(2, 0)}, {1});
    CHECK_EQ(fewest_doors(site), std::optional<int>(3));
}

TEST(1, "the nearest item is not always the best round trip") {
    // Item at 1 is one door away but four doors back; item at 5 is two doors away and one back.
    const Site site = site_of(7, 0,
                              {open_one_way(0, 1), open_one_way(1, 2), open_one_way(2, 3),
                               open_one_way(3, 4), open_one_way(4, 0), open_one_way(0, 6),
                               open_one_way(6, 5), open_one_way(5, 0)},
                              {1, 5});
    CHECK_EQ(fewest_doors(site), std::optional<int>(3));
}

TEST(1, "repeated and parallel entries change nothing") {
    const Site site = site_of(2, 0, {open_two_way(0, 1), open_two_way(0, 1)}, {1, 1, 1});
    CHECK_EQ(fewest_doors(site), std::optional<int>(2));
}

TEST(1, "random unlocked sites match a rule-by-rule search") {
    Rng rng;
    for (int round = 0; round < 700; ++round) {
        const Site site = random_site(rng, 8, 12, 0);
        INFO("round ", round);
        REQUIRE_EQ(fewest_doors(site), brute_fewest(site));
    }
}

// One search per item location multiplies the map size by tens of thousands of items.
PERF(1, "many item locations need one search", 2000) {
    Rng rng;
    const int n = 200'000;
    Site site{n, 0, {}, {}, {}};
    site.doors.reserve(idx(2 * n));
    for (int v = 1; v < n; ++v) {
        const int parent = static_cast<int>(rng.uniform(0, v - 1));
        site.doors.push_back({parent, v, false, kNoBadge});
    }
    for (int v = 1; v < n; v += 2) {
        site.item_locations.push_back(v);
    }
    // No door leads back to the dock, so every item is a dead end and the whole map is searched.
    CHECK_EQ(fewest_doors(site), std::optional<int>());
}

TEST(2, "a locked door without its badge is a wall") {
    const Site site = site_of(3, 0, {locked_two_way(0, 1, 0)}, {1}, {2});
    CHECK_EQ(fewest_doors(site), std::optional<int>());
}

TEST(2, "a badge at the dock opens doors immediately") {
    const Site site = site_of(2, 0, {locked_two_way(0, 1, 0)}, {1}, {0});
    CHECK_EQ(fewest_doors(site), std::optional<int>(2));
}

TEST(2, "fetching a badge may require coming back through the dock") {
    const Site site = site_of(3, 0, {open_two_way(0, 1), locked_two_way(0, 2, 0)}, {2}, {1});
    CHECK_EQ(fewest_doors(site), std::optional<int>(4));
}

TEST(2, "a badge collected on the way is usable on the next door") {
    const Site site = site_of(3, 0, {open_one_way(0, 1), locked_one_way(1, 2, 0), open_one_way(2, 0)}, {2}, {1});
    CHECK_EQ(fewest_doors(site), std::optional<int>(3));
}

TEST(2, "badges can be chained behind each other") {
    // Badge 1 sits behind a door that needs badge 0, and the item sits behind a door that needs badge 1.
    const Site site = site_of(4, 0,
                              {open_two_way(0, 1), locked_two_way(1, 2, 0), locked_two_way(0, 3, 1)},
                              {3}, {1, 2});
    CHECK_EQ(fewest_doors(site), std::optional<int>(6));
}

TEST(2, "a badge locked behind its own door is unreachable") {
    const Site site = site_of(3, 0, {locked_two_way(0, 1, 0), open_two_way(1, 2)}, {2}, {1});
    CHECK_EQ(fewest_doors(site), std::optional<int>());
}

TEST(2, "a two-way locked door needs the badge in both directions") {
    // The way in is an open one-way door; the only way back is the locked door, in reverse.
    const Site site = site_of(3, 0, {open_one_way(0, 1), locked_two_way(0, 1, 0)}, {1}, {2});
    CHECK_EQ(fewest_doors(site), std::optional<int>());
}

TEST(2, "a badge can be fetched after the item on the way back") {
    const Site site = site_of(3, 0,
                              {open_two_way(0, 1), open_two_way(1, 2), locked_one_way(2, 0, 0)},
                              {2}, {2});
    CHECK_EQ(fewest_doors(site), std::optional<int>(3));
}

TEST(2, "random locked sites match a rule-by-rule search") {
    Rng rng;
    int solved = 0;
    for (int round = 0; round < 1500; ++round) {
        const Site site = random_site(rng, 6, 9, 3);
        INFO("round ", round, " n=", site.locations, " dock=", site.dock);
        const auto expected = brute_fewest(site);
        REQUIRE_EQ(fewest_doors(site), expected);
        solved += expected.has_value() ? 1 : 0;
    }
    CHECK_GT(solved, 150);
}

// A search that restarts per item, or stores states in an ordered map, is an order of magnitude slower.
PERF(2, "the search stays linear in the map with four badges", 2000) {
    Rng rng;
    const int n = 100'000;
    Site site{n, 0, {}, {}, {}};
    for (int b = 0; b < 4; ++b) {
        site.badge_locations.push_back(static_cast<int>(rng.uniform(0, n / 2 - 1)));
    }
    site.doors.reserve(idx(3 * n));
    for (int i = 0; i < 3 * n; ++i) {
        const int a = static_cast<int>(rng.uniform(0, n / 2 - 1));
        const int b = static_cast<int>(rng.uniform(0, n / 2 - 1));
        site.doors.push_back({a, b, true, rng.coin(0.1) ? static_cast<int>(rng.uniform(0, 3)) : kNoBadge});
    }
    for (int v = n / 2; v < n; ++v) {
        site.doors.push_back({static_cast<int>(rng.uniform(0, n / 2 - 1)), v, false, kNoBadge});
        site.item_locations.push_back(v);
    }
    CHECK_EQ(fewest_doors(site), std::optional<int>());
}

TEST(3, "no errand gives nullopt") {
    const Site site = site_of(2, 0, {open_one_way(0, 1)}, {1});
    CHECK_EQ(errand_route(site), std::optional<std::vector<int>>());
}

TEST(3, "an item at the dock is a one-element route") {
    const Site site = site_of(2, 0, {open_two_way(0, 1)}, {0});
    CHECK_EQ(errand_route(site), std::optional(std::vector<int>{0}));
}

TEST(3, "a route can pass the dock before the errand is finished") {
    const Site site = site_of(3, 0, {open_two_way(0, 1), locked_two_way(0, 2, 0)}, {2}, {1});
    CHECK_EQ(errand_route(site), std::optional(std::vector<int>{0, 1, 0, 2, 0}));
}

TEST(3, "a route can visit the same location twice to fetch a badge") {
    const Site site = site_of(4, 0,
                              {open_two_way(0, 1), open_two_way(1, 2), locked_two_way(1, 3, 0)},
                              {3}, {2});
    CHECK_EQ(errand_route(site), std::optional(std::vector<int>{0, 1, 2, 1, 3, 1, 0}));
}

TEST(3, "random sites produce valid shortest routes") {
    Rng rng;
    for (int round = 0; round < 1500; ++round) {
        const Site site = random_site(rng, 6, 9, 3);
        INFO("round ", round);
        const auto expected = brute_fewest(site);
        const auto route = errand_route(site);
        REQUIRE_EQ(route.has_value(), expected.has_value());
        if (route) {
            check_route(site, *route, *expected);
        }
    }
}

// Inserting at the front of the route while walking parents is quadratic on a long corridor.
PERF(3, "a long route is rebuilt in linear time", 1500) {
    const int n = 400'000;
    Site site{n, 0, {}, {n - 1}, {}};
    site.doors.reserve(idx(n));
    for (int v = 1; v < n; ++v) {
        site.doors.push_back({v - 1, v, true, kNoBadge});
    }
    const auto route = errand_route(site);
    REQUIRE(route.has_value());
    CHECK_EQ(route->size(), 2 * (n - 1) + 1);
}
