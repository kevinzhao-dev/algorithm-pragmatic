#include <algorithm>
#include <functional>
#include <optional>
#include <queue>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Edges = std::vector<std::pair<int, int>>;

namespace {

Edges random_edges(Rng& rng, int n, int m) {
    Edges edges;
    for (int i = 0; i < m; ++i) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        int v = static_cast<int>(rng.uniform(0, n - 1));
        if (u == v) {
            v = (v + 1) % n;
        }
        edges.emplace_back(u, v);
    }
    return edges;
}

// Validates: starts at source, ends at target, consecutive, simple, edge ids in range.
void check_route(const Edges& edges, const std::vector<int>& route, int source, int target) {
    REQUIRE(!route.empty());
    CHECK_EQ(edges[route.front()].first, source);
    CHECK_EQ(edges[route.back()].second, target);
    std::set<int> visited{source};
    for (std::size_t i = 0; i < route.size(); ++i) {
        const auto& e = edges[route[i]];
        if (i > 0) {
            REQUIRE(edges[route[i - 1]].second == e.first);
        }
        CHECK(visited.insert(e.second).second);
    }
}

void check_disjoint(const std::vector<std::vector<int>>& routes) {
    std::set<int> seen;
    for (const auto& route : routes) {
        for (int edge : route) {
            CHECK(seen.insert(edge).second);
        }
    }
}

// Enumerates simple paths with DFS, exhaustively backtracking over all assignments
// (tiny graphs only). `limits` is empty to mean unlimited vertex capacity.
bool brute_feasible(int n, const Edges& edges, const std::vector<int>& sources, int target,
                    const std::vector<int>& limits) {
    std::vector<std::vector<int>> adj(n);
    for (int id = 0; id < static_cast<int>(edges.size()); ++id) {
        adj[edges[id].first].push_back(id);
    }
    std::vector<char> edge_used(edges.size(), 0);
    std::vector<int> through(n, 0);
    const auto can_touch = [&](int v) { return limits.empty() || limits[v] < 0 || through[v] < limits[v]; };
    const std::function<bool(std::size_t)> assign = [&](std::size_t i) -> bool {
        if (i == sources.size()) {
            return true;
        }
        const int source = sources[i];
        std::vector<char> on_path(n, 0);
        const std::function<bool(int)> extend = [&](int v) -> bool {
            if (v == target) {
                return assign(i + 1);
            }
            for (int id : adj[v]) {
                const int to = edges[id].second;
                if (edge_used[id] || on_path[to] || !can_touch(to)) {
                    continue;
                }
                edge_used[id] = 1;
                on_path[to] = 1;
                ++through[to];
                if (extend(to)) {
                    return true;
                }
                --through[to];
                on_path[to] = 0;
                edge_used[id] = 0;
            }
            return false;
        };
        if (!can_touch(source)) {
            return false;
        }
        on_path[source] = 1;
        ++through[source];
        const bool ok = extend(source);
        --through[source];
        return ok;
    };
    return assign(0);
}

bool reachable_without(int n, const Edges& edges, const std::vector<char>& removed, int from,
                       int to) {
    std::vector<std::vector<int>> adj(n);
    for (int id = 0; id < static_cast<int>(edges.size()); ++id) {
        if (!removed[id]) {
            adj[edges[id].first].push_back(edges[id].second);
        }
    }
    std::vector<char> seen(n, 0);
    std::queue<int> q;
    q.push(from);
    seen[from] = 1;
    while (!q.empty()) {
        const int v = q.front();
        q.pop();
        for (int w : adj[v]) {
            if (!seen[w]) {
                seen[w] = 1;
                q.push(w);
            }
        }
    }
    return seen[to];
}

std::vector<int> sample_sources(Rng& rng, int n, int k, int target) {
    std::vector<int> vertices;
    for (int v = 0; v < n; ++v) {
        if (v != target) {
            vertices.push_back(v);
        }
    }
    rng.shuffle(vertices);
    vertices.resize(static_cast<std::size_t>(k));
    return vertices;
}

}  // namespace

TEST(1, "single source and direct edge") {
    const Edges edges{{0, 1}};
    const std::vector<int> sources{0};
    const auto routes = edge_disjoint_routes(2, edges, sources, 1);
    REQUIRE(routes.has_value());
    REQUIRE_EQ(routes->size(), 1);
    check_route(edges, (*routes)[0], 0, 1);
}

TEST(1, "two sources must not share the only bridge") {
    const Edges edges{{0, 2}, {1, 2}, {2, 3}};
    const std::vector<int> sources{0, 1};
    CHECK(!edge_disjoint_routes(4, edges, sources, 3).has_value());
}

