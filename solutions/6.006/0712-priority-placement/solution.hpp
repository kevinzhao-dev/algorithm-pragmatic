#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <span>
#include <utility>
#include <vector>

struct Task {
    int priority;
    int demand;
};

struct Server {
    int latency;
    int capacity;
};

namespace detail {

inline constexpr std::int64_t kInfeasible = std::numeric_limits<std::int64_t>::max() / 2;

// Task indices from most to least important.
inline std::vector<std::size_t> tasks_by_priority(std::span<const Task> tasks) {
    std::vector<std::size_t> order(tasks.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::ranges::sort(order, [&](std::size_t a, std::size_t b) {
        return tasks[a].priority > tasks[b].priority;
    });
    return order;
}

// Server indices from closest to farthest.
inline std::vector<std::size_t> servers_by_latency(std::span<const Server> servers) {
    std::vector<std::size_t> order(servers.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::ranges::sort(order, [&](std::size_t a, std::size_t b) {
        return servers[a].latency < servers[b].latency;
    });
    return order;
}

inline std::int64_t overload(const Task& task, const Server& server) {
    return std::max(0, task.demand - server.capacity);
}

}  // namespace detail

inline std::optional<std::int64_t> min_overload(std::span<const Task> tasks,
                                                std::span<const Server> servers) {
    const std::size_t n = tasks.size();
    const std::size_t m = servers.size();
    if (n > m) {
        return std::nullopt;
    }
    const auto task_order = detail::tasks_by_priority(tasks);
    const auto server_order = detail::servers_by_latency(servers);
    // below[j]: cost of placing the tasks after the current one using servers j and beyond.
    std::vector<std::int64_t> below(m + 1, 0);
    std::vector<std::int64_t> current(m + 1, detail::kInfeasible);
    for (std::size_t i = n; i-- > 0;) {
        const Task& task = tasks[task_order[i]];
        current[m] = detail::kInfeasible;
        for (std::size_t j = m; j-- > 0;) {
            const std::int64_t place = below[j + 1] == detail::kInfeasible
                                           ? detail::kInfeasible
                                           : detail::overload(task, servers[server_order[j]]) +
                                                 below[j + 1];
            current[j] = std::min(place, current[j + 1]);
        }
        std::swap(below, current);
    }
    return below[0];
}

inline std::optional<std::vector<std::size_t>> assign_servers(std::span<const Task> tasks,
                                                              std::span<const Server> servers) {
    const std::size_t n = tasks.size();
    const std::size_t m = servers.size();
    if (n > m) {
        return std::nullopt;
    }
    const auto task_order = detail::tasks_by_priority(tasks);
    const auto server_order = detail::servers_by_latency(servers);
    std::vector<std::int64_t> table((n + 1) * (m + 1), 0);
    const auto cell = [&](std::size_t i, std::size_t j) -> std::int64_t& {
        return table[i * (m + 1) + j];
    };
    for (std::size_t i = 0; i < n; ++i) {
        cell(i, m) = detail::kInfeasible;
    }
    for (std::size_t i = n; i-- > 0;) {
        for (std::size_t j = m; j-- > 0;) {
            const std::int64_t rest = cell(i + 1, j + 1);
            const std::int64_t place =
                rest == detail::kInfeasible
                    ? detail::kInfeasible
                    : detail::overload(tasks[task_order[i]], servers[server_order[j]]) + rest;
            cell(i, j) = std::min(place, cell(i, j + 1));
        }
    }

    std::vector<std::size_t> assignment(n);
    std::size_t i = 0;
    for (std::size_t j = 0; i < n; ++j) {
        if (cell(i, j) != cell(i, j + 1)) {
            assignment[task_order[i]] = server_order[j];
            ++i;
        }
    }
    return assignment;
}

struct DropPlan {
    std::int64_t cost;
    std::vector<std::optional<std::size_t>> assignment;
};

inline DropPlan plan_with_drops(std::span<const Task> tasks, std::span<const Server> servers,
                                std::span<const std::int64_t> drop_penalty) {
    const std::size_t n = tasks.size();
    const std::size_t m = servers.size();
    const auto task_order = detail::tasks_by_priority(tasks);
    const auto server_order = detail::servers_by_latency(servers);
    std::vector<std::int64_t> table((n + 1) * (m + 1), 0);
    const auto cell = [&](std::size_t i, std::size_t j) -> std::int64_t& {
        return table[i * (m + 1) + j];
    };
    for (std::size_t i = n; i-- > 0;) {
        const std::size_t task = task_order[i];
        cell(i, m) = drop_penalty[task] + cell(i + 1, m);
        for (std::size_t j = m; j-- > 0;) {
            cell(i, j) = std::min({detail::overload(tasks[task], servers[server_order[j]]) +
                                       cell(i + 1, j + 1),
                                   cell(i, j + 1), drop_penalty[task] + cell(i + 1, j)});
        }
    }

    DropPlan plan{cell(0, 0), std::vector<std::optional<std::size_t>>(n)};
    std::size_t j = 0;
    for (std::size_t i = 0; i < n;) {
        const std::size_t task = task_order[i];
        if (cell(i, j) == drop_penalty[task] + cell(i + 1, j)) {
            ++i;
        } else if (j < m && cell(i, j) == detail::overload(tasks[task], servers[server_order[j]]) +
                                              cell(i + 1, j + 1)) {
            plan.assignment[task] = server_order[j];
            ++i;
            ++j;
        } else {
            ++j;
        }
    }
    return plan;
}
