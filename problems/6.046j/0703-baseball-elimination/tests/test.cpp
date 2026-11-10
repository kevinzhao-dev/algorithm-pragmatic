#include <algorithm>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

Standings random_standings(Rng& rng, int n, int max_wins, int max_games) {
    Standings s;
    s.wins.resize(n);
    s.games_left.assign(n, std::vector<int>(n, 0));
    for (int i = 0; i < n; ++i) {
        s.wins[i] = static_cast<int>(rng.uniform(0, max_wins));
        for (int j = i + 1; j < n; ++j) {
            s.games_left[i][j] = s.games_left[j][i] = static_cast<int>(rng.uniform(0, max_games));
        }
    }
    return s;
}

int remaining(const Standings& s, int team) {
    int total = 0;
    for (int g : s.games_left[team]) {
        total += g;
    }
    return total;
}

// Subset form of the elimination theorem: exhaustive over all subsets (n <= 12).
std::vector<int> brute_certificate(const Standings& s, int team) {
    const int n = static_cast<int>(s.wins.size());
    const long long target = s.wins[team] + remaining(s, team);
    for (int mask = 1; mask < (1 << n); ++mask) {
        if (mask >> team & 1) {
            continue;
        }
        long long wins = 0, within = 0;
        int size = 0;
        for (int i = 0; i < n; ++i) {
            if (mask >> i & 1) {
                ++size;
                wins += s.wins[i];
                for (int j = i + 1; j < n; ++j) {
                    if (mask >> j & 1) {
                        within += s.games_left[i][j];
                    }
                }
            }
        }
        if (wins + within > target * size) {
            std::vector<int> out;
            for (int i = 0; i < n; ++i) {
                if (mask >> i & 1) {
                    out.push_back(i);
                }
            }
            return out;
        }
    }
    return {};
}

bool certifies(const Standings& s, int team, const std::vector<int>& group) {
    long long wins = 0, within = 0;
    for (std::size_t i = 0; i < group.size(); ++i) {
        wins += s.wins[group[i]];
        for (std::size_t j = i + 1; j < group.size(); ++j) {
            within += s.games_left[group[i]][group[j]];
        }
    }
    const long long target = s.wins[team] + remaining(s, team);
    return wins + within > target * static_cast<long long>(group.size());
}

}  // namespace

TEST(1, "already mathematically behind") {
    const Standings s{{10, 3}, {{0, 2}, {2, 0}}};
    CHECK(trivially_eliminated(s, 1));
    CHECK(!trivially_eliminated(s, 0));
}

TEST(1, "catching up is still possible") {
    const Standings s{{10, 8}, {{0, 2}, {2, 0}}};
    CHECK(!trivially_eliminated(s, 1));
    CHECK(!trivially_eliminated(s, 0));
}

TEST(2, "a team tied at the top is not eliminated") {
    const Standings s{{5, 5}, {{0, 0}, {0, 0}}};
    CHECK(!is_eliminated(s, 0));
    CHECK(!is_eliminated(s, 1));
}

TEST(2, "two leaders must split their last game") {
    // Teams 1 and 2 each have 6 wins and play each other twice more; team 0 can reach 6 at most.
    Standings s;
    s.wins = {6, 6, 6};
    s.games_left = {{0, 0, 0}, {0, 0, 2}, {0, 2, 0}};
    CHECK(is_eliminated(s, 0));
    CHECK(!trivially_eliminated(s, 0));
}

TEST(2, "a clever split keeps the trailing team alive") {
    Standings s;
    s.wins = {7, 6, 6};
    s.games_left = {{0, 0, 0}, {0, 0, 2}, {0, 2, 0}};
    CHECK(!is_eliminated(s, 0));
}

TEST(2, "a team that already leads with no games left") {
    const Standings s{{9, 3}, {{0, 0}, {0, 0}}};
    CHECK(!is_eliminated(s, 0));
    CHECK(is_eliminated(s, 1));
}

