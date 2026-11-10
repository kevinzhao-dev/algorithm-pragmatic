#pragma once

#include <algorithm>
#include <limits>
#include <queue>
#include <stdexcept>
#include <vector>

class FlowNetwork {
public:
    struct PathFlow {
        std::vector<int> edges;
        long long amount = 0;
    };

    explicit FlowNetwork(int n) : adj_(n) {}

    int add_edge(int u, int v, long long capacity) {
        const int id = static_cast<int>(capacity_.size() / 2);
        adj_[u].push_back(static_cast<int>(capacity_.size()));
        to_.push_back(v);
        capacity_.push_back(capacity);
        adj_[v].push_back(static_cast<int>(capacity_.size()));
        to_.push_back(u);
        capacity_.push_back(0);
        return id;
    }

    long long max_flow_dfs(int s, int t) {
        begin(s, t);
        std::vector<char> visited(adj_.size());
        while (true) {
            std::fill(visited.begin(), visited.end(), 0);
            const long long pushed = dfs(s, t, kUnbounded, visited);
            if (pushed == 0) {
                break;
            }
            value_ += pushed;
            ++augmentations_;
        }
        return value_;
    }

    long long max_flow_bfs(int s, int t) {
        begin(s, t);
        while (true) {
            std::vector<int> via(adj_.size(), -1);
            std::queue<int> frontier;
            frontier.push(s);
            via[s] = -2;
            while (!frontier.empty() && via[t] == -1) {
                const int v = frontier.front();
                frontier.pop();
                for (int arc : adj_[v]) {
                    if (residual_[arc] > 0 && via[to_[arc]] == -1) {
                        via[to_[arc]] = arc;
                        frontier.push(to_[arc]);
                    }
                }
            }
            if (via[t] == -1) {
                break;
            }
            long long pushed = kUnbounded;
            for (int v = t; v != s; v = to_[via[v] ^ 1]) {
                pushed = std::min(pushed, residual_[via[v]]);
            }
            for (int v = t; v != s; v = to_[via[v] ^ 1]) {
                residual_[via[v]] -= pushed;
                residual_[via[v] ^ 1] += pushed;
            }
            value_ += pushed;
            ++augmentations_;
        }
        return value_;
    }

    long long flow_on(int id) const { return residual_[2 * id + 1]; }
    int augmentations() const { return augmentations_; }

    std::vector<bool> min_cut_side() const {
        std::vector<bool> side(adj_.size(), false);
        std::queue<int> frontier;
        frontier.push(source_);
        side[source_] = true;
        while (!frontier.empty()) {
            const int v = frontier.front();
            frontier.pop();
            for (int arc : adj_[v]) {
                if (residual_[arc] > 0 && !side[to_[arc]]) {
                    side[to_[arc]] = true;
                    frontier.push(to_[arc]);
                }
            }
        }
        return side;
    }

    std::vector<int> cut_edges() const {
        const auto side = min_cut_side();
        std::vector<int> out;
        for (int id = 0; id * 2 < static_cast<int>(capacity_.size()); ++id) {
            const int u = to_[2 * id + 1];
            const int v = to_[2 * id];
            if (side[u] && !side[v]) {
                out.push_back(id);
            }
        }
        return out;
    }

    std::vector<PathFlow> decompose() const {
        std::vector<long long> left;
        for (int id = 0; id * 2 < static_cast<int>(capacity_.size()); ++id) {
            left.push_back(flow_on(id));
        }
        std::vector<PathFlow> out;
        long long remaining = value_;
        while (remaining > 0) {
            // BFS over edges that still carry undistributed flow.
            std::vector<int> via(adj_.size(), -1);
            std::queue<int> frontier;
            frontier.push(source_);
            via[source_] = -2;
            while (!frontier.empty() && via[sink_] == -1) {
                const int v = frontier.front();
                frontier.pop();
                for (int arc : adj_[v]) {
                    if (arc % 2 == 0 && left[arc / 2] > 0 && via[to_[arc]] == -1) {
                        via[to_[arc]] = arc;
                        frontier.push(to_[arc]);
                    }
                }
            }
            long long amount = kUnbounded;
            std::vector<int> edges;
            for (int v = sink_; v != source_; v = to_[via[v] ^ 1]) {
                amount = std::min(amount, left[via[v] / 2]);
                edges.push_back(via[v] / 2);
            }
            std::reverse(edges.begin(), edges.end());
            for (int edge : edges) {
                left[edge] -= amount;
            }
            remaining -= amount;
            out.push_back({std::move(edges), amount});
        }
        return out;
    }

private:
    static constexpr long long kUnbounded = std::numeric_limits<long long>::max();

    void begin(int s, int t) {
        if (s == t) {
            throw std::invalid_argument("source equals sink");
        }
        source_ = s;
        sink_ = t;
        value_ = 0;
        augmentations_ = 0;
        residual_ = capacity_;
    }

    long long dfs(int v, int t, long long limit, std::vector<char>& visited) {
        if (v == t) {
            return limit;
        }
        visited[v] = 1;
        for (int arc : adj_[v]) {
            if (residual_[arc] > 0 && !visited[to_[arc]]) {
                const long long pushed =
                    dfs(to_[arc], t, std::min(limit, residual_[arc]), visited);
                if (pushed > 0) {
                    residual_[arc] -= pushed;
                    residual_[arc ^ 1] += pushed;
                    return pushed;
                }
            }
        }
        return 0;
    }

    std::vector<std::vector<int>> adj_;
    std::vector<int> to_;
    std::vector<long long> capacity_;
    std::vector<long long> residual_;
    int source_ = 0;
    int sink_ = 0;
    long long value_ = 0;
    int augmentations_ = 0;
};
