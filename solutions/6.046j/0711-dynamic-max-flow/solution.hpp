#pragma once

#include <algorithm>
#include <limits>
#include <queue>
#include <vector>

class DynamicMaxFlow {
public:
    DynamicMaxFlow(int n, int source, int sink) : adj_(n), s_(source), t_(sink) {}

    int add_edge(int u, int v, long long capacity) {
        const int id = static_cast<int>(to_.size() / 2);
        adj_[u].push_back(static_cast<int>(to_.size()));
        to_.push_back(v);
        cap_.push_back(capacity);
        flow_.push_back(0);
        adj_[v].push_back(static_cast<int>(to_.size()));
        to_.push_back(u);
        cap_.push_back(0);
        flow_.push_back(0);
        return id;
    }

    long long compute() {
        std::fill(flow_.begin(), flow_.end(), 0);
        augment_until_blocked();
        return value_ = net_out_of_source();
    }

    long long value() const { return value_; }
    long long flow_on(int id) const { return flow_[2 * id]; }
    long long capacity_of(int id) const { return cap_[2 * id]; }

    long long set_capacity(int id, long long r) {
        const int arc = 2 * id;
        const long long old = cap_[arc];
        if (r >= old) {
            cap_[arc] = r;
            if (r > old) {
                augment_until_blocked();
            }
            return value_ = net_out_of_source();
        }
        cap_[arc] = r;
        if (flow_[arc] > r) {
            // Pull the excess off this edge, one unit at a time.
            const int u = to_[arc ^ 1];
            const int v = to_[arc];
            for (long long remaining = flow_[arc] - r; remaining > 0; --remaining) {
                add(arc, -1);
                cancel_unit(u, v);
            }
            augment_until_blocked();
        }
        return value_ = net_out_of_source();
    }

private:
    long long residual(int arc) const { return cap_[arc] - flow_[arc]; }

    void add(int arc, long long amount) {
        flow_[arc] += amount;
        flow_[arc ^ 1] -= amount;
    }

    long long net_out_of_source() const {
        long long net = 0;
        for (int arc : adj_[s_]) {
            net += flow_[arc];
        }
        return net;
    }

    void augment_until_blocked() {
        while (true) {
            std::vector<int> via(adj_.size(), -1);
            std::queue<int> frontier;
            frontier.push(s_);
            via[s_] = -2;
            while (!frontier.empty() && via[t_] == -1) {
                const int v = frontier.front();
                frontier.pop();
                for (int arc : adj_[v]) {
                    if (residual(arc) > 0 && via[to_[arc]] == -1) {
                        via[to_[arc]] = arc;
                        frontier.push(to_[arc]);
                    }
                }
            }
            if (via[t_] == -1) {
                return;
            }
            long long pushed = std::numeric_limits<long long>::max();
            for (int x = t_; x != s_; x = to_[via[x] ^ 1]) {
                pushed = std::min(pushed, residual(via[x]));
            }
            for (int x = t_; x != s_; x = to_[via[x] ^ 1]) {
                add(via[x], pushed);
            }
        }
    }

    // Removes one unit of flow along a positive-flow path, restoring conservation after one unit
    // has been taken off (u, v): excess at u, deficit at v.
    void cancel_unit(int u, int v) {
        const auto path = positive_flow_path(u, /*backward=*/true, v);
        for (int arc : path) {
            add(arc, 1);  // arc runs against a positive flow edge
        }
        const int reached = path.empty() ? u : to_[path.back()];
        if (reached == v) {
            return;
        }
        for (int arc : positive_flow_path(v, /*backward=*/false, -1)) {
            add(arc, -1);  // arc follows a positive flow edge
        }
    }

    // BFS over edges carrying flow, from `start`, stopping at s, t, or `extra_stop`.
    // Backward walks use arcs whose partner edge carries positive flow into the current vertex.
    std::vector<int> positive_flow_path(int start, bool backward, int extra_stop) const {
        if (start == s_ || start == t_ || start == extra_stop) {
            return {};
        }
        std::vector<int> via(adj_.size(), -1);
        std::queue<int> frontier;
        frontier.push(start);
        via[start] = -2;
        int hit = -1;
        while (!frontier.empty() && hit == -1) {
            const int x = frontier.front();
            frontier.pop();
            for (int arc : adj_[x]) {
                const bool usable = backward ? flow_[arc] < 0 : flow_[arc] > 0;
                const int y = to_[arc];
                if (usable && via[y] == -1) {
                    via[y] = arc;
                    if (y == s_ || y == t_ || y == extra_stop) {
                        hit = y;
                        break;
                    }
                    frontier.push(y);
                }
            }
        }
        std::vector<int> path;
        for (int y = hit; y != start; y = to_[via[y] ^ 1]) {
            path.push_back(via[y]);
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    std::vector<std::vector<int>> adj_;
    std::vector<int> to_;
    std::vector<long long> cap_;
    std::vector<long long> flow_;
    int s_;
    int t_;
    long long value_ = 0;
};
