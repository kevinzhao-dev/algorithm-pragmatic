#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "forkjoin.hpp"

// Stage 1
struct Cost {
    long long work = 0;
    long long span = 0;

    double parallelism() const {
        return span == 0 ? 1.0 : static_cast<double>(work) / static_cast<double>(span);
    }
};

inline Cost series(Cost a, Cost b) { return {a.work + b.work, a.span + b.span}; }
inline Cost parallel(Cost a, Cost b) { return {a.work + b.work, std::max(a.span, b.span)}; }

// Stage 2
class Meter {
public:
    void charge(long long units = 1) {
        current_.work += units;
        current_.span += units;
    }

    template <typename F, typename G>
    void fork2(F&& f, G&& g) {
        ++spawns_;
        charge();  // the fork itself runs on the strand that forks
        const Cost before = current_;
        current_ = Cost{};
        f();
        const Cost left = current_;
        current_ = Cost{};
        g();
        const Cost right = current_;
        current_ = series(before, parallel(left, right));
    }

    template <typename Body>
    void parallel_for(std::size_t begin, std::size_t end, Body&& body, std::size_t grain = 1) {
        if (end <= begin) {
            return;
        }
        if (end - begin <= std::max<std::size_t>(grain, 1)) {
            for (std::size_t i = begin; i < end; ++i) {
                body(i);
            }
            return;
        }
        const std::size_t middle = begin + (end - begin) / 2;
        fork2([&] { parallel_for(begin, middle, body, grain); },
              [&] { parallel_for(middle, end, body, grain); });
    }

    Cost cost() const { return current_; }
    long long spawns() const { return spawns_; }

private:
    Cost current_;
    long long spawns_ = 0;
};

// Stage 3
template <typename Exec>
long long fib(Exec& exec, int n, int cutoff) {
    exec.charge();
    if (n <= 1) {
        return n;
    }
    if (n <= cutoff) {
        return fib(exec, n - 1, cutoff) + fib(exec, n - 2, cutoff);
    }
    long long a = 0;
    long long b = 0;
    exec.fork2([&] { a = fib(exec, n - 1, cutoff); }, [&] { b = fib(exec, n - 2, cutoff); });
    return a + b;
}

// Stage 4
class ForkJoinExec {
public:
    void charge(long long = 1) const {}

    template <typename F, typename G>
    void fork2(F&& f, G&& g) const {
        practice::fork2(std::forward<F>(f), std::forward<G>(g));
    }

    template <typename Body>
    void parallel_for(std::size_t begin, std::size_t end, Body&& body, std::size_t grain = 1) const {
        practice::parallel_for(begin, end, std::forward<Body>(body), grain);
    }
};

namespace detail_0501 {

// Sums [begin, end) by divide and conquer so that nothing is shared between strands.
template <typename Exec>
long long sum_range(Exec& exec, std::span<const int> data, std::size_t begin, std::size_t end,
                    std::size_t grain) {
    if (end - begin <= std::max<std::size_t>(grain, 1)) {
        long long total = 0;
        for (std::size_t i = begin; i < end; ++i) {
            exec.charge();
            total += data[i];
        }
        return total;
    }
    const std::size_t middle = begin + (end - begin) / 2;
    long long left = 0;
    long long right = 0;
    exec.fork2([&] { left = sum_range(exec, data, begin, middle, grain); },
               [&] { right = sum_range(exec, data, middle, end, grain); });
    return left + right;
}

}  // namespace detail_0501

template <typename Exec>
long long parallel_sum(Exec& exec, std::span<const int> data, std::size_t grain) {
    if (data.empty()) {
        return 0;
    }
    return detail_0501::sum_range(exec, data, 0, data.size(), grain);
}

template <typename Exec>
void transpose_in_place(Exec& exec, std::span<int> grid, std::size_t n, std::size_t grain) {
    exec.parallel_for(0, n, [&](std::size_t i) {
        // Each unordered pair (i, j), j > i, is touched by exactly one strand: strands with
        // different i write disjoint cells, so there is no race.
        exec.parallel_for(i + 1, n, [&, i](std::size_t j) {
            exec.charge();
            std::swap(grid[i * n + j], grid[j * n + i]);
        }, grain);
    }, grain);
}

// Stage 5
struct SpeedupBounds {
    double lower = 0;
    double upper = 0;
};

inline SpeedupBounds speedup_bounds(Cost cost, int processors) {
    const auto p = static_cast<double>(processors);
    const auto t1 = static_cast<double>(cost.work);
    const auto tinf = static_cast<double>(cost.span);
    return {t1 / (t1 / p + tinf), std::min(p, cost.parallelism())};
}