TEST(1, "sharing a vertex but not an edge is allowed") {
    const Edges edges{{0, 2}, {1, 2}, {2, 3}, {2, 3}};
    const std::vector<int> sources{0, 1};
    const auto routes = edge_disjoint_routes(4, edges, sources, 3);
    REQUIRE(routes.has_value());
    REQUIRE_EQ(routes->size(), 2);
    check_route(edges, (*routes)[0], 0, 3);
    check_route(edges, (*routes)[1], 1, 3);
    check_disjoint(*routes);
}

TEST(1, "a detour is needed instead of the direct path") {
    // Routing source 0 through edge 0->1 first would block the only exit of source 1.
    const Edges edges{{0, 1}, {1, 3}, {0, 2}, {2, 3}, {4, 1}};
    const std::vector<int> sources{0, 4};
    const auto routes = edge_disjoint_routes(5, edges, sources, 3);
    REQUIRE(routes.has_value());
    check_route(edges, (*routes)[0], 0, 3);
    check_route(edges, (*routes)[1], 4, 3);
    check_disjoint(*routes);
}

TEST(1, "random graphs agree with exhaustive search") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 7));
        const int target = n - 1;
        const int k = static_cast<int>(rng.uniform(1, std::max(1, n - 1)));
        const auto edges = random_edges(rng, n, static_cast<int>(rng.uniform(1, 14)));
        const auto sources = sample_sources(rng, n, k, target);
        INFO("round ", round);
        const auto routes = edge_disjoint_routes(n, edges, sources, target);
        REQUIRE_EQ(routes.has_value(), brute_feasible(n, edges, sources, target, {}));
        if (routes) {
            REQUIRE_EQ(routes->size(), sources.size());
            for (int i = 0; i < k; ++i) {
                check_route(edges, (*routes)[i], sources[i], target);
            }
            check_disjoint(*routes);
        }
    }
}

PERF(1, "hundreds of routes through a big layered graph", 2500) {
    // Width-W layers with straight columns, so k <= W edge-disjoint routes always exist,
    // plus random shortcuts that make the path search work harder.
    Rng rng;
    const int width = 500, layers = 40;
    const int n = width * layers + 1;
    const int target = n - 1;
    Edges edges;
    for (int layer = 0; layer + 1 < layers; ++layer) {
        for (int i = 0; i < width; ++i) {
            edges.emplace_back(layer * width + i, (layer + 1) * width + i);
        }
    }
    for (int i = 0; i < width; ++i) {
        edges.emplace_back((layers - 1) * width + i, target);
    }
    for (int e = 0; e < 60'000; ++e) {
        const int layer = static_cast<int>(rng.uniform(0, layers - 2));
        edges.emplace_back(layer * width + rng.uniform(0, width - 1),
                           (layer + 1) * width + rng.uniform(0, width - 1));
    }
    std::vector<int> sources;
    for (int i = 0; i < 450; ++i) {
        sources.push_back(i);
    }
    const auto routes = edge_disjoint_routes(n, edges, sources, target);
    CHECK(routes.has_value());
}

namespace {

// Validity of a partial routing: disjoint simple paths, and a Menger certificate of matching size.
void check_certificate(int n, const Edges& edges, const std::vector<int>& sources, int target,
                       const PartialRouting& result) {
    const int k = static_cast<int>(sources.size());
    REQUIRE_EQ(result.routes.size(), sources.size());
    std::vector<std::vector<int>> served;
    for (int i = 0; i < k; ++i) {
        if (!result.routes[i].empty()) {
            check_route(edges, result.routes[i], sources[i], target);
            served.push_back(result.routes[i]);
        }
    }
    check_disjoint(served);
    CHECK(std::is_sorted(result.cut_edges.begin(), result.cut_edges.end()));
    CHECK(std::is_sorted(result.free_sources.begin(), result.free_sources.end()));
    CHECK_EQ(served.size(), result.cut_edges.size() + result.free_sources.size());
    std::vector<char> removed(edges.size(), 0);
    for (int id : result.cut_edges) {
        REQUIRE(id >= 0 && id < static_cast<int>(edges.size()));
        removed[id] = 1;
    }
    std::vector<char> is_free(sources.size(), 0);
    for (int i : result.free_sources) {
        REQUIRE(i >= 0 && i < k);
        is_free[i] = 1;
    }
    for (int i = 0; i < k; ++i) {
        if (!is_free[i]) {
            CHECK(!reachable_without(n, edges, removed, sources[i], target));
        }
    }
}

}  // namespace

