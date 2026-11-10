#pragma once

#include <algorithm>
#include <cstddef>
#include <queue>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

class ComputationDag {
public:
    explicit ComputationDag(std::size_t strands) : successors_(strands), in_degree_(strands, 0) {}

    void add_edge(std::size_t before, std::size_t after) {
        if (before >= size() || after >= size()) {
            throw std::out_of_range("strand index out of range");
        }
        if (before == after) {
            throw std::invalid_argument("a strand cannot precede itself");
        }
        successors_[before].push_back(after);
        ++in_degree_[after];
    }

    std::size_t size() const { return successors_.size(); }
    const std::vector<std::size_t>& successors(std::size_t strand) const {
        return successors_.at(strand);
    }

    std::size_t work() const { return size(); }

    std::size_t span() const {
        // Longest path in vertices, relaxing in topological (Kahn) order.
        std::vector<std::size_t> pending = in_degree_;
        std::vector<std::size_t> longest(size(), 1);
        std::vector<std::size_t> frontier;
        for (std::size_t v = 0; v < size(); ++v) {
            if (pending[v] == 0) {
                frontier.push_back(v);
            }
        }
        std::size_t visited = 0;
        std::size_t best = 0;
        while (!frontier.empty()) {
            const std::size_t v = frontier.back();
            frontier.pop_back();
            ++visited;
            best = std::max(best, longest[v]);
            for (const std::size_t w : successors_[v]) {
                longest[w] = std::max(longest[w], longest[v] + 1);
                if (--pending[w] == 0) {
                    frontier.push_back(w);
                }
            }
        }
        if (visited != size()) {
            throw std::logic_error("the graph has a cycle");
        }
        return best;
    }

    double parallelism() const {
        const std::size_t s = span();
        return s == 0 ? 1.0 : static_cast<double>(work()) / static_cast<double>(s);
    }

    const std::vector<std::size_t>& in_degrees() const { return in_degree_; }

private:
    std::vector<std::vector<std::size_t>> successors_;
    std::vector<std::size_t> in_degree_;
};

struct Schedule {
    std::vector<std::size_t> step_of;
    std::size_t length = 0;
};

inline Schedule greedy_schedule(const ComputationDag& dag, std::size_t processors,
                                const std::vector<std::size_t>& priority) {
    if (processors == 0) {
        throw std::invalid_argument("need at least one processor");
    }
    const std::size_t n = dag.size();
    Schedule schedule;
    schedule.step_of.assign(n, 0);
    std::vector<std::size_t> pending = dag.in_degrees();
    using Entry = std::pair<std::size_t, std::size_t>;  // (priority, strand)
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> ready;
    for (std::size_t v = 0; v < n; ++v) {
        if (pending[v] == 0) {
            ready.emplace(priority[v], v);
        }
    }
    std::size_t done = 0;
    std::size_t step = 0;
    while (!ready.empty()) {
        std::vector<std::size_t> running;
        while (!ready.empty() && running.size() < processors) {
            running.push_back(ready.top().second);
            ready.pop();
        }
        for (const std::size_t v : running) {
            schedule.step_of[v] = step;
        }
        // Strands made ready by this step wait for the next one.
        for (const std::size_t v : running) {
            ++done;
            for (const std::size_t w : dag.successors(v)) {
                if (--pending[w] == 0) {
                    ready.emplace(priority[w], w);
                }
            }
        }
        ++step;
    }
    if (done != n) {
        throw std::logic_error("the graph has a cycle");
    }
    schedule.length = step;
    return schedule;
}

struct NearTwiceInstance {
    ComputationDag dag;
    std::vector<std::size_t> good_priority;
    std::vector<std::size_t> bad_priority;
};

inline NearTwiceInstance near_twice_instance(std::size_t processors, std::size_t chain_length) {
    // A chain of k strands plus (p-1)k independent strands: run the chain first and everything
    // finishes in k steps; run the independent strands first and the chain starts only after
    // they are done, nearly doubling the time.
    const std::size_t independent = (processors - 1) * chain_length;
    const std::size_t n = chain_length + independent;
    NearTwiceInstance instance{ComputationDag(n), std::vector<std::size_t>(n),
                               std::vector<std::size_t>(n)};
    for (std::size_t v = 0; v + 1 < chain_length; ++v) {
        instance.dag.add_edge(v, v + 1);
    }
    for (std::size_t v = 0; v < n; ++v) {
        const bool in_chain = v < chain_length;
        instance.good_priority[v] = in_chain ? 0 : 1;
        instance.bad_priority[v] = in_chain ? 1 : 0;
    }
    return instance;
}

struct BuildTask {
    long long duration = 1;
    std::vector<std::size_t> deps;
};

struct BuildPlan {
    long long total_work = 0;
    long long critical_path = 0;
    long long makespan = 0;
};

inline BuildPlan plan_build(std::span<const BuildTask> tasks, std::size_t workers) {
    const std::size_t n = tasks.size();
    BuildPlan plan;
    if (n == 0) {
        return plan;
    }
    if (workers == 0) {
        throw std::invalid_argument("need at least one worker");
    }
    std::vector<std::vector<std::size_t>> successors(n);
    std::vector<std::size_t> pending(n, 0);
    for (std::size_t v = 0; v < n; ++v) {
        plan.total_work += tasks[v].duration;
        for (const std::size_t d : tasks[v].deps) {
            successors.at(d).push_back(v);
            ++pending[v];
        }
    }

    // Critical path by Kahn's algorithm.
    {
        std::vector<std::size_t> remaining = pending;
        std::vector<long long> finish(n, 0);
        std::vector<std::size_t> frontier;
        for (std::size_t v = 0; v < n; ++v) {
            if (remaining[v] == 0) {
                frontier.push_back(v);
            }
        }
        std::size_t seen = 0;
        while (!frontier.empty()) {
            const std::size_t v = frontier.back();
            frontier.pop_back();
            ++seen;
            const long long done = finish[v] + tasks[v].duration;
            plan.critical_path = std::max(plan.critical_path, done);
            for (const std::size_t w : successors[v]) {
                finish[w] = std::max(finish[w], done);
                if (--remaining[w] == 0) {
                    frontier.push_back(w);
                }
            }
        }
        if (seen != n) {
            throw std::logic_error("the task graph has a cycle");
        }
    }

    // Event-driven simulation: free workers take the lowest-numbered ready task.
    std::priority_queue<std::size_t, std::vector<std::size_t>, std::greater<std::size_t>> ready;
    for (std::size_t v = 0; v < n; ++v) {
        if (pending[v] == 0) {
            ready.push(v);
        }
    }
    using Finish = std::pair<long long, std::size_t>;  // (finish time, task)
    std::priority_queue<Finish, std::vector<Finish>, std::greater<Finish>> running;
    long long now = 0;
    std::size_t idle = workers;
    while (!ready.empty() || !running.empty()) {
        while (idle > 0 && !ready.empty()) {
            const std::size_t task = ready.top();
            ready.pop();
            running.emplace(now + tasks[task].duration, task);
            --idle;
        }
        now = running.top().first;
        // Finish everything scheduled for this exact instant before starting anything new.
        while (!running.empty() && running.top().first == now) {
            const std::size_t done = running.top().second;
            running.pop();
            ++idle;
            plan.makespan = std::max(plan.makespan, now);
            for (const std::size_t w : successors[done]) {
                if (--pending[w] == 0) {
                    ready.push(w);
                }
            }
        }
    }
    return plan;
}
