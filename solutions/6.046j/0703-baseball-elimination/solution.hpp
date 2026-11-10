#pragma once

#include <algorithm>
#include <limits>
#include <queue>
#include <vector>

struct Standings {
    std::vector<int> wins;
    std::vector<std::vector<int>> games_left;
};

namespace baseball_detail {

inline int remaining(const Standings& s, int team) {
    int total = 0;
    for (int games : s.games_left[team]) {
        total += games;
    }
    return total;
}

// Tiny Edmonds-Karp over an adjacency-list residual graph.
class Flow {
public:
    explicit Flow(int n) : adj_(n) {}
    void add(int u, int v, long long cap) {
        adj_[u].push_back(static_cast<int>(to_.size()));
        to_.push_back(v);
        res_.push_back(cap);
        adj_[v].push_back(static_cast<int>(to_.size()));
        to_.push_back(u);
        res_.push_back(0);
    }
    long long max_flow(int s, int t) {
        long long total = 0;
        while (true) {
            std::vector<int> via(adj_.size(), -1);
            std::queue<int> q;
            q.push(s);
            via[s] = -2;
            while (!q.empty() && via[t] == -1) {
                const int v = q.front();
                q.pop();
                for (int arc : adj_[v]) {
                    if (res_[arc] > 0 && via[to_[arc]] == -1) {
                        via[to_[arc]] = arc;
                        q.push(to_[arc]);
                    }
                }
            }
            if (via[t] == -1) {
                return total;
            }
            long long pushed = std::numeric_limits<long long>::max();
            for (int v = t; v != s; v = to_[via[v] ^ 1]) {
                pushed = std::min(pushed, res_[via[v]]);
            }
            for (int v = t; v != s; v = to_[via[v] ^ 1]) {
                res_[via[v]] -= pushed;
                res_[via[v] ^ 1] += pushed;
            }
            total += pushed;
        }
    }
    std::vector<char> reachable(int s) const {
        std::vector<char> seen(adj_.size(), 0);
        std::queue<int> q;
        q.push(s);
        seen[s] = 1;
        while (!q.empty()) {
            const int v = q.front();
            q.pop();
            for (int arc : adj_[v]) {
                if (res_[arc] > 0 && !seen[to_[arc]]) {
                    seen[to_[arc]] = 1;
                    q.push(to_[arc]);
                }
            }
        }
        return seen;
    }

private:
    std::vector<std::vector<int>> adj_;
    std::vector<int> to_;
    std::vector<long long> res_;
};

struct Network {
    Flow flow;
    int s;
    int t;
    long long games_total;
    std::vector<int> team_node;
};

// Returns false in `network` slot `trivial` when some team already passes the target.
inline Network build(const Standings& st, int team, int target, bool* trivial) {
    const int n = static_cast<int>(st.wins.size());
    *trivial = false;
    long long games = 0;
    int game_nodes = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (i != team && j != team && st.games_left[i][j] > 0) {
                ++game_nodes;
            }
        }
    }
    // Node ids: 0 = source, 1 = sink, 2..n+1 = teams, then one node per pending pair.
    Flow flow(n + 2 + game_nodes);
    std::vector<int> team_node(n, -1);
    for (int i = 0; i < n; ++i) {
        team_node[i] = 2 + i;
    }
    int node = 2 + n;
    for (int i = 0; i < n; ++i) {
        if (i == team) {
            continue;
        }
        const int cap = target - st.wins[i];
        if (cap < 0) {
            *trivial = true;
        }
        flow.add(team_node[i], 1, std::max(cap, 0));
        for (int j = i + 1; j < n; ++j) {
            if (j != team && st.games_left[i][j] > 0) {
                flow.add(0, node, st.games_left[i][j]);
                flow.add(node, team_node[i], std::numeric_limits<int>::max());
                flow.add(node, team_node[j], std::numeric_limits<int>::max());
                games += st.games_left[i][j];
                ++node;
            }
        }
    }
    return {std::move(flow), 0, 1, games, team_node};
}

}  // namespace baseball_detail

inline bool trivially_eliminated(const Standings& s, int team) {
    const int target = s.wins[team] + baseball_detail::remaining(s, team);
    for (int i = 0; i < static_cast<int>(s.wins.size()); ++i) {
        if (i != team && s.wins[i] > target) {
            return true;
        }
    }
    return false;
}

inline bool is_eliminated(const Standings& s, int team) {
    const int target = s.wins[team] + baseball_detail::remaining(s, team);
    bool trivial = false;
    auto net = baseball_detail::build(s, team, target, &trivial);
    if (trivial) {
        return true;
    }
    return net.flow.max_flow(net.s, net.t) < net.games_total;
}

inline std::vector<int> elimination_certificate(const Standings& s, int team) {
    const int n = static_cast<int>(s.wins.size());
    const int target = s.wins[team] + baseball_detail::remaining(s, team);
    for (int i = 0; i < n; ++i) {
        if (i != team && s.wins[i] > target) {
            return {i};
        }
    }
    bool trivial = false;
    auto net = baseball_detail::build(s, team, target, &trivial);
    if (net.flow.max_flow(net.s, net.t) == net.games_total) {
        return {};
    }
    const auto side = net.flow.reachable(net.s);
    std::vector<int> out;
    for (int i = 0; i < n; ++i) {
        if (i != team && side[net.team_node[i]]) {
            out.push_back(i);
        }
    }
    return out;
}

class SeasonTracker {
public:
    explicit SeasonTracker(Standings initial) : standings_(std::move(initial)) {}

    void record_game(int winner, int loser) {
        ++standings_.wins[winner];
        --standings_.games_left[winner][loser];
        --standings_.games_left[loser][winner];
    }

    std::vector<int> contenders() const {
        std::vector<int> out;
        for (int i = 0; i < static_cast<int>(standings_.wins.size()); ++i) {
            if (is_contender(i)) {
                out.push_back(i);
            }
        }
        return out;
    }

    bool is_contender(int team) const { return !is_eliminated(standings_, team); }

private:
    Standings standings_;
};
