#pragma once

#include <algorithm>
#include <numeric>
#include <span>
#include <utility>
#include <vector>

struct Cnf {
    int num_vars = 0;
    std::vector<std::vector<int>> clauses;
};
using Assignment = std::vector<bool>;  // size num_vars + 1, index 0 unused

inline bool satisfies(const Cnf& f, const Assignment& a) {
    for (const auto& clause : f.clauses) {
        bool satisfied = false;
        for (int literal : clause) {
            const int var = literal > 0 ? literal : -literal;
            satisfied = satisfied || (literal > 0 ? a[var] : !a[var]);
        }
        if (!satisfied) {
            return false;
        }
    }
    return true;
}

inline Cnf reduce_sat_to_triple_sat(const Cnf& f) {
    Cnf out = f;
    out.num_vars = f.num_vars + 2;
    return out;
}

inline Assignment lift_triple_sat_to_sat(const Cnf& f, const Assignment& a) {
    return Assignment(a.begin(), a.begin() + f.num_vars + 1);
}

struct DonutInstance {
    int n = 0;
    std::vector<std::pair<int, int>> edges;
    std::vector<long long> profit;
    long long k = 0;
};

inline DonutInstance reduce_3sat_to_donut(const Cnf& f) {
    DonutInstance out;
    out.n = static_cast<int>(f.clauses.size()) * 3;
    out.profit.assign(out.n, 1);
    out.k = static_cast<long long>(f.clauses.size());
    const auto vertex = [](int clause, int position) { return clause * 3 + position; };
    for (int c = 0; c < static_cast<int>(f.clauses.size()); ++c) {
        out.edges.emplace_back(vertex(c, 0), vertex(c, 1));
        out.edges.emplace_back(vertex(c, 1), vertex(c, 2));
        out.edges.emplace_back(vertex(c, 0), vertex(c, 2));
        for (int d = c + 1; d < static_cast<int>(f.clauses.size()); ++d) {
            for (int p = 0; p < 3; ++p) {
                for (int q = 0; q < 3; ++q) {
                    if (f.clauses[c][p] == -f.clauses[d][q]) {
                        out.edges.emplace_back(vertex(c, p), vertex(d, q));
                    }
                }
            }
        }
    }
    return out;
}

inline Assignment lift_donut_to_3sat(const Cnf& f, const DonutInstance& inst,
                                     std::span<const int> chosen) {
    Assignment a(static_cast<std::size_t>(f.num_vars + 1), false);
    for (int v : chosen) {
        const int literal = f.clauses[v / 3][v % 3];
        if (literal > 0) {
            a[literal] = true;
        }
    }
    return a;
}

struct Task {
    long long time = 0;
    long long profit = 0;
    long long deadline = 0;
};
struct ScheduledTask {
    int task = 0;
    long long start = 0;
};
using Schedule = std::vector<ScheduledTask>;

inline bool valid_schedule(std::span<const Task> tasks, const Schedule& schedule) {
    std::vector<char> used(tasks.size(), 0);
    Schedule ordered = schedule;
    for (const auto& s : ordered) {
        if (s.task < 0 || s.task >= static_cast<int>(tasks.size()) || used[s.task] || s.start < 0 ||
            s.start + tasks[s.task].time > tasks[s.task].deadline) {
            return false;
        }
        used[s.task] = 1;
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const auto& a, const auto& b) { return a.start < b.start; });
    for (std::size_t i = 0; i + 1 < ordered.size(); ++i) {
        if (ordered[i].start + tasks[ordered[i].task].time > ordered[i + 1].start) {
            return false;
        }
    }
    return true;
}

inline long long schedule_profit(std::span<const Task> tasks, const Schedule& schedule) {
    long long total = 0;
    for (const auto& s : schedule) {
        total += tasks[s.task].profit;
    }
    return total;
}

struct SubsetSumInstance {
    std::vector<long long> a;
    long long t = 0;
};
struct SchedInstance {
    std::vector<Task> tasks;
    long long k = 0;
};

inline SchedInstance reduce_subset_sum_to_sched(const SubsetSumInstance& src) {
    SchedInstance out;
    out.k = src.t;
    for (long long value : src.a) {
        out.tasks.push_back({value, value, src.t});
    }
    return out;
}

inline std::vector<int> lift_sched_to_subset_sum(const SubsetSumInstance& src,
                                                 const SchedInstance& dst,
                                                 const Schedule& schedule) {
    std::vector<int> out;
    for (const auto& s : schedule) {
        out.push_back(s.task);
    }
    std::sort(out.begin(), out.end());
    return out;
}

inline Schedule best_schedule(std::span<const Task> tasks) {
    const int n = static_cast<int>(tasks.size());
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(),
              [&](int a, int b) { return tasks[a].deadline < tasks[b].deadline; });
    long long horizon = 0;
    for (const Task& task : tasks) {
        horizon = std::max(horizon, task.deadline);
    }
    // best[t]: most profit using the first i tasks (in deadline order) with total time exactly t.
    // choice[i][t] records whether task order[i] was taken to reach time t.
    constexpr long long kUnreachable = -1;
    std::vector<long long> best(static_cast<std::size_t>(horizon) + 1, kUnreachable);
    best[0] = 0;
    std::vector<std::vector<char>> taken(n, std::vector<char>(static_cast<std::size_t>(horizon) + 1, 0));
    for (int i = 0; i < n; ++i) {
        const Task& task = tasks[order[i]];
        for (long long t = task.deadline; t >= task.time; --t) {
            const long long prev = best[static_cast<std::size_t>(t - task.time)];
            if (prev != kUnreachable && prev + task.profit > best[static_cast<std::size_t>(t)]) {
                best[static_cast<std::size_t>(t)] = prev + task.profit;
                taken[i][static_cast<std::size_t>(t)] = 1;
            }
        }
    }
    long long end = 0;
    for (long long t = 0; t <= horizon; ++t) {
        if (best[static_cast<std::size_t>(t)] > best[static_cast<std::size_t>(end)]) {
            end = t;
        }
    }
    // Walk backwards through the DP, in reverse deadline order, placing tasks from the end.
    std::vector<int> chosen;
    long long t = end;
    for (int i = n - 1; i >= 0; --i) {
        if (taken[i][static_cast<std::size_t>(t)]) {
            chosen.push_back(order[i]);
            t -= tasks[order[i]].time;
        }
    }
    std::reverse(chosen.begin(), chosen.end());
    Schedule schedule;
    long long clock = 0;
    for (int id : chosen) {
        schedule.push_back({id, clock});
        clock += tasks[id].time;
    }
    return schedule;
}