TEST(2, "random leagues match the subset theorem") {
    Rng rng;
    for (int round = 0; round < 400; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 8));
        const auto s = random_standings(rng, n, 30, 6);
        for (int team = 0; team < n; ++team) {
            INFO("round ", round, " team ", team);
            CHECK_EQ(is_eliminated(s, team), !brute_certificate(s, team).empty());
        }
    }
}

TEST(3, "no certificate for a live team") {
    const Standings s{{5, 5}, {{0, 0}, {0, 0}}};
    CHECK(elimination_certificate(s, 0).empty());
}

TEST(3, "a single dominant team is a one-element certificate") {
    const Standings s{{20, 1}, {{0, 0}, {0, 0}}};
    CHECK_EQ(elimination_certificate(s, 1), std::vector<int>{0});
}

TEST(3, "the certificate needs several teams when none dominates alone") {
    Standings s;
    s.wins = {6, 6, 6};
    s.games_left = {{0, 0, 0}, {0, 0, 2}, {0, 2, 0}};
    CHECK_EQ(elimination_certificate(s, 0), std::vector<int>{1, 2});
}

TEST(3, "random leagues produce valid certificates exactly when eliminated") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 10));
        const auto s = random_standings(rng, n, 40, 5);
        for (int team = 0; team < n; ++team) {
            INFO("round ", round, " team ", team);
            const auto group = elimination_certificate(s, team);
            REQUIRE_EQ(group.empty(), !is_eliminated(s, team));
            if (!group.empty()) {
                CHECK(std::is_sorted(group.begin(), group.end()));
                CHECK(std::find(group.begin(), group.end(), team) == group.end());
                CHECK(certifies(s, team, group));
            }
            CHECK_EQ(group.empty(), brute_certificate(s, team).empty());
        }
    }
}

TEST(4, "a game result can knock a team out") {
    Standings s;
    s.wins = {5, 5, 6};
    s.games_left = {{0, 1, 0}, {1, 0, 0}, {0, 0, 0}};
    SeasonTracker tracker(s);
    CHECK_EQ(tracker.contenders(), std::vector<int>{0, 1, 2});
    tracker.record_game(0, 1);
    CHECK(tracker.is_contender(0));
    CHECK(!tracker.is_contender(1));
    CHECK(tracker.is_contender(2));
    CHECK_EQ(tracker.contenders(), std::vector<int>{0, 2});
}

TEST(4, "playing out a random season stays consistent") {
    Rng rng;
    for (int round = 0; round < 50; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 7));
        Standings s = random_standings(rng, n, 20, 3);
        SeasonTracker tracker(s);
        std::vector<std::pair<int, int>> pending;
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                for (int g = 0; g < s.games_left[i][j]; ++g) {
                    pending.emplace_back(i, j);
                }
            }
        }
        rng.shuffle(pending);
        for (const auto& [a, b] : pending) {
            const bool first_wins = rng.coin();
            const int winner = first_wins ? a : b;
            const int loser = first_wins ? b : a;
            tracker.record_game(winner, loser);
            ++s.wins[winner];
            --s.games_left[a][b];
            --s.games_left[b][a];
            INFO("round ", round);
            for (int team = 0; team < n; ++team) {
                CHECK_EQ(tracker.is_contender(team), !is_eliminated(s, team));
            }
        }
    }
}

PERF(4, "standings service across a large league", 3000) {
    Rng rng;
    const int n = 40;
    Standings s = random_standings(rng, n, 60, 2);
    SeasonTracker tracker(s);
    long long alive = 0;
    for (int step = 0; step < 30; ++step) {
        const int a = static_cast<int>(rng.uniform(0, n - 1));
        const int b = (a + 1 + static_cast<int>(rng.uniform(0, n - 2))) % n;
        if (s.games_left[a][b] > 0) {
            tracker.record_game(a, b);
            --s.games_left[a][b];
            --s.games_left[b][a];
        }
        alive += static_cast<long long>(tracker.contenders().size());
    }
    CHECK_GE(alive, 0);
}
