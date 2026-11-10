#pragma once

#include <algorithm>
#include <functional>
#include <numeric>
#include <queue>
#include <span>
#include <vector>

inline std::vector<int> spt_order(std::span<const long long> durations) {
    std::vector<int> order(durations.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(),
                     [&](int a, int b) { return durations[a] < durations[b]; });
    return order;
}

inline long long total_completion_time(std::span<const long long> durations,
                                       std::span<const int> order) {
    long long clock = 0;
    long long total = 0;
    for (int id : order) {
        clock += durations[id];
        total += clock;
    }
    return total;
}

struct Job {
    long long release = 0;
    long long duration = 0;
};

inline std::vector<long long> srpt_completion(std::span<const Job> jobs) {
    const int n = static_cast<int>(jobs.size());
    std::vector<long long> done(n, 0);
    if (n == 0) {
        return done;
    }
    std::vector<int> by_release(n);
    std::iota(by_release.begin(), by_release.end(), 0);
    std::sort(by_release.begin(), by_release.end(),
              [&](int a, int b) { return jobs[a].release < jobs[b].release; });

    using Entry = std::pair<long long, int>;  // (remaining, job)
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> ready;
    long long now = 0;
    int next = 0;
    while (next < n || !ready.empty()) {
        if (ready.empty()) {
            now = std::max(now, jobs[by_release[next]].release);
        }
        while (next < n && jobs[by_release[next]].release <= now) {
            ready.emplace(jobs[by_release[next]].duration, by_release[next]);
            ++next;
        }
        auto [remaining, id] = ready.top();
        ready.pop();
        const long long horizon =
            next < n ? std::min(remaining, jobs[by_release[next]].release - now) : remaining;
        now += horizon;
        remaining -= horizon;
        if (remaining == 0) {
            done[id] = now;
        } else {
            ready.emplace(remaining, id);
        }
    }
    return done;
}

struct Interval {
    long long start = 0;
    long long finish = 0;
};

struct Allocation {
    int machines = 0;
    std::vector<int> assignment;
};

inline Allocation allocate(std::span<const Interval> intervals) {
    const int n = static_cast<int>(intervals.size());
    Allocation out;
    out.assignment.assign(n, -1);
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(),
              [&](int a, int b) { return intervals[a].start < intervals[b].start; });
    using Entry = std::pair<long long, int>;  // (busy until, machine)
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> busy;
    for (int id : order) {
        if (!busy.empty() && busy.top().first <= intervals[id].start) {
            const int machine = busy.top().second;
            busy.pop();
            out.assignment[id] = machine;
            busy.emplace(intervals[id].finish, machine);
        } else {
            out.assignment[id] = out.machines++;
            busy.emplace(intervals[id].finish, out.assignment[id]);
        }
    }
    return out;
}
