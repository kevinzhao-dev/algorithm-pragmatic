#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <queue>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

struct Assignment {
    std::int64_t makespan;
    std::vector<int> machine;
};

namespace detail {

// For each sum in [0, limit], the 1-based index of the first job that makes it reachable, or -1.
inline std::vector<int> first_job_reaching(std::span<const int> jobs, std::size_t limit) {
    std::vector<int> first(limit + 1, -1);
    first[0] = 0;
    for (std::size_t k = 0; k < jobs.size(); ++k) {
        const auto value = static_cast<std::size_t>(jobs[k]);
        for (std::size_t sum = limit; sum + 1 > value; --sum) {
            if (first[sum] == -1 && first[sum - value] != -1) {
                first[sum] = static_cast<int>(k) + 1;
            }
        }
    }
    return first;
}

}  // namespace detail

inline Assignment balance_two(std::span<const int> jobs) {
    const std::int64_t total = std::accumulate(jobs.begin(), jobs.end(), std::int64_t{0});
    const auto half = static_cast<std::size_t>(total / 2);
    const auto first = detail::first_job_reaching(jobs, half);
    std::size_t best = half;
    while (first[best] == -1) {
        --best;
    }
    Assignment result{total - static_cast<std::int64_t>(best), std::vector<int>(jobs.size(), 1)};
    for (std::size_t sum = best; sum > 0;) {
        const auto k = static_cast<std::size_t>(first[sum]) - 1;
        result.machine[k] = 0;
        sum -= static_cast<std::size_t>(jobs[k]);
    }
    return result;
}

namespace detail {

// Reachability over the loads of machines 0, 1 and 2; machine 3 takes whatever is left.
struct FourMachineTable {
    std::size_t side;
    std::vector<std::int32_t> first;  // 1-based job that first made a state reachable, or -1
    std::size_t index(std::size_t a, std::size_t b, std::size_t c) const {
        return (a * side + b) * side + c;
    }
};

inline FourMachineTable build_four_machine_table(std::span<const int> jobs, std::size_t total) {
    const std::size_t side = total + 1;
    FourMachineTable table{side, std::vector<std::int32_t>(side * side * side, -1)};
    table.first[table.index(0, 0, 0)] = 0;
    for (std::size_t k = 0; k < jobs.size(); ++k) {
        const auto job = static_cast<std::size_t>(jobs[k]);
        const auto stamp = static_cast<std::int32_t>(k) + 1;
        // Descending in every coordinate: predecessors still describe the state before this job.
        for (std::size_t a = total + 1; a-- > 0;) {
            for (std::size_t b = total + 1; b-- > 0;) {
                for (std::size_t c = total + 1; c-- > 0;) {
                    std::int32_t& cell = table.first[table.index(a, b, c)];
                    if (cell != -1) {
                        continue;
                    }
                    const bool via_a = a >= job && table.first[table.index(a - job, b, c)] != -1;
                    const bool via_b = b >= job && table.first[table.index(a, b - job, c)] != -1;
                    const bool via_c = c >= job && table.first[table.index(a, b, c - job)] != -1;
                    if (via_a || via_b || via_c) {
                        cell = stamp;
                    }
                }
            }
        }
    }
    return table;
}

}  // namespace detail

inline Assignment balance_four(std::span<const int> jobs) {
    const std::int64_t total = std::accumulate(jobs.begin(), jobs.end(), std::int64_t{0});
    const auto side_total = static_cast<std::size_t>(total);
    const auto table = detail::build_four_machine_table(jobs, side_total);

    std::int64_t best = std::numeric_limits<std::int64_t>::max();
    std::size_t best_a = 0;
    std::size_t best_b = 0;
    std::size_t best_c = 0;
    for (std::size_t a = 0; a <= side_total; ++a) {
        for (std::size_t b = 0; b + a <= side_total; ++b) {
            for (std::size_t c = 0; c + b + a <= side_total; ++c) {
                if (table.first[table.index(a, b, c)] == -1) {
                    continue;
                }
                const auto d = static_cast<std::int64_t>(side_total - a - b - c);
                const std::int64_t makespan =
                    std::max({static_cast<std::int64_t>(a), static_cast<std::int64_t>(b),
                              static_cast<std::int64_t>(c), d});
                if (makespan < best) {
                    best = makespan;
                    best_a = a;
                    best_b = b;
                    best_c = c;
                }
            }
        }
    }

    Assignment result{jobs.empty() ? 0 : best, std::vector<int>(jobs.size(), 3)};
    std::size_t a = best_a;
    std::size_t b = best_b;
    std::size_t c = best_c;
    while (a + b + c > 0) {
        const std::int32_t stamp = table.first[table.index(a, b, c)];
        const auto k = static_cast<std::size_t>(stamp) - 1;
        const auto job = static_cast<std::size_t>(jobs[k]);
        const auto before = [&](std::size_t x, std::size_t y, std::size_t z) {
            const std::int32_t earlier = table.first[table.index(x, y, z)];
            return earlier != -1 && earlier < stamp;
        };
        if (a >= job && before(a - job, b, c)) {
            result.machine[k] = 0;
            a -= job;
        } else if (b >= job && before(a, b - job, c)) {
            result.machine[k] = 1;
            b -= job;
        } else {
            result.machine[k] = 2;
            c -= job;
        }
    }
    return result;
}

inline std::int64_t min_makespan_four(std::span<const int> jobs) {
    return balance_four(jobs).makespan;
}

struct Plan {
    std::int64_t makespan;
    std::vector<int> machine;
    bool exact;
};

namespace detail {

inline Plan longest_processing_time(std::span<const int> jobs, std::size_t machines) {
    std::vector<std::size_t> order(jobs.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::ranges::stable_sort(order, [&](std::size_t a, std::size_t b) { return jobs[a] > jobs[b]; });

    using Load = std::pair<std::int64_t, std::size_t>;  // load, machine
    std::priority_queue<Load, std::vector<Load>, std::greater<>> loads;
    for (std::size_t m = 0; m < machines; ++m) {
        loads.emplace(0, m);
    }
    Plan plan{0, std::vector<int>(jobs.size(), 0), false};
    for (const std::size_t job : order) {
        auto [load, machine] = loads.top();
        loads.pop();
        load += jobs[job];
        plan.machine[job] = static_cast<int>(machine);
        plan.makespan = std::max(plan.makespan, load);
        loads.emplace(load, machine);
    }
    return plan;
}

}  // namespace detail

inline Plan plan_jobs(std::span<const int> jobs, std::size_t machines, std::uint64_t work_limit) {
    if (machines == 0) {
        throw std::invalid_argument("need at least one machine");
    }
    const std::int64_t total = std::accumulate(jobs.begin(), jobs.end(), std::int64_t{0});
    const auto span_size = static_cast<std::uint64_t>(total) + 1;
    const std::uint64_t n = jobs.size();
    if (machines == 1) {
        return Plan{total, std::vector<int>(jobs.size(), 0), true};
    }
    if (machines == 2 && n * span_size <= work_limit) {
        Assignment best = balance_two(jobs);
        return Plan{best.makespan, std::move(best.machine), true};
    }
    if (machines == 4 && n * span_size * span_size * span_size <= work_limit) {
        Assignment best = balance_four(jobs);
        return Plan{best.makespan, std::move(best.machine), true};
    }
    return detail::longest_processing_time(jobs, machines);
}