TEST(2, "everyone routable has an empty certificate side") {
    const Edges edges{{0, 1}, {1, 2}};
    const std::vector<int> sources{0};
    const auto result = best_effort_routes(3, edges, sources, 2);
    REQUIRE_EQ(result.routes.size(), 1);
    check_route(edges, result.routes[0], 0, 2);
    check_certificate(3, edges, sources, 2, result);
}

TEST(2, "a bridge limits two sources to one route") {
    const Edges edges{{0, 2}, {1, 2}, {2, 3}};
    const std::vector<int> sources{0, 1};
    const auto result = best_effort_routes(4, edges, sources, 3);
    CHECK_EQ(result.cut_edges, std::vector<int>{2});
    CHECK(result.free_sources.empty());
    int served = 0;
    for (std::size_t i = 0; i < 2; ++i) {
        if (!result.routes[i].empty()) {
            check_route(edges, result.routes[i], sources[i], 3);
            ++served;
        }
    }
    CHECK_EQ(served, 1);
}

TEST(2, "a source with no path out cannot be served") {
    const Edges edges{{1, 2}};
    const std::vector<int> sources{0, 1};
    const auto result = best_effort_routes(3, edges, sources, 2);
    CHECK(result.routes[0].empty());
    check_route(edges, result.routes[1], 1, 2);
    check_certificate(3, edges, sources, 2, result);
}

TEST(2, "random graphs: certificates prove optimality") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 8));
        const int target = n - 1;
        const int k = static_cast<int>(rng.uniform(1, std::max(1, n - 1)));
        const auto edges = random_edges(rng, n, static_cast<int>(rng.uniform(0, 14)));
        const auto sources = sample_sources(rng, n, k, target);
        INFO("round ", round);
        const auto result = best_effort_routes(n, edges, sources, target);
        check_certificate(n, edges, sources, target, result);
    }
}

TEST(3, "no limits behaves like edge-disjoint routing") {
    const Edges edges{{0, 2}, {1, 2}, {2, 3}, {2, 3}};
    const std::vector<int> sources{0, 1};
    const std::vector<int> limits(4, -1);
    const auto routes = routes_with_hub_limits(4, edges, sources, 3, limits);
    REQUIRE(routes.has_value());
    check_disjoint(*routes);
}

TEST(3, "a hub limit of one blocks two routes through it") {
    const Edges edges{{0, 2}, {1, 2}, {2, 3}, {2, 3}};
    const std::vector<int> sources{0, 1};
    const std::vector<int> limits{-1, -1, 1, -1};
    CHECK(!routes_with_hub_limits(4, edges, sources, 3, limits).has_value());
}

TEST(3, "source and target limits are enforced") {
    const Edges edges{{0, 2}, {1, 2}};
    const std::vector<int> sources{0, 1};
    const std::vector<int> limits{-1, -1, 1};  // target can only take one route
    CHECK(!routes_with_hub_limits(3, edges, sources, 2, limits).has_value());
}

TEST(3, "random graphs agree with exhaustive search under limits") {
    Rng rng;
    for (int round = 0; round < 400; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 7));
        const int target = n - 1;
        const int k = static_cast<int>(rng.uniform(1, std::max(1, n - 1)));
        const auto edges = random_edges(rng, n, static_cast<int>(rng.uniform(1, 12)));
        const auto sources = sample_sources(rng, n, k, target);
        std::vector<int> limits(n, -1);
        for (int v = 0; v < n; ++v) {
            if (rng.coin(0.5)) {
                limits[v] = static_cast<int>(rng.uniform(1, 3));
            }
        }
        INFO("round ", round);
        const auto routes = routes_with_hub_limits(n, edges, sources, target, limits);
        REQUIRE_EQ(routes.has_value(), brute_feasible(n, edges, sources, target, limits));
        if (routes) {
            REQUIRE_EQ(routes->size(), sources.size());
            std::vector<int> through(n, 0);
            for (int i = 0; i < k; ++i) {
                check_route(edges, (*routes)[i], sources[i], target);
                std::set<int> touched{sources[i]};
                for (int edge : (*routes)[i]) {
                    touched.insert(edges[edge].second);
                }
                for (int v : touched) {
                    ++through[v];
                }
            }
            check_disjoint(*routes);
            for (int v = 0; v < n; ++v) {
                if (limits[v] >= 0) {
                    CHECK_LE(through[v], limits[v]);
                }
            }
        }
    }
}
